#include "MainFrame.h"

#include "CrankConfigPanel.h"
#include "core/common/engine_params_builder.h"

// core calculators / export
#include "core/Kinematic/Calculations/KinematicCalculator.h"
#include "core/Dynamic/Calculations/DynamicCalculator.h"
#include "core/Dynamic/output_data/dynamic_output.h"
#include "dynamic_input.h"
#include "core/common/common_types.h"

// wxWidgets
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/any.h>
#include <wx/event.h>
#include <wx/thread.h>
#include <wx/log.h>
#include <wx/spinctrl.h>   // wxSpinCtrl
#include <wx/grid.h>       // wxGrid

// STL
#include <memory>
#include <sstream>
#include <set>
#include <algorithm>
#include <cmath>

// -------------------------------
// Пользовательские события потоков
// -------------------------------
wxDEFINE_EVENT(wxEVT_KINEMATIC_COMPLETE, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_DYNAMIC_COMPLETE,  wxThreadEvent);

// -------------------------------
// ID элементов управления
// -------------------------------
enum
{
    ID_SidebarKinematic = wxID_HIGHEST + 1,
    ID_SidebarDynamic,
    ID_SidebarResults,

    ID_CalcButton,
    ID_BackButton,
    ID_SaveCSVButton,
    ID_SaveTXTButton,

    ID_GraphTypeChoice,
    ID_CylinderCheckList,
    ID_SelectAllBtn,
    ID_ClearAllBtn,

    ID_SaveGraphPNG,
    ID_TemplateChoice,

    ID_SaveSessionBtn,
    ID_LoadSessionBtn,

    ID_DynUnitChoice,
    ID_DynLoadBtn,
    ID_DynCalcBtn,
    ID_DynGraphTypeChoice,
    ID_DynBackBtn,
    ID_DynCylinderChoice,
    ID_DynCylinderCheckList,

    ID_DynSaveCSVWideButton,
    ID_DynSaveCSVLongButton,
    ID_DynSaveTXTButton,

    ID_DynSelectAllCylBtn,
    ID_DynClearAllCylBtn,

    ID_SessionMenuBtn,
    ID_ExportKinematicBtn,
    ID_ExportDynamicBtn,

    ID_MenuSessionSave,
    ID_MenuSessionLoad,

    ID_MenuKinExportCSV,
    ID_MenuKinExportTXT,
    ID_MenuKinExportPNG,

    ID_MenuDynExportCSVWide,
    ID_MenuDynExportCSVLong,
    ID_MenuDynExportTXT,

    ID_GoToDynamicBtn,
    ID_DynNextBtn,
    ID_DynPrevBtn
};

// -------------------------------
// Таблица событий wxWidgets
// -------------------------------
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_SidebarKinematic, MainFrame::OnSidebarKinematic)
    EVT_BUTTON(ID_SidebarDynamic,   MainFrame::OnSidebarStep2Pressure)
    EVT_BUTTON(ID_SidebarResults,   MainFrame::OnSidebarStep3Results)

    EVT_BUTTON(ID_CalcButton,       MainFrame::OnCalculate)

    EVT_CHOICE(ID_GraphTypeChoice,  MainFrame::OnGraphTypeChanged)
    EVT_CHECKLISTBOX(ID_CylinderCheckList, MainFrame::OnCylinderCheckListChanged)

    EVT_BUTTON(ID_SelectAllBtn,     MainFrame::OnSelectAll)
    EVT_BUTTON(ID_ClearAllBtn,      MainFrame::OnClearAll)
    EVT_BUTTON(ID_BackButton,       MainFrame::OnBackToInput)

    EVT_BUTTON(ID_SaveCSVButton,    MainFrame::OnSaveCsv)
    EVT_BUTTON(ID_SaveTXTButton,    MainFrame::OnSaveTxt)
    EVT_BUTTON(ID_SaveGraphPNG,     MainFrame::OnSaveGraphPNG)


    EVT_BUTTON(ID_SaveSessionBtn,   MainFrame::OnSaveSession)
    EVT_BUTTON(ID_LoadSessionBtn,   MainFrame::OnLoadSession)

    EVT_BUTTON(ID_DynLoadBtn,       MainFrame::OnLoadPressure)
    EVT_BUTTON(ID_DynCalcBtn,       MainFrame::OnCalculateDynamic)
    EVT_CHOICE(ID_DynGraphTypeChoice, MainFrame::OnDynGraphTypeChanged)
    EVT_BUTTON(ID_DynBackBtn,       MainFrame::OnDynamicBackToInput)

    EVT_CHECKLISTBOX(ID_DynCylinderCheckList, MainFrame::OnDynCylinderCheckListChanged)
    EVT_BUTTON(ID_DynSaveCSVWideButton, MainFrame::OnDynSaveCsvWide)
    EVT_BUTTON(ID_DynSaveCSVLongButton, MainFrame::OnDynSaveCsvLong)
    EVT_BUTTON(ID_DynSaveTXTButton,     MainFrame::OnDynSaveTxt)

    EVT_BUTTON(ID_DynSelectAllCylBtn, MainFrame::OnDynSelectAllDynCylinders)
    EVT_BUTTON(ID_DynClearAllCylBtn,  MainFrame::OnDynClearAllDynCylinders)

    EVT_BUTTON(ID_SessionMenuBtn,     MainFrame::OnSessionMenu)
    EVT_BUTTON(ID_ExportKinematicBtn, MainFrame::OnExportKinematicMenu)
    EVT_BUTTON(ID_ExportDynamicBtn,   MainFrame::OnExportDynamicMenu)
    EVT_BUTTON(ID_GoToDynamicBtn,     MainFrame::OnGoToDynamic)
    EVT_BUTTON(ID_DynNextBtn, MainFrame::OnDynNext)
    EVT_BUTTON(ID_DynPrevBtn, MainFrame::OnDynPrev)
wxEND_EVENT_TABLE()

// =====================================================
// Утилиты компоновки (ДОЛЖНЫ БЫТЬ ОДИН РАЗ В ФАЙЛЕ)
// =====================================================

// Парсим строку порядка работы: "1-3-4-2", "1,3,4,2", "1 3 4 2"
static bool ParseFiringOrder(const wxString& s, int nCyl, std::vector<int>& outOrder, wxString& err)
{
    outOrder.clear(); // сброс результата

    // Приводим разделители к пробелам
    std::string str = std::string(s.ToUTF8().data());
    for (char& c : str)
        if (c == ',' || c == ';' || c == '-' || c == '\t') c = ' ';

    // Читаем числа
    std::istringstream iss(str);
    int v = 0;
    while (iss >> v) outOrder.push_back(v);

    // Проверка количества
    if ((int)outOrder.size() != nCyl) {
        err = wxString::Format(wxString::FromUTF8("Нужно %d чисел."), nCyl);
        return false;
    }

    // Проверка уникальности
    std::set<int> uniq(outOrder.begin(), outOrder.end());
    if ((int)uniq.size() != nCyl) {
        err = wxString::FromUTF8("Есть повторы в порядке работы.");
        return false;
    }

    // Проверка диапазона 1..nCyl
    for (int x : outOrder) {
        if (x < 1 || x > nCyl) {
            err = wxString::Format(wxString::FromUTF8("Цилиндр %d вне 1..%d."), x, nCyl);
            return false;
        }
    }

    // Делаем так, чтобы цилиндр 1 начинался с фазы 0
    auto it = std::find(outOrder.begin(), outOrder.end(), 1);
    if (it != outOrder.end() && it != outOrder.begin())
        std::rotate(outOrder.begin(), it, outOrder.end());

    return true; // OK
}

// Преобразуем порядок работы в фазовые углы (равномерно по циклу 360/720)
static std::vector<double> PhasesFromOrder(const std::vector<int>& order, int nCyl, int taktnost)
{
    const double cycle = (taktnost == 2) ? 360.0 : 720.0;  // цикл по тактности
    const double step  = cycle / nCyl;                     // шаг между вспышками

    std::vector<double> phase(nCyl, 0.0); // phase[cyl-1] = угол фазы
    for (int k = 0; k < nCyl; ++k) {
        const int cyl = order[k];         // номер цилиндра 1..nCyl
        phase[cyl - 1] = k * step;        // фаза цилиндра
    }
    return phase;
}

// =====================================================
// Встраиваемая панель компоновки (без отдельного окна)
// =====================================================
class LayoutPanel final : public wxPanel
{
public:
    explicit LayoutPanel(wxWindow* parent)
        : wxPanel(parent)
    {
        auto* root = new wxBoxSizer(wxVERTICAL);

        // ---- Строка размеров: рядов/секций
        auto* dimSizer = new wxBoxSizer(wxHORIZONTAL);

        m_rowsLbl = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Число рядов:"));
        dimSizer->Add(m_rowsLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        m_rows = new wxSpinCtrl(this, wxID_ANY);
        m_rows->SetRange(1, 4);
        m_rows->SetValue(1);
        dimSizer->Add(m_rows, 0, wxRIGHT, 20);

        m_sectionsLbl = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Число секций:"));
        dimSizer->Add(m_sectionsLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        m_sections = new wxSpinCtrl(this, wxID_ANY);
        m_sections->SetRange(1, 32);
        m_sections->SetValue(1);
        dimSizer->Add(m_sections, 0);

        root->Add(dimSizer, 0, wxALL, 8);

        // ---- Режим ввода: таблица / строка порядка
        auto* modeSizer = new wxBoxSizer(wxHORIZONTAL);

        m_modeAngles = new wxRadioButton(this, wxID_ANY, wxString::FromUTF8("Фазовые углы (таблица)"),
                                         wxDefaultPosition, wxDefaultSize, wxRB_GROUP);

        m_modeOrder = new wxRadioButton(this, wxID_ANY, wxString::FromUTF8("Порядок работы (строка)"));

        modeSizer->Add(m_modeAngles, 0, wxRIGHT, 18);
        modeSizer->Add(m_modeOrder,  0);

        root->Add(modeSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        // ---- Строка порядка + кнопка применить
        auto* orderSizer = new wxBoxSizer(wxHORIZONTAL);

        m_orderLbl = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Порядок:"));
        orderSizer->Add(m_orderLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        m_orderText = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8("1-3-4-2"));
        orderSizer->Add(m_orderText, 1, wxRIGHT, 10);

        m_applyOrderBtn = new wxButton(this, wxID_ANY, wxString::FromUTF8("Применить"));
        orderSizer->Add(m_applyOrderBtn, 0);

        root->Add(orderSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        // ---- Таблица фаз
        m_grid = new wxGrid(this, wxID_ANY);
        m_grid->CreateGrid(1, 1);

        // Тёмная тема для grid
        m_grid->SetBackgroundColour(wxColour(0x18, 0x1C, 0x22));
        m_grid->SetDefaultCellBackgroundColour(wxColour(0x25, 0x2A, 0x30));
        m_grid->SetDefaultCellTextColour(wxColour(0xF0, 0xF0, 0xF0));
        m_grid->SetLabelBackgroundColour(wxColour(0x18, 0x1C, 0x22));
        m_grid->SetLabelTextColour(wxColour(0xE0, 0xE0, 0xE0));
        m_grid->SetGridLineColour(wxColour(0x50, 0x58, 0x60));

        m_grid->EnableEditing(true);
        m_grid->EnableGridLines(true);
        m_grid->SetRowLabelSize(70);

        root->Add(m_grid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        m_grid->Bind(wxEVT_GRID_CELL_CHANGED, [this](wxGridEvent&) {
            if (m_status) m_status->SetLabel(wxString::FromUTF8(" "));
        });

        // ---- Строка статуса
        m_status = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(" "));
        m_status->SetForegroundColour(wxColour(0xB0, 0xB0, 0xB0));
        root->Add(m_status, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        SetSizer(root);

        // ---- События: изменение размеров сетки
        m_rows->Bind(wxEVT_SPINCTRL,     [this](wxCommandEvent&) { Rebuild(); });
        m_sections->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) { Rebuild(); });

        // ---- События: переключение режима
        m_modeAngles->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { UpdateModeUI(); });
        m_modeOrder->Bind(wxEVT_RADIOBUTTON,  [this](wxCommandEvent&) { UpdateModeUI(); });

        // ---- Событие: применить порядок
        m_applyOrderBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ApplyOrder(); });

        UpdateModeUI();
        Rebuild();
    }

    // Запретить пользователю менять rows/sections (оставляем только авто из Configure/SetDims)
    void LockDimensions(bool lock)
    {
        if (m_rowsLbl)     m_rowsLbl->Show(!lock);
        if (m_sectionsLbl) m_sectionsLbl->Show(!lock);
        if (m_rows)        m_rows->Show(!lock);
        if (m_sections)    m_sections->Show(!lock);
        Layout();
    }

    // Оставить ТОЛЬКО таблицу углов: скрыть режимы и строку порядка
    void SetTableOnly(bool on)
    {
        if (!on) return;

        if (m_modeAngles) m_modeAngles->SetValue(true);

        if (m_modeAngles) m_modeAngles->Hide();
        if (m_modeOrder)  m_modeOrder->Hide();

        if (m_orderLbl)      m_orderLbl->Hide();
        if (m_orderText)     m_orderText->Hide();
        if (m_applyOrderBtn) m_applyOrderBtn->Hide();

        UpdateModeUI();
        Layout();
    }

    void SetInitialPhases(const std::vector<double>& phases)
    {
        if (m_nCyl <= 0) return;
        if ((int)phases.size() != m_nCyl) return;

        const int rows = GetRows();
        const int sec  = GetSections();
        if (rows * sec != m_nCyl) return;

        for (int s = 0; s < sec; ++s) {
            for (int r = 0; r < rows; ++r) {
                const int idx = s * rows + r;
                m_grid->SetCellValue(r, s, wxString::Format("%.0f", phases[idx]));
            }
        }
        if (m_status) m_status->SetLabel(wxString::FromUTF8("Фазы восстановлены."));
    }

    void SetDims(int rows, int sections)
    {
        rows = std::max(1, rows);
        sections = std::max(1, sections);

        if (rows > 4) rows = 4;
        if (sections > 32) sections = 32;

        if (rows * sections != m_nCyl) return;

        m_rows->SetValue(rows);
        m_sections->SetValue(sections);
        Rebuild();
    }

    void Configure(int nCyl, int taktnost)
    {
        m_nCyl = std::max(1, nCyl);
        m_taktnost = (taktnost == 2) ? 2 : 4;

        int rows = m_rows->GetValue();
        int sec  = m_sections->GetValue();

        if (rows * sec != m_nCyl) {
            m_rows->SetValue(1);
            m_sections->SetValue(m_nCyl);
        }

        Rebuild();
    }

    int GetRows() const { return m_rows ? m_rows->GetValue() : 1; }
    int GetSections() const { return m_sections ? m_sections->GetValue() : 1; }

    bool GetPhases(std::vector<double>& out, wxString& err) const
    {
        const int rows = GetRows();
        const int sec  = GetSections();

        if (rows * sec != m_nCyl) {
            err = wxString::Format(wxString::FromUTF8("rows*sections должно быть %d."), m_nCyl);
            return false;
        }

        out.assign(m_nCyl, 0.0);

        const double cycle = (m_taktnost == 2) ? 360.0 : 720.0;

        for (int s = 0; s < sec; ++s) {
            for (int r = 0; r < rows; ++r) {
                const int idx = s * rows + r;

                wxString cell = m_grid->GetCellValue(r, s);
                cell.Replace(",", ".");

                double v = 0.0;
                if (!cell.ToDouble(&v)) {
                    err = wxString::Format(wxString::FromUTF8("Некорректное число: ряд %d, секция %d."),
                                           r + 1, s + 1);
                    return false;
                }

                v = std::fmod(v, cycle);
                if (v < 0) v += cycle;

                out[idx] = v;
            }
        }

        // предупреждение о совпадениях (по 1 градусу)
        std::set<int> uniq;
        bool hasDup = false;
        for (double a : out) {
            int key = (int)std::lround(a);
            if (!uniq.insert(key).second) { hasDup = true; break; }
        }

        if (m_status) {
            m_status->SetLabel(hasDup
                ? wxString::FromUTF8("⚠ Есть совпадающие фазы (одновременные события).")
                : wxString::FromUTF8(" "));
        }

        return true;
    }

private:
    void UpdateModeUI()
    {
        const bool orderMode = (m_modeOrder && m_modeOrder->GetValue());

        if (m_orderText)     m_orderText->Enable(orderMode);
        if (m_applyOrderBtn) m_applyOrderBtn->Enable(orderMode);

        if (m_orderLbl) m_orderLbl->Enable(orderMode);
    }

    void Rebuild()
    {
        const int rows = GetRows();
        const int sec  = GetSections();

        if (m_grid->GetNumberRows() > 0) m_grid->DeleteRows(0, m_grid->GetNumberRows());
        if (m_grid->GetNumberCols() > 0) m_grid->DeleteCols(0, m_grid->GetNumberCols());

        m_grid->AppendRows(rows);
        m_grid->AppendCols(sec);

        for (int r = 0; r < rows; ++r)
            m_grid->SetRowLabelValue(r, wxString::Format(wxString::FromUTF8("Ряд %d"), r + 1));
        for (int c = 0; c < sec; ++c)
            m_grid->SetColLabelValue(c, wxString::Format(wxString::FromUTF8("Секция %d"), c + 1));

        // дефолт: равномерно
        const double cycle = (m_taktnost == 2) ? 360.0 : 720.0;
        const double step  = cycle / std::max(1, m_nCyl);

        for (int s = 0; s < sec; ++s) {
            for (int r = 0; r < rows; ++r) {
                const int idx = s * rows + r;
                if (idx < m_nCyl)
                    m_grid->SetCellValue(r, s, wxString::Format("%.0f", idx * step));
            }
        }

        m_grid->AutoSize();

        // ограничение высоты под rows
        int rowH = m_grid->GetDefaultRowSize();
        if (rowH <= 0) rowH = 24;

        const int headerH = 26;
        int gridH = headerH + rows * rowH + 6;

        const int minH = 120;
        const int maxH = 260;
        gridH = std::max(minH, std::min(maxH, gridH));

        m_grid->SetMinSize(wxSize(-1, gridH));
        m_grid->SetMaxSize(wxSize(-1, gridH));

        if (m_status) m_status->SetLabel(wxString::FromUTF8(" "));
        Layout();
        FitInside();
    }

    void ApplyOrder()
    {
        if (!m_modeOrder || !m_modeOrder->GetValue()) return;

        wxString err;
        std::vector<int> order;

        if (!ParseFiringOrder(m_orderText->GetValue(), m_nCyl, order, err)) {
            if (m_status) m_status->SetLabel(wxString::FromUTF8("⚠ ") + err);
            return;
        }

        const std::vector<double> phase = PhasesFromOrder(order, m_nCyl, m_taktnost);

        const int rows = GetRows();
        const int sec  = GetSections();
        if (rows * sec != m_nCyl) {
            if (m_status) m_status->SetLabel(wxString::FromUTF8("⚠ rows*sections не равно числу цилиндров."));
            return;
        }

        for (int s = 0; s < sec; ++s) {
            for (int r = 0; r < rows; ++r) {
                const int idx = s * rows + r;
                m_grid->SetCellValue(r, s, wxString::Format("%.0f", phase[idx]));
            }
        }

        if (m_status) m_status->SetLabel(wxString::FromUTF8("Порядок применён."));
    }

private:
    int m_nCyl = 1;
    int m_taktnost = 4;

    wxStaticText* m_rowsLbl = nullptr;
    wxStaticText* m_sectionsLbl = nullptr;
    wxStaticText* m_orderLbl = nullptr;

    wxSpinCtrl* m_rows = nullptr;
    wxSpinCtrl* m_sections = nullptr;

    wxRadioButton* m_modeAngles = nullptr;
    wxRadioButton* m_modeOrder  = nullptr;

    wxTextCtrl* m_orderText = nullptr;
    wxButton* m_applyOrderBtn = nullptr;

    wxGrid* m_grid = nullptr;
    wxStaticText* m_status = nullptr;
};

// =====================================================
// Потоки расчёта (детачед, результат через wxThreadEvent)
// =====================================================

// Поток расчёта кинематики
class KinematicThread final : public wxThread
{
public:
    KinematicThread(wxEvtHandler* parent, const EngineParams& params)
        : wxThread(wxTHREAD_DETACHED) // сам себя удалит после завершения
        , m_parent(parent)            // куда отправим событие
        , m_params(params)            // копия параметров
    {}

protected:
    ExitCode Entry() override
    {
        auto* evt = new wxThreadEvent(wxEVT_KINEMATIC_COMPLETE); // событие в GUI

        try {
            KinematicCalculator calc(m_params); // калькулятор
            auto results = std::make_shared<CalculationResults>(calc.calculateAll(nullptr, nullptr)); // расчёт
            evt->SetPayload(results);  // payload = данные
            evt->SetString(wxString()); // пусто = OK
        }
        catch (const std::exception& e) {
            evt->SetPayload(std::shared_ptr<CalculationResults>{}); // пусто
            evt->SetString(wxString::Format("Kinematic error: %s", e.what())); // текст ошибки
        }
        catch (...) {
            evt->SetPayload(std::shared_ptr<CalculationResults>{});
            evt->SetString("Kinematic unknown error");
        }

        wxQueueEvent(m_parent, evt); // отправляем в главный поток
        return (ExitCode)0;
    }

private:
    wxEvtHandler* m_parent = nullptr; // receiver
    EngineParams m_params;            // input params
};

// Поток расчёта динамики
class DynamicThread final : public wxThread
{
public:
    DynamicThread(wxEvtHandler* parent,
                  const EngineParams& engineParams,
                  const CalculationResults& kinematicResults,
                  const std::vector<double>& pressureAngles,
                  const std::vector<double>& pressureValues,
                  double massPiston,
                  double massRod,
                  double kRodOsc,
                  double bore)
        : wxThread(wxTHREAD_DETACHED)
        , m_parent(parent)
        , m_engineParams(engineParams)
        , m_kinematicResults(kinematicResults)
        , m_pressureAngles(pressureAngles)
        , m_pressureValues(pressureValues)
        , m_massPiston(massPiston)
        , m_massRod(massRod)
        , m_kRodOsc(kRodOsc)
        , m_bore(bore)
    {}

protected:
    ExitCode Entry() override
    {
        auto* evt = new wxThreadEvent(wxEVT_DYNAMIC_COMPLETE); // событие в GUI

        try {
            DynamicCalculator calc(
                m_engineParams,
                m_kinematicResults,
                m_pressureAngles,
                m_pressureValues,
                m_massPiston,
                m_massRod,
                m_kRodOsc,
                m_bore
            );

            DynamicResults results = calc.calculate(nullptr, nullptr); // расчёт динамики

            evt->SetPayload(std::make_shared<DynamicResults>(std::move(results))); // payload = данные
            evt->SetString(wxString()); // пусто = OK
        }
        catch (const std::exception& e) {
            evt->SetPayload(std::shared_ptr<DynamicResults>{});
            evt->SetString(wxString::Format("Dynamic error: %s", e.what()));
        }
        catch (...) {
            evt->SetPayload(std::shared_ptr<DynamicResults>{});
            evt->SetString("Dynamic unknown error");
        }

        wxQueueEvent(m_parent, evt); // отправка в GUI поток
        return (ExitCode)0;
    }

private:
    wxEvtHandler* m_parent = nullptr; // receiver

    EngineParams m_engineParams;               // входные параметры двигателя
    CalculationResults m_kinematicResults;     // результаты кинематики
    std::vector<double> m_pressureAngles;      // углы давления
    std::vector<double> m_pressureValues;      // давление

    double m_massPiston = 0.0; // масса поршня
    double m_massRod    = 0.0; // масса шатуна
    double m_kRodOsc    = 0.0; // доля возвратно-поступательной массы шатуна
    double m_bore       = 0.0; // диаметр цилиндра
};

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1150, 720))
{
    BuildLayout();
    UpdateSidebarSteps();

    Bind(wxEVT_KINEMATIC_COMPLETE, &MainFrame::OnKinematicComplete, this);
    Bind(wxEVT_DYNAMIC_COMPLETE,   &MainFrame::OnDynamicComplete,   this);

    Centre();
}

static void NormalizePressureFromDieselRK(std::vector<double>& angles,
                                          std::vector<double>& pressures,
                                          double cycleDeg)
{
    // Diesel-RK: 0° = начало сжатия (НМТ).
    // В программе: 0° = ВМТ конца сжатия => смещение -180° по оси угла.
    const double shift = 180.0;

    const size_t n = std::min(angles.size(), pressures.size());
    if (n == 0) { angles.clear(); pressures.clear(); return; }

    std::vector<std::pair<double,double>> ap;
    ap.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        double a = std::fmod(angles[i] - shift, cycleDeg);
        if (a < 0) a += cycleDeg;
        ap.emplace_back(a, pressures[i]);
    }

    // сортируем по углу, чтобы интерполяция работала корректно
    std::sort(ap.begin(), ap.end(),
              [](const auto& x, const auto& y){ return x.first < y.first; });

    angles.resize(n);
    pressures.resize(n);
    for (size_t i = 0; i < n; ++i) {
        angles[i] = ap[i].first;
        pressures[i] = ap[i].second;
    }
}


// Парсит список double из строки вида: "0,180,360" или "0 180 360" или "0;180;360"
static bool ParseDoubleList(const wxString& s, std::vector<double>& out)
{
    out.clear();

    wxString t = s;
    t.Replace(";", " ");
    t.Replace("\t", " ");
    t.Replace(",", " ");   // важно: тут запятая как разделитель (не как десятичная)
    t.Replace("\r", " ");
    t.Replace("\n", " ");

    wxStringTokenizer tok(t, " ");
    while (tok.HasMoreTokens())
    {
        wxString item = tok.GetNextToken();
        item.Trim(true).Trim(false);
        if (item.empty()) continue;

        item.Replace(",", "."); // если вдруг десятичные с запятой (на всякий)
        double v = 0.0;
        if (!item.ToDouble(&v)) return false;
        out.push_back(v);
    }

    return true;
}

void MainFrame::BuildLayout()
{
    auto* mainPanel = new wxPanel(this);
    ApplyDarkTheme(mainPanel);

    auto* hbox = new wxBoxSizer(wxHORIZONTAL);

    // Левая панель навигации
    m_sidebarPanel = new wxPanel(mainPanel);
    ApplyDarkTheme(m_sidebarPanel);
    BuildSidebar(m_sidebarPanel);

    // Правая колонка
    auto* rightPanel = new wxPanel(mainPanel);
    ApplyDarkTheme(rightPanel);

    m_rightBook = new wxSimplebook(rightPanel, wxID_ANY);
    ApplyDarkTheme(m_rightBook);

    // Страница кинематики
    auto* kinPage = new wxPanel(m_rightBook);
    ApplyDarkTheme(kinPage);
    BuildKinematicPages(kinPage);
    m_rightBook->AddPage(kinPage, wxString::FromUTF8("Расчёт кинематики"), true);

    // Страница динамики 
    auto* dynPage = new wxPanel(m_rightBook);
    ApplyDarkTheme(dynPage);
    BuildDynamicPages(dynPage);
m_rightBook->AddPage(dynPage, wxString::FromUTF8("Расчёт динамики"), false);

    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(m_rightBook, 1, wxEXPAND | wxALL, 10);
    rightPanel->SetSizer(rightSizer);

    hbox->Add(m_sidebarPanel, 0, wxEXPAND);
    hbox->Add(rightPanel, 1, wxEXPAND);

    mainPanel->SetSizer(hbox);
}

void MainFrame::BuildSidebar(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* title = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("РАСЧЁТ ДВИГАТЕЛЯ"));
    title->SetForegroundColour(*wxLIGHT_GREY);
    title->SetFont(wxFontInfo(12).Bold());
    vbox->Add(title, 0, wxALL, 20);

    auto makeStepRow = [&](wxStaticText*& statusOut, wxButton*& btnOut,
                           int id, const wxString& text)
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);

        statusOut = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("⭕"));
        statusOut->SetMinSize(wxSize(24, -1));
        statusOut->SetFont(wxFontInfo(12).Bold());
        ApplyDarkTheme(statusOut);

        btnOut = new wxButton(parent, id, text,
                              wxDefaultPosition, wxSize(0, 40), wxBORDER_NONE);
        ApplyDarkTheme(btnOut);

        row->Add(statusOut, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        row->Add(btnOut,    1, wxEXPAND | wxLEFT | wxRIGHT, 6);
        vbox->Add(row, 0, wxEXPAND | wxTOP, 8);
    };

    makeStepRow(m_step1Status, m_btnStep1Params,   ID_SidebarKinematic,
                wxString::FromUTF8("1. Параметры и кинематика"));

    makeStepRow(m_step2Status, m_btnStep2Pressure, ID_SidebarDynamic,
                wxString::FromUTF8("2. Диаграмма давления"));

    makeStepRow(m_step3Status, m_btnStep3Results,  ID_SidebarResults,
                wxString::FromUTF8("3. Динамика и экспорт"));

    vbox->AddStretchSpacer();
    parent->SetSizer(vbox);
}

void MainFrame::OnSidebarKinematic(wxCommandEvent&)
{
    m_rightBook->SetSelection(0);
    if (!m_kinematicBook) return;

    if (m_hasResults)
        m_kinematicBook->SetSelection(1); // результаты
    else
        m_kinematicBook->SetSelection(0); // ввод
}

void MainFrame::OnSidebarStep2Pressure(wxCommandEvent&)
{
    m_rightBook->SetSelection(1);
    if (m_dynamicBook) m_dynamicBook->SetSelection(0); // на ввод динамики (загрузка файла)
}

void MainFrame::OnSidebarStep3Results(wxCommandEvent&)
{
    m_rightBook->SetSelection(1);
    if (!m_dynamicBook) return;

    if (m_hasDynamicResults)
    m_dynamicBook->SetSelection(2); // results page
else
    m_dynamicBook->SetSelection(0); // pressure page
}

void MainFrame::UpdateSidebarSteps()
{
    auto setStatus = [&](wxStaticText* st, int state)
    {
        if (!st) return;
        if (state == 1)      st->SetLabel(wxString::FromUTF8("✅"));
        else if (state == 2) st->SetLabel(wxString::FromUTF8("⚠"));
        else                 st->SetLabel(wxString::FromUTF8("⭕"));
    };

    const bool pressureOk =
        !m_dynPressureAngles.empty() &&
        (m_dynPressureAngles.size() == m_dynPressureValues.size());

    // Шаг 1: кинематика
    setStatus(m_step1Status, m_hasResults ? 1 : 0);

    // Шаг 2: диаграмма давления
    setStatus(m_step2Status, pressureOk ? 1 : 0);

    // Шаг 3: динамика
    setStatus(m_step3Status, m_hasDynamicResults ? 1 : 0);

    // Кнопка "Перейти к динамике"
    if (m_goToDynamicBtn)
    m_goToDynamicBtn->Enable(m_hasResults);

    if (m_dynNextBtn)
    m_dynNextBtn->Enable(m_hasResults && pressureOk);

    // Кнопка "Рассчитать динамику"
    if (m_dynCalcBtn)
        m_dynCalcBtn->Enable(m_hasResults && pressureOk);

    if (m_sidebarPanel) m_sidebarPanel->Layout();


}

void MainFrame::BuildKinematicPages(wxPanel* parent)
{
    auto* pageSizer = new wxBoxSizer(wxVERTICAL);

    // -------------------------
    // Заголовок
    // -------------------------
    {
        auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* title = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("РАСЧЁТ КИНЕМАТИКИ"));
        title->SetForegroundColour(*wxWHITE);
        title->SetFont(wxFontInfo(16).Bold());
        headerSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL);

        headerSizer->AddStretchSpacer();

        pageSizer->Add(headerSizer, 0, wxALL | wxEXPAND, 10);
        pageSizer->Add(new wxStaticLine(parent), 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
    }

    // -------------------------
    // Book: ввод / результаты
    // -------------------------
    m_kinematicBook = new wxSimplebook(parent, wxID_ANY);
    ApplyDarkTheme(m_kinematicBook);

    // =========================================================
    // 1) СТРАНИЦА ВВОДА
    // =========================================================
    m_kinInputPanel = new wxPanel(m_kinematicBook);
    ApplyDarkTheme(m_kinInputPanel);

    auto* inputSizer = new wxBoxSizer(wxVERTICAL);

    
    // -------------------------
    // Главный блок ввода: CrankConfigPanel (внутри всё: тактность, кривошипы, фазы, шатуны, γ/e, α/r/λ/n)
    // -------------------------
    m_crankPanel = new CrankConfigPanel(m_kinInputPanel);
    ApplyDarkTheme(m_crankPanel);
    inputSizer->Add(m_crankPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);

    // -------------------------
    // Кнопка "РАССЧИТАТЬ"
    // -------------------------
    m_calcButton = new wxButton(m_kinInputPanel, ID_CalcButton, wxString::FromUTF8("РАССЧИТАТЬ"));
    ApplyDarkTheme(m_calcButton);
    inputSizer->Add(m_calcButton, 0, wxALIGN_RIGHT | wxRIGHT | wxTOP | wxBOTTOM, 16);

    // -------------------------
    // Инструменты (сессия)
    // -------------------------
    {
        auto* toolsSizer = new wxBoxSizer(wxHORIZONTAL);

        m_sessionMenuBtn = new wxButton(m_kinInputPanel, ID_SessionMenuBtn, wxString::FromUTF8("Сессия ▾"));
        ApplyDarkTheme(m_sessionMenuBtn);

        toolsSizer->Add(m_sessionMenuBtn, 0);
        inputSizer->Add(toolsSizer, 0, wxALIGN_LEFT | wxLEFT | wxBOTTOM, 16);
    }

    m_kinInputPanel->SetSizer(inputSizer);

    // =========================================================
    // 2) СТРАНИЦА РЕЗУЛЬТАТОВ
    // =========================================================
    m_kinResultPanel = new wxPanel(m_kinematicBook);
    ApplyDarkTheme(m_kinResultPanel);

    auto* resSizer = new wxBoxSizer(wxVERTICAL);

    m_resultStatus = new wxStaticText(m_kinResultPanel, wxID_ANY, wxString::FromUTF8("Расчёт ещё не выполнялся."));
    ApplyDarkTheme(m_resultStatus);
    resSizer->Add(m_resultStatus, 0, wxALL, 10);

    m_resultBook = new wxNotebook(m_kinResultPanel, wxID_ANY);
    ApplyDarkTheme(m_resultBook);
    m_resultBook->SetBackgroundColour(wxColour(0x25, 0x2A, 0x30));

    // --- График
    {
        auto* graphPage = new wxPanel(m_resultBook);
        ApplyDarkTheme(graphPage);

        auto* graphSizer = new wxBoxSizer(wxVERTICAL);
        auto* topSizer   = new wxBoxSizer(wxHORIZONTAL);

        auto* ctrlSizer = new wxBoxSizer(wxVERTICAL);

        auto* graphLabel = new wxStaticText(graphPage, wxID_ANY, wxString::FromUTF8("Тип графика:"));
        ApplyDarkTheme(graphLabel);
        ctrlSizer->Add(graphLabel, 0, wxBOTTOM, 5);

        wxArrayString graphTypes;
        graphTypes.Add(wxString::FromUTF8("Перемещение"));
        graphTypes.Add(wxString::FromUTF8("Скорость"));
        graphTypes.Add(wxString::FromUTF8("Ускорение"));

        m_graphTypeChoice = new wxChoice(graphPage, ID_GraphTypeChoice, wxDefaultPosition, wxSize(180, -1), graphTypes);
        m_graphTypeChoice->SetSelection(0);
        ApplyDarkTheme(m_graphTypeChoice);
        ctrlSizer->Add(m_graphTypeChoice, 0, wxBOTTOM, 10);

        auto* selectLabel = new wxStaticText(graphPage, wxID_ANY, wxString::FromUTF8("Отобразить:"));
        ApplyDarkTheme(selectLabel);
        ctrlSizer->Add(selectLabel, 0, wxBOTTOM, 5);

        m_cylinderCheckList = new wxCheckListBox(graphPage, ID_CylinderCheckList, wxDefaultPosition, wxSize(180, 150));
        ApplyDarkTheme(m_cylinderCheckList);
        ctrlSizer->Add(m_cylinderCheckList, 1, wxEXPAND | wxBOTTOM, 10);

        auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
        m_selectAllBtn = new wxButton(graphPage, ID_SelectAllBtn, wxString::FromUTF8("Выбрать все"),
                                      wxDefaultPosition, wxSize(90, -1));
        m_clearAllBtn  = new wxButton(graphPage, ID_ClearAllBtn, wxString::FromUTF8("Очистить всё"),
                                      wxDefaultPosition, wxSize(90, -1));
        ApplyDarkTheme(m_selectAllBtn);
        ApplyDarkTheme(m_clearAllBtn);
        btnSizer->Add(m_selectAllBtn, 1, wxRIGHT, 5);
        btnSizer->Add(m_clearAllBtn,  1);
        ctrlSizer->Add(btnSizer, 0, wxEXPAND | wxBOTTOM, 10);

        topSizer->Add(ctrlSizer, 0, wxALL | wxALIGN_TOP, 10);

        m_plotPanel = new KinematicPlotPanel(graphPage);
        topSizer->Add(m_plotPanel, 1, wxEXPAND | wxALL, 10);

        graphSizer->Add(topSizer, 1, wxEXPAND);
        graphPage->SetSizer(graphSizer);

        m_resultBook->AddPage(graphPage, wxString::FromUTF8("График"), true);
    }

    // --- Таблица
    {
        auto* tablePage = new wxPanel(m_resultBook);
        ApplyDarkTheme(tablePage);

        auto* tableSizer = new wxBoxSizer(wxVERTICAL);
        m_tablePanel = new KinematicTablePanel(tablePage);
        tableSizer->Add(m_tablePanel, 1, wxEXPAND | wxALL, 5);

        tablePage->SetSizer(tableSizer);
        m_resultBook->AddPage(tablePage, wxString::FromUTF8("Таблица"), false);
    }

    resSizer->Add(m_resultBook, 1, wxEXPAND | wxALL, 5);

    // Нижние кнопки
    {
        auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

        m_backButton = new wxButton(m_kinResultPanel, ID_BackButton, wxString::FromUTF8("← Назад"));
        ApplyDarkTheme(m_backButton);
        bottomSizer->Add(m_backButton, 0, wxRIGHT, 10);

        m_goToDynamicBtn = new wxButton(m_kinResultPanel, ID_GoToDynamicBtn, wxString::FromUTF8("Дальше →"));
        ApplyDarkTheme(m_goToDynamicBtn);
        m_goToDynamicBtn->Enable(false);
        bottomSizer->Add(m_goToDynamicBtn, 0, wxRIGHT, 10);

        m_exportKinBtn = new wxButton(m_kinResultPanel, ID_ExportKinematicBtn, wxString::FromUTF8("Экспорт ▾"));
        ApplyDarkTheme(m_exportKinBtn);
        bottomSizer->Add(m_exportKinBtn, 0);

        resSizer->Add(bottomSizer, 0, wxALIGN_LEFT | wxALL, 10);
    }

    m_kinResultPanel->SetSizer(resSizer);

    if (m_graphTypeChoice)
        m_graphTypeChoice->SetToolTip(wxString::FromUTF8("Выберите тип отображаемой зависимости"));
    if (m_cylinderCheckList)
        m_cylinderCheckList->SetToolTip(wxString::FromUTF8("Выберите цилиндры для отображения"));

    // -------------------------
    // Добавляем страницы
    // -------------------------
    m_kinematicBook->AddPage(m_kinInputPanel,  wxString::FromUTF8("Ввод параметров"), true);
    m_kinematicBook->AddPage(m_kinResultPanel, wxString::FromUTF8("Результаты"), false);

    pageSizer->Add(m_kinematicBook, 1, wxEXPAND | wxALL, 5);
    parent->SetSizer(pageSizer);

    // первичная валидность кнопки
    UpdateCalculateButtonState();
}

// =====================================================================
//  Вспомогательные функции
// =====================================================================

void MainFrame::ApplyDarkTheme(wxWindow* w)
{
    if (!w) return;
    w->SetBackgroundColour(wxColour(0x18, 0x1C, 0x22));
    w->SetForegroundColour(*wxWHITE);
    for (auto* child : w->GetChildren())
        ApplyDarkTheme(child);
}


void MainFrame::UpdateParameterVisibility()
{
    bool showGamma      = false;
    bool showDezax      = false;
    bool showGammaPric  = false;
    bool showR1         = false;
    bool showL1         = false;

    
    

    m_gammaLabel->Show(showGamma);
    m_gammaInput->Show(showGamma);
    m_dezaxLabel->Show(showDezax);
    m_dezaxInput->Show(showDezax);
    m_gammaPricLabel->Show(showGammaPric);
    m_gammaPricInput->Show(showGammaPric);
    m_radcrank1Label->Show(showR1);
    m_radcrank1Input->Show(showR1);
    m_lengthRod1Label->Show(showL1);
    m_lengthRod1Input->Show(showL1);

    if (m_graphTypeChoice) {
    int currentSel = m_graphTypeChoice->GetSelection();
    if (currentSel < 0 || currentSel > 2) currentSel = 0;

    m_graphTypeChoice->Clear();
    m_graphTypeChoice->Append(wxString::FromUTF8("Перемещение"));
    m_graphTypeChoice->Append(wxString::FromUTF8("Скорость"));
    m_graphTypeChoice->Append(wxString::FromUTF8("Ускорение"));

    m_graphTypeChoice->SetSelection(currentSel);
}

    m_kinInputPanel->Layout();
    if (m_kinResultPanel) m_kinResultPanel->Layout();
    UpdateCalculateButtonState();
}

void MainFrame::UpdateCylinderCheckList()
{
    if (!m_cylinderCheckList || !m_plotPanel) return;

    m_cylinderCheckList->Clear();

    const int nCyl = (int)m_lastParams.countCyl;
    if (nCyl <= 0) return;

    // Всегда показываем цилиндры 1..N (новая модель компоновки не "угадывает" V/оппозит по gamma)
    for (int i = 0; i < nCyl; ++i)
        m_cylinderCheckList->Append(wxString::Format(wxString::FromUTF8("Цилиндр %d"), i + 1));

    // По умолчанию включаем 1-й цилиндр
    if (m_cylinderCheckList->GetCount() > 0)
        m_cylinderCheckList->Check(0, true);

    
    // Применяем выбор в панель графика
    std::vector<int> selected;
    selected.reserve((size_t)nCyl);
    for (unsigned i = 0; i < m_cylinderCheckList->GetCount(); ++i) {
        if (m_cylinderCheckList->IsChecked(i))
            selected.push_back((int)i);
    }

    m_plotPanel->SetSelectedIndices(selected);

    
    m_plotPanel->Refresh();
}

bool MainFrame::ReadParamsFromUI(EngineParams& p, wxString& err)
{
    err.clear();
    p = EngineParams{}; // сброс

    // 1) Забираем всё из CrankConfigPanel
    if (!m_crankPanel) {
        err = wxString::FromUTF8("Не найдена панель конфигурации коленвала.");
        return false;
    }

    CrankConfig cfg;
    wxString cerr;
    if (!m_crankPanel->GetConfig(cfg, cerr)) {
        err = cerr;
        return false;
    }

    // 2) Собираем EngineBuildInput (чисто конфигурация КШМ/вала/цилиндров)
    EngineBuildInput in;
    in.taktnost = cfg.taktnost;
    in.crank_count = cfg.crank_count;
    in.cyl_per_crankpin = cfg.cyl_per_crankpin;
    in.articulated_rod = (cfg.rod_pair == RodPairType::Articulated);
    in.crank_phase_deg = cfg.crank_phase_deg;

    in.gamma_deg = cfg.gamma_deg;
    in.dezaxial_m = cfg.dezaxial_m;
    in.gamma_pric_deg = cfg.gamma_pric_deg;
    in.radcrank1_m = cfg.radcrank1_m;
    in.lengthRod1_m = cfg.lengthRod1_m;

    // (опоры — если у тебя поле есть в EngineBuildInput, заполни тут)
    // in.full_support_bearings = cfg.full_support_bearings;

    // 3) Строим EngineParams через builder (он должен посчитать countCyl, фазы цилиндров и т.д.)
    EngineParams tmp;
    std::string berr;
    if (!BuildEngineParamsFromCrank(in, tmp, berr)) {
        err = wxString::FromUTF8(berr.c_str());
        return false;
    }

    // 4) Дописываем базовые параметры кинематики (теперь они тоже в cfg)
    tmp.step_alpha = cfg.step_alpha_deg;
    tmp.radcrank   = cfg.r_m;
    tmp.lyambda    = cfg.lambda;
    tmp.n          = cfg.n_rpm;

    // 5) end_alpha согласуем с тактностью
    tmp.taktnost  = cfg.taktnost;
    tmp.end_alpha = (tmp.taktnost == 2) ? 360.0 : 720.0;

    p = std::move(tmp);
    return true;
}



void MainFrame::BuildDynamicPages(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // Заголовок
    auto* title = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("РАСЧЁТ ДИНАМИКИ"));
    title->SetForegroundColour(*wxWHITE);
    title->SetFont(wxFontInfo(16).Bold());
    vbox->Add(title, 0, wxALL, 10);
    vbox->Add(new wxStaticLine(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // Книжка ввод/результат
    m_dynamicBook = new wxSimplebook(parent, wxID_ANY);
    ApplyDarkTheme(m_dynamicBook);

    // Страница ввода
    // 1) Страница: Давление
m_dynPressurePage = new wxPanel(m_dynamicBook);
ApplyDarkTheme(m_dynPressurePage);
BuildDynamicPressurePage(m_dynPressurePage);

// 2) Страница: Углы вспышек
m_dynIgnitionPage = new wxPanel(m_dynamicBook);
ApplyDarkTheme(m_dynIgnitionPage);
BuildDynamicIgnitionPage(m_dynIgnitionPage);

// 3) Страница результатов
m_dynResultPage = new wxPanel(m_dynamicBook);
ApplyDarkTheme(m_dynResultPage);
BuildDynamicResultPage(m_dynResultPage);

m_dynamicBook->AddPage(m_dynPressurePage, wxString::FromUTF8("Давление"), true);
m_dynamicBook->AddPage(m_dynIgnitionPage, wxString::FromUTF8("Углы вспышек"), false);
m_dynamicBook->AddPage(m_dynResultPage, wxString::FromUTF8("Результаты"), false);

    vbox->Add(m_dynamicBook, 1, wxEXPAND | wxALL, 5);
    parent->SetSizer(vbox);
}

void MainFrame::BuildDynamicPressurePage(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // Группа "Массы"
    auto* massBox = new wxStaticBoxSizer(wxVERTICAL, parent, wxString::FromUTF8("Массы деталей"));
    ApplyDarkTheme(massBox->GetStaticBox());

    auto* massGrid = new wxFlexGridSizer(0, 2, 8, 12);
    massGrid->AddGrowableCol(1, 1);

    auto addMassRow = [&](const wxString& label, wxTextCtrl*& ctrl, const wxString& def, const wxString& tooltip) {
        auto* lbl = new wxStaticText(parent, wxID_ANY, label);
        massGrid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
        ctrl = new wxTextCtrl(parent, wxID_ANY, def);
        ctrl->SetToolTip(tooltip);
        massGrid->Add(ctrl, 1, wxEXPAND);
        ApplyDarkTheme(lbl);
        ApplyDarkTheme(ctrl);
    };

    addMassRow(wxString::FromUTF8("Масса поршня, кг:"), m_massPistonInput, "0.5",
               wxString::FromUTF8("Масса поршня в сборе"));
    addMassRow(wxString::FromUTF8("Масса шатуна, кг:"), m_massRodInput, "0.8",
               wxString::FromUTF8("Полная масса шатуна"));
    addMassRow(wxString::FromUTF8("Доля возвратно-поступат. массы шатуна:"), m_kRodOscInput, "0.3",
               wxString::FromUTF8("Обычно 0.3–0.4, остальное — вращающаяся часть"));
    addMassRow(wxString::FromUTF8("Диаметр цилиндра, м:"), m_dynBoreInput, "0.08",
           wxString::FromUTF8("Диаметр цилиндра (для расчёта площади поршня)"));

    massBox->Add(massGrid, 0, wxEXPAND | wxALL, 5);
    vbox->Add(massBox, 0, wxEXPAND | wxALL, 10);

    // Группа "Давление газов"
    auto* pressureBox = new wxStaticBoxSizer(wxVERTICAL, parent, wxString::FromUTF8("Давление газов"));
    ApplyDarkTheme(pressureBox->GetStaticBox());

    auto* pressureGrid = new wxFlexGridSizer(0, 3, 8, 12);
    pressureGrid->AddGrowableCol(1, 1);


    pressureGrid->Add(new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Единицы измерения:")),
                      0, wxALIGN_CENTER_VERTICAL);

    wxArrayString unitNames;
    unitNames.Add(wxString::FromUTF8("бар"));
    unitNames.Add(wxString::FromUTF8("МПа"));
    unitNames.Add(wxString::FromUTF8("кПа"));
    unitNames.Add(wxString::FromUTF8("PSI"));
    m_dynUnitChoice = new wxChoice(parent, ID_DynUnitChoice, wxDefaultPosition, wxSize(100, -1), unitNames);
    m_dynUnitChoice->SetSelection(0);
    ApplyDarkTheme(m_dynUnitChoice);
    pressureGrid->Add(m_dynUnitChoice, 0, wxALIGN_CENTER_VERTICAL);
    pressureGrid->AddStretchSpacer();

    pressureGrid->Add(new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Файл:")),
                      0, wxALIGN_CENTER_VERTICAL);

    m_dynFileLabel = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("не выбран"));
    ApplyDarkTheme(m_dynFileLabel);
    pressureGrid->Add(m_dynFileLabel, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

    m_dynLoadBtn = new wxButton(parent, ID_DynLoadBtn, wxString::FromUTF8("Обзор..."));
    ApplyDarkTheme(m_dynLoadBtn);
    pressureGrid->Add(m_dynLoadBtn, 0, wxALIGN_CENTER_VERTICAL);

    pressureBox->Add(pressureGrid, 0, wxEXPAND | wxALL, 5);
    // Статус загрузки (вместо всплывающих окон)
    m_dynLoadStatus = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Файл не загружен."));
    m_dynLoadStatus->SetForegroundColour(wxColour(0xB0, 0xB0, 0xB0));
    ApplyDarkTheme(m_dynLoadStatus);
    pressureBox->Add(m_dynLoadStatus, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // Предварительный просмотр
    auto* previewLabel = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Предварительный просмотр:"));
    ApplyDarkTheme(previewLabel);
    pressureBox->Add(previewLabel, 0, wxTOP | wxLEFT | wxRIGHT, 10);

    m_dynPreviewPlot = new KinematicPlotPanel(parent);
m_dynPreviewPlot->SetMinSize(wxSize(-1, 220));   // фикс по высоте
ApplyDarkTheme(m_dynPreviewPlot);
pressureBox->Add(m_dynPreviewPlot, 0, wxEXPAND | wxALL, 10);  // <-- было 1, стало 0

    vbox->Add(pressureBox, 0, wxEXPAND | wxALL, 10);

    // Кнопка "Дальше"
m_dynNextBtn = new wxButton(parent, ID_DynNextBtn, wxString::FromUTF8("Дальше →"));
ApplyDarkTheme(m_dynNextBtn);
m_dynNextBtn->Enable(false); // включим после загрузки диаграммы и кинематики
vbox->Add(m_dynNextBtn, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 20);

parent->SetSizer(vbox);
    parent->Layout();

}

void MainFrame::BuildDynamicIgnitionPage(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* title = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("УГЛЫ ВСПЫШЕК"));
    title->SetForegroundColour(*wxWHITE);
    title->SetFont(wxFontInfo(16).Bold());
    vbox->Add(title, 0, wxALL, 10);
    vbox->Add(new wxStaticLine(parent), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    auto* ignitionBox = new wxStaticBoxSizer(wxVERTICAL, parent, wxString::FromUTF8("Углы рабочего такта (вспышки)"));
    ApplyDarkTheme(ignitionBox->GetStaticBox());

    // Допуск ВМТ
    {
        auto* tolSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* tolLbl = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Допуск ВМТ, град:"));
        ApplyDarkTheme(tolLbl);

        m_tdcTolInput = new wxTextCtrl(parent, wxID_ANY, wxString::FromUTF8("1.0"));
        ApplyDarkTheme(m_tdcTolInput);

        tolSizer->Add(tolLbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
        tolSizer->Add(m_tdcTolInput, 0, wxALIGN_CENTER_VERTICAL);

        ignitionBox->Add(tolSizer, 0, wxEXPAND | wxALL, 8);
    }

    // Таблица углов
    m_ignitionPanelHost = new LayoutPanel(parent);
    ApplyDarkTheme(m_ignitionPanelHost);
    m_ignitionPanelHost->SetMinSize(wxSize(-1, 320));
    ignitionBox->Add(m_ignitionPanelHost, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    vbox->Add(ignitionBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    auto* ip = static_cast<LayoutPanel*>(m_ignitionPanelHost);
ip->LockDimensions(true);
ip->SetTableOnly(true);

    // Статус
    m_dynCalcStatus = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8(" "));
    m_dynCalcStatus->SetForegroundColour(wxColour(0xB0, 0xB0, 0xB0));
    ApplyDarkTheme(m_dynCalcStatus);
    vbox->Add(m_dynCalcStatus, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // Нижние кнопки: Назад + Рассчитать
    auto* bottom = new wxBoxSizer(wxHORIZONTAL);

    m_dynPrevBtn = new wxButton(parent, ID_DynPrevBtn, wxString::FromUTF8("← Назад"));
    ApplyDarkTheme(m_dynPrevBtn);
    bottom->Add(m_dynPrevBtn, 0, wxRIGHT, 10);

    m_dynCalcBtn = new wxButton(parent, ID_DynCalcBtn, wxString::FromUTF8("РАССЧИТАТЬ ДИНАМИКУ"));
    ApplyDarkTheme(m_dynCalcBtn);
    m_dynCalcBtn->Enable(false);
    bottom->Add(m_dynCalcBtn, 0);

    vbox->Add(bottom, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 20);

    parent->SetSizer(vbox);
}

void MainFrame::BuildDynamicResultPage(wxPanel* parent)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    // Книжка вкладок
    m_dynResultBook = new wxNotebook(parent, wxID_ANY);
    ApplyDarkTheme(m_dynResultBook);
    m_dynResultBook->SetBackgroundColour(wxColour(0x25, 0x2A, 0x30));

    // =======================
    // Вкладка "График"
    // =======================
    m_dynGraphPage = new wxPanel(m_dynResultBook);
    ApplyDarkTheme(m_dynGraphPage);

    auto* graphPageSizer = new wxBoxSizer(wxHORIZONTAL);

    // ---- Левая панель управления (как в кинематике)
    auto* leftCtrl = new wxPanel(m_dynGraphPage);
    ApplyDarkTheme(leftCtrl);
    leftCtrl->SetMinSize(wxSize(240, -1));

    auto* leftSizer = new wxBoxSizer(wxVERTICAL);

    // Тип графика
    auto* typeLbl = new wxStaticText(leftCtrl, wxID_ANY, wxString::FromUTF8("Показатель:"));
    ApplyDarkTheme(typeLbl);
    leftSizer->Add(typeLbl, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    wxArrayString graphTypes;
    graphTypes.Add(wxString::FromUTF8("Суммарный момент двигателя"));
    graphTypes.Add(wxString::FromUTF8("Сила газов"));
    graphTypes.Add(wxString::FromUTF8("Сила инерции"));
    graphTypes.Add(wxString::FromUTF8("Суммарная сила"));
    graphTypes.Add(wxString::FromUTF8("Тангенциальная сила"));
    graphTypes.Add(wxString::FromUTF8("Радиальная сила"));
    graphTypes.Add(wxString::FromUTF8("Момент цилиндра"));

    m_dynGraphTypeChoice = new wxChoice(leftCtrl, ID_DynGraphTypeChoice,
                                        wxDefaultPosition, wxSize(220, -1), graphTypes);
    m_dynGraphTypeChoice->SetSelection(0);
    ApplyDarkTheme(m_dynGraphTypeChoice);
    leftSizer->Add(m_dynGraphTypeChoice, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    // Цилиндры
    auto* cylLbl = new wxStaticText(leftCtrl, wxID_ANY, wxString::FromUTF8("Цилиндры:"));
    ApplyDarkTheme(cylLbl);
    leftSizer->Add(cylLbl, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    m_dynCylinderCheckList = new wxCheckListBox(leftCtrl, ID_DynCylinderCheckList,
                                               wxDefaultPosition, wxSize(220, 180));
    ApplyDarkTheme(m_dynCylinderCheckList);
    leftSizer->Add(m_dynCylinderCheckList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);

    // Выбрать все / Очистить
    auto* cylBtnSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* selectAllCylBtn = new wxButton(leftCtrl, ID_DynSelectAllCylBtn, wxString::FromUTF8("Выбрать все"),
                                     wxDefaultPosition, wxSize(106, 30));
auto* clearAllCylBtn  = new wxButton(leftCtrl, ID_DynClearAllCylBtn,  wxString::FromUTF8("Очистить все"),
                                     wxDefaultPosition, wxSize(106, 30));

    ApplyDarkTheme(selectAllCylBtn);
    ApplyDarkTheme(clearAllCylBtn);

    cylBtnSizer->Add(selectAllCylBtn, 1, wxRIGHT, 8);
    cylBtnSizer->Add(clearAllCylBtn,  1);

    leftSizer->Add(cylBtnSizer, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    // Подсказка пользователю
    auto* hint = new wxStaticText(leftCtrl, wxID_ANY,
                                  wxString::FromUTF8("Для суммарного момента\nвыбор цилиндров не нужен."));
    hint->SetForegroundColour(wxColour(0xB0, 0xB0, 0xB0));
    leftSizer->Add(hint, 0, wxLEFT | wxRIGHT | wxTOP, 10);

    leftSizer->AddStretchSpacer();
    leftCtrl->SetSizer(leftSizer);

    // ---- Правая часть: график
    m_dynPlotResult = new KinematicPlotPanel(m_dynGraphPage);
    ApplyDarkTheme(m_dynPlotResult);

    graphPageSizer->Add(leftCtrl, 0, wxEXPAND | wxALL, 10);
    graphPageSizer->Add(m_dynPlotResult, 1, wxEXPAND | wxALL, 10);

    m_dynGraphPage->SetSizer(graphPageSizer);
    m_dynResultBook->AddPage(m_dynGraphPage, wxString::FromUTF8("График"), true);

    // =======================
    // Вкладка "Таблица"
    // =======================
    m_dynTablePage = new wxPanel(m_dynResultBook);
    ApplyDarkTheme(m_dynTablePage);

    auto* tableSizer = new wxBoxSizer(wxVERTICAL);

    m_dynGrid = new wxGrid(m_dynTablePage, wxID_ANY);
    m_dynGrid->CreateGrid(0, 8);

    m_dynGrid->SetColLabelValue(0, wxString::FromUTF8("α, град"));
    m_dynGrid->SetColLabelValue(1, wxString::FromUTF8("Сила газов, Н"));
    m_dynGrid->SetColLabelValue(2, wxString::FromUTF8("Сила инерции, Н"));
    m_dynGrid->SetColLabelValue(3, wxString::FromUTF8("Суммарная сила, Н"));
    m_dynGrid->SetColLabelValue(4, wxString::FromUTF8("Тангенц. сила, Н"));
    m_dynGrid->SetColLabelValue(5, wxString::FromUTF8("Радиальная сила, Н"));
    m_dynGrid->SetColLabelValue(6, wxString::FromUTF8("Момент цил., Н·м"));
    m_dynGrid->SetColLabelValue(7, wxString::FromUTF8("Сумм. момент, Н·м"));

    m_dynGrid->SetBackgroundColour(wxColour(0x25, 0x2A, 0x30));
    m_dynGrid->SetLabelBackgroundColour(wxColour(0x18, 0x1C, 0x22));
    m_dynGrid->SetLabelTextColour(wxColour(0xE0, 0xE0, 0xE0));
    m_dynGrid->SetDefaultCellBackgroundColour(wxColour(0x25, 0x2A, 0x30));
    m_dynGrid->SetDefaultCellTextColour(wxColour(0xF0, 0xF0, 0xF0));
    m_dynGrid->EnableEditing(false);
    m_dynGrid->AutoSizeColumns();

    tableSizer->Add(m_dynGrid, 1, wxEXPAND | wxALL, 10);
    m_dynTablePage->SetSizer(tableSizer);
    m_dynResultBook->AddPage(m_dynTablePage, wxString::FromUTF8("Таблица"), false);

    // ---- Собираем страницу
    sizer->Add(m_dynResultBook, 1, wxEXPAND | wxALL, 5);

    // Нижняя панель: Назад + Экспорт
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* dynBackBtn = new wxButton(parent, ID_DynBackBtn, wxString::FromUTF8("← Назад"));
    ApplyDarkTheme(dynBackBtn);
    bottomSizer->Add(dynBackBtn, 0, wxRIGHT, 10);

    m_exportDynBtn = new wxButton(parent, ID_ExportDynamicBtn, wxString::FromUTF8("Экспорт ▾"));
    ApplyDarkTheme(m_exportDynBtn);
    bottomSizer->Add(m_exportDynBtn, 0);

    sizer->Add(bottomSizer, 0, wxALIGN_LEFT | wxALL, 10);

    parent->SetSizer(sizer);

    // Сразу синхронизируем доступность цилиндров под выбранный показатель
    UpdateDynamicControlsState();
}

void MainFrame::OnDynGraphTypeChanged(wxCommandEvent&)
{
    UpdateDynamicControlsState();
    UpdateDynamicGraph();
}


void MainFrame::OnLoadPressure(wxCommandEvent&)
{
    wxFileDialog dlg(this, wxString::FromUTF8("Выберите файл с индикаторной диаграммой"),
                     "", "", "CSV files (*.csv)|*.csv|Text files (*.txt)|*.txt|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString path = dlg.GetPath();

    PressureUnit unit = PressureUnit::BAR;
    int sel = m_dynUnitChoice->GetSelection();
    switch (sel)
    {
    case 0: unit = PressureUnit::BAR; break;
    case 1: unit = PressureUnit::MPA; break;
    case 2: unit = PressureUnit::KPA; break;
    case 3: unit = PressureUnit::PSI; break;
    }

    std::vector<double> angles, pressures;
    wxString errorMsg;
    if (!LoadPressureFromFile(path, angles, pressures, errorMsg, unit))
    {
        wxMessageBox(errorMsg, wxString::FromUTF8("Ошибка загрузки"), wxOK | wxICON_ERROR, this);
        return;
    }

    // Если диаграмма только на 0..360, дублируем до 0..720 (удобнее для общего режима)
auto isApprox = [](double a, double b) { return std::fabs(a - b) < 1e-6; };

double aMin = angles.front();
double aMax = angles.back();

// Если файл отсортирован не гарантированно, можно заменить на поиск min/max циклом.
// Сейчас оставляем простой вариант под твой текущий парсер.
const bool looks360 = (aMax > 300.0 && aMax < 420.0);
const bool looks720 = (aMax > 660.0 && aMax < 780.0);

if (looks360 && !looks720)
{
    const size_t n = angles.size();

    std::vector<double> a2; a2.reserve(n * 2);
    std::vector<double> p2; p2.reserve(n * 2);

    // первый проход как есть
    for (size_t i = 0; i < n; ++i) {
        a2.push_back(angles[i]);
        p2.push_back(pressures[i]);
    }

    // второй проход +360, пропускаем точки, которые дадут дубль границы (например, 0 и 360)
    for (size_t i = 0; i < n; ++i) {
        const double a = angles[i] + 360.0;
        // если первая точка второго цикла совпадает с последней первой (часто 360),
        // пропускаем, чтобы не было одинаковых углов подряд
        if (i == 0 && isApprox(a, angles.back())) continue;
        a2.push_back(a);
        p2.push_back(pressures[i]);
    }

    angles.swap(a2);
    pressures.swap(p2);

    // Обновим статус, что была авто-операция
    if (m_dynLoadStatus) {
        m_dynLoadStatus->SetLabel(wxString::FromUTF8("Диаграмма 0..360° автоматически продублирована до 0..720°."));
    }
}

// Нормализация под систему программы:
    // alpha=0 в программе = ВМТ конца сжатия, а в Diesel-RK alpha=0 = начало сжатия.
    const double cycle = (m_lastParams.taktnost == 2) ? 360.0 : 720.0;
    NormalizePressureFromDieselRK(angles, pressures, cycle);

    m_dynPressureAngles = angles;
    m_dynPressureValues = pressures;
    UpdateSidebarSteps();
    m_dynPressureFile = path;

    if (angles.empty() || pressures.empty() || angles.size() != pressures.size())
{
    wxMessageBox(wxString::FromUTF8("Ошибка: данные пусты или некорректны"));
    return;
}

    m_dynFileLabel->SetLabel(wxFileName(path).GetFullName());


    m_dynPreviewPlot->SetPreviewData(angles, pressures,
    wxString::FromUTF8("Угол, град"),
    wxString::FromUTF8("Давление"));
    UpdateSidebarSteps();

    if (m_dynLoadStatus)
{
    const double aMin = angles.front();
    const double aMax = angles.back();

    wxString unitStr;
    switch (unit)
    {
    case PressureUnit::BAR: unitStr = wxString::FromUTF8("бар"); break;
    case PressureUnit::MPA: unitStr = wxString::FromUTF8("МПа"); break;
    case PressureUnit::KPA: unitStr = wxString::FromUTF8("кПа"); break;
    case PressureUnit::PSI: unitStr = wxString::FromUTF8("PSI"); break;
    default: unitStr = "?"; break;
    }

    m_dynLoadStatus->SetLabel(wxString::Format(
        wxString::FromUTF8("Загружено: %zu точек, угол: %.1f..%.1f°, единицы: %s"),
        angles.size(), aMin, aMax, unitStr
    ));
}

}


// Вспомогательные методы обновления

void MainFrame::UpdateDynamicTable()
{
    if (m_dynResults.total_torque.empty() || m_lastResults.alpha.empty())
        return;

    // Очищаем таблицу
    if (m_dynGrid->GetNumberRows() > 0)
        m_dynGrid->DeleteRows(0, m_dynGrid->GetNumberRows());

    size_t numPoints = m_lastResults.alpha.size();
    m_dynGrid->AppendRows(numPoints);

    for (size_t i = 0; i < numPoints; ++i)
    {
        m_dynGrid->SetCellValue(i, 0, wxString::Format("%.2f", m_lastResults.alpha[i]));

        // Для первого цилиндра (индекс 0)
        if (!m_dynResults.gas_force.empty() && i < m_dynResults.gas_force[0].size())
        {
            m_dynGrid->SetCellValue(i, 1, wxString::Format("%.3f", m_dynResults.gas_force[0][i]));
            m_dynGrid->SetCellValue(i, 2, wxString::Format("%.3f", m_dynResults.inertia_force[0][i]));
            m_dynGrid->SetCellValue(i, 3, wxString::Format("%.3f", m_dynResults.total_force[0][i]));
            m_dynGrid->SetCellValue(i, 4, wxString::Format("%.3f", m_dynResults.tangential_force[0][i]));
            m_dynGrid->SetCellValue(i, 5, wxString::Format("%.3f", m_dynResults.radial_force[0][i]));
            m_dynGrid->SetCellValue(i, 6, wxString::Format("%.3f", m_dynResults.cylinder_torque[0][i]));
            m_dynGrid->SetCellValue(i, 7, wxString::Format("%.3f", m_dynResults.total_torque[i]));
        }
        else
        {
            for (int col = 1; col <= 7; ++col)
                m_dynGrid->SetCellValue(i, col, "0.0");
        }
    }

    m_dynGrid->AutoSizeColumns();
}


// =====================================================================
//  Обработчики событий
// =====================================================================



void MainFrame::OnDynNext(wxCommandEvent&)
{
    if (!m_dynamicBook) return;
    m_dynamicBook->SetSelection(1); // ignition page
}

void MainFrame::OnDynPrev(wxCommandEvent&)
{
    if (!m_dynamicBook) return;
    m_dynamicBook->SetSelection(0); // pressure page
}



void MainFrame::OnGoToDynamic(wxCommandEvent&)
{
    // Переходим на шаг 2 (ввод динамики/загрузка файла)
    m_rightBook->SetSelection(1);
    if (m_dynamicBook) m_dynamicBook->SetSelection(0);
}

static bool IsPressureOk(const std::vector<double>& a, const std::vector<double>& p)
{
    return !a.empty() && (a.size() == p.size());
}




void MainFrame::OnSessionMenu(wxCommandEvent&)
{
    wxMenu menu;
    menu.Append(ID_MenuSessionSave, wxString::FromUTF8("Сохранить сессию..."));
    menu.Append(ID_MenuSessionLoad, wxString::FromUTF8("Загрузить сессию..."));

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnSaveSession(dummy);
    }, ID_MenuSessionSave);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnLoadSession(dummy);
    }, ID_MenuSessionLoad);

    if (m_sessionMenuBtn)
        m_sessionMenuBtn->PopupMenu(&menu);
}

void MainFrame::OnExportKinematicMenu(wxCommandEvent&)
{
    wxMenu menu;
    menu.Append(ID_MenuKinExportCSV, wxString::FromUTF8("Сохранить результаты в CSV..."));
    menu.Append(ID_MenuKinExportTXT, wxString::FromUTF8("Сохранить результаты в TXT..."));
    menu.AppendSeparator();
    menu.Append(ID_MenuKinExportPNG, wxString::FromUTF8("Сохранить график как PNG..."));

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnSaveCsv(dummy);
    }, ID_MenuKinExportCSV);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnSaveTxt(dummy);
    }, ID_MenuKinExportTXT);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnSaveGraphPNG(dummy);
    }, ID_MenuKinExportPNG);

    if (m_exportKinBtn)
        m_exportKinBtn->PopupMenu(&menu);
}

void MainFrame::OnExportDynamicMenu(wxCommandEvent&)
{
    wxMenu menu;
    menu.Append(ID_MenuDynExportCSVWide, wxString::FromUTF8("Сохранить CSV (таблица)..."));
    menu.Append(ID_MenuDynExportCSVLong, wxString::FromUTF8("Сохранить CSV (инженерный)..."));
    menu.Append(ID_MenuDynExportTXT,     wxString::FromUTF8("Сохранить TXT..."));

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnDynSaveCsvWide(dummy);
    }, ID_MenuDynExportCSVWide);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnDynSaveCsvLong(dummy);
    }, ID_MenuDynExportCSVLong);

    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        wxCommandEvent dummy;
        OnDynSaveTxt(dummy);
    }, ID_MenuDynExportTXT);

    if (m_exportDynBtn)
        m_exportDynBtn->PopupMenu(&menu);
}



void MainFrame::UpdateDynamicControlsState()
{
    if (!m_dynGraphTypeChoice || !m_dynCylinderCheckList) return;

    int selType = m_dynGraphTypeChoice->GetSelection();
    if (selType == wxNOT_FOUND) selType = 0;

    bool needCyl = (selType != 0); // 0 = суммарный момент (не требует выбора цилиндров)
    m_dynCylinderCheckList->Enable(needCyl);

    // Найти кнопки "Выбрать все" и "Очистить всё" по ID и установить их enabled
    wxWindow* btnAll = FindWindow(ID_DynSelectAllCylBtn);
    wxWindow* btnClr = FindWindow(ID_DynClearAllCylBtn);
    if (btnAll) btnAll->Enable(needCyl);
    if (btnClr) btnClr->Enable(needCyl);
}


void MainFrame::OnSaveGraphPNG(wxCommandEvent&) {
    if (!m_hasResults) {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт."),
                     wxString::FromUTF8("Нет данных"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog dlg(this, wxString::FromUTF8("Сохранить график как PNG"),
                     "", "graph.png",
                     "PNG files (*.png)|*.png",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString path = dlg.GetPath();
    if (!m_plotPanel->SaveAsPNG(path)) {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить изображение."),
                     wxString::FromUTF8("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    } else {
        wxMessageBox(wxString::FromUTF8("График сохранён."),
                     wxString::FromUTF8("Успех"),
                     wxOK | wxICON_INFORMATION, this);
    }
}


void MainFrame::OnGraphTypeChanged(wxCommandEvent&)
{
    if (!m_plotPanel || !m_graphTypeChoice) return;

    const int sel = m_graphTypeChoice->GetSelection();
    switch (sel) {
    case 0: m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementMain); break;
    case 1: m_plotPanel->SetMode(KinematicPlotPanel::Mode::VelocityMain); break;
    case 2: m_plotPanel->SetMode(KinematicPlotPanel::Mode::AccelerationMain); break;
    default: m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementMain); break;
    }
    m_plotPanel->Refresh();
}

void MainFrame::OnCylinderCheckListChanged(wxCommandEvent&)
{
    if (!m_cylinderCheckList || !m_plotPanel) return;

    std::vector<int> selected;
    const unsigned count = m_cylinderCheckList->GetCount();
    for (unsigned i = 0; i < count; ++i)
        if (m_cylinderCheckList->IsChecked(i))
            selected.push_back((int)i);

    m_plotPanel->SetSelectedIndices(selected);
    m_plotPanel->Refresh();
}


void MainFrame::OnSelectAll(wxCommandEvent& evt)
{
    int count = m_cylinderCheckList->GetCount();
    std::vector<int> selected;
    for (int i = 0; i < count; ++i) {
        m_cylinderCheckList->Check(i);
        selected.push_back(i);
    }
    m_plotPanel->SetSelectedIndices(selected);
    m_plotPanel->Refresh();
}

void MainFrame::OnClearAll(wxCommandEvent& evt)
{
    int count = m_cylinderCheckList->GetCount();
    for (int i = 0; i < count; ++i)
        m_cylinderCheckList->Check(i, false);
    
    std::vector<int> selected; // пустой
    m_plotPanel->SetSelectedIndices(selected);
    m_plotPanel->Refresh();
}

void MainFrame::OnBackToInput(wxCommandEvent&)
{
    // Просто возвращаем на ввод параметров.
    // Экспорт делается через кнопку "Экспорт ▾" и не должен мешать редактированию.
    m_kinematicBook->SetSelection(0);

}

void MainFrame::OnSaveCsv(wxCommandEvent&)
{
    if (!m_hasResults) {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт."), wxString::FromUTF8("Нет данных"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog dlg(this, wxString::FromUTF8("Сохранить"), "", "ksm_results", "CSV|*.csv",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    wxDateTime now = wxDateTime::Now();
    wxString timestamp = now.Format("%H.%M.%S__%d.%m.%Y");
    dlg.SetFilename("ksm_results_" + timestamp + ".csv");

    if (dlg.ShowModal() != wxID_OK) return;

    std::string filename = dlg.GetPath().ToUTF8().data();
    bool ok = KinematicOutput::saveToCSV(m_lastResults, m_lastParams, filename);
    if (ok)
        wxMessageBox(wxString::FromUTF8("Файл успешно сохранён."), wxString::FromUTF8("Сохранение завершено"),
                     wxOK | wxICON_INFORMATION, this);
    else
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить файл."), wxString::FromUTF8("Ошибка сохранения"),
                     wxOK | wxICON_ERROR, this);
}

void MainFrame::OnSaveTxt(wxCommandEvent&)
{
    if (!m_hasResults) {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт."), wxString::FromUTF8("Нет данных"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog dlg(this, wxString::FromUTF8("Сохранить"), "", "ksm_results", "TXT|*.txt",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    wxDateTime now = wxDateTime::Now();
    wxString timestamp = now.Format("%H.%M.%S__%d.%m.%Y");
    dlg.SetFilename("ksm_results_" + timestamp + ".txt");

    if (dlg.ShowModal() != wxID_OK) return;

    std::string filename = dlg.GetPath().ToUTF8().data();
    bool ok = KinematicOutput::saveToFormattedText(m_lastResults, m_lastParams, filename);
    if (ok)
        wxMessageBox(wxString::FromUTF8("Файл успешно сохранён."), wxString::FromUTF8("Сохранение завершено"),
                     wxOK | wxICON_INFORMATION, this);
    else
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить файл."), wxString::FromUTF8("Ошибка сохранения"),
                     wxOK | wxICON_ERROR, this);
}

bool MainFrame::ValidateField(wxTextCtrl* field, double minVal, double maxVal, 
                              bool allowZero, bool positiveOnly)
{
    if (!field) return false;
    
    // Если поле скрыто, считаем его валидным (не влияет на общую проверку)
    if (!field->IsShown()) {
        field->SetBackgroundColour(wxNullColour); // сброс цвета
        field->Refresh();
        return true;
    }
    
    wxString valStr = field->GetValue();
    double value;
    bool ok = valStr.ToDouble(&value);
    
    if (!ok) {
        field->SetBackgroundColour(wxColour(0x80, 0x00, 0x00)); // тёмно-красный
        field->Refresh();
        return false;
    }
    
    // Проверка на положительность, если требуется
    if (positiveOnly && value <= 0) {
        field->SetBackgroundColour(wxColour(0x80, 0x00, 0x00));
        field->Refresh();
        return false;
    }
    
    // Проверка диапазона
    if (value < minVal || value > maxVal) {
        field->SetBackgroundColour(wxColour(0x80, 0x00, 0x00));
        field->Refresh();
        return false;
    }
    
    // Всё хорошо – сбрасываем цвет на стандартный для темы
    field->SetBackgroundColour(wxColour(0x18, 0x1C, 0x22)); // цвет темы
    field->Refresh();
    return true;
}

void MainFrame::UpdateCalculateButtonState()
{
    bool allValid = true;

    if (!m_crankPanel) {
        allValid = false;
    } else {
        CrankConfig cfg;
        wxString err;
        allValid = m_crankPanel->GetConfig(cfg, err);
    }

    if (m_calcButton)
        m_calcButton->Enable(allValid);
}


bool MainFrame::SaveSessionToFile(const wxString& filename)
{
    wxFileOutputStream out(filename);
    if (!out.IsOk()) return false;
    wxTextOutputStream txt(out);

    // Записываем сигнатуру
    txt << "# Engine Design Session File\n";

    // Записываем параметры
    txt << "[Parameters]\n";
    txt << "countCyl=" << m_lastParams.countCyl << "\n";
    txt << "taktnost=" << m_lastParams.taktnost << "\n";
    txt << "step_alpha=" << m_lastParams.step_alpha << "\n";
    txt << "end_alpha=" << m_lastParams.end_alpha << "\n";
    txt << "radcrank=" << m_lastParams.radcrank << "\n";
    txt << "lyambda=" << m_lastParams.lyambda << "\n";
    txt << "n=" << m_lastParams.n << "\n";
    txt << "gamma=" << m_lastParams.gamma << "\n";
    txt << "dezaxial=" << m_lastParams.dezaxial << "\n";
    txt << "gammaPric=" << m_lastParams.gammaPric << "\n";
    txt << "radcrank1=" << m_lastParams.radcrank1 << "\n";
    txt << "lengthRod1=" << m_lastParams.lengthRod1 << "\n";

    // --- Компоновка / фазы цилиндров ---
    txt << "layout_rows=" << m_lastParams.layout_rows << "\n";
    txt << "layout_sections=" << m_lastParams.layout_sections << "\n";

    // Сохраняем фазы как CSV: "0,180,360,..."
    txt << "cyl_phase_deg=";
    for (size_t i = 0; i < m_lastParams.cyl_phase_deg.size(); ++i)
    {
        if (i > 0) txt << ",";
        txt << wxString::Format("%.6f", m_lastParams.cyl_phase_deg[i]);
    }
    txt << "\n";

    // Если есть результаты, записываем их
    if (m_hasResults)
{
    txt << "[Results]\n";
    txt << "alpha,stroke_full,velocity_full,acceleration_full,betta_rod,omega_rod,eps_rod\n";
    for (size_t i = 0; i < m_lastResults.alpha.size(); ++i)
    {
        txt << m_lastResults.alpha[i] << ","
            << (m_lastResults.cylinder_stroke_full.empty() ? 0.0 : m_lastResults.cylinder_stroke_full[0][i]) << ","
            << (m_lastResults.cylinder_velocity_full.empty() ? 0.0 : m_lastResults.cylinder_velocity_full[0][i]) << ","
            << (m_lastResults.cylinder_acceleration_full.empty() ? 0.0 : m_lastResults.cylinder_acceleration_full[0][i]) << ","
            << (m_lastResults.cylinder_betta_rod.empty() ? 0.0 : m_lastResults.cylinder_betta_rod[0][i]) << ","
            << (m_lastResults.cylinder_omega_rod.empty() ? 0.0 : m_lastResults.cylinder_omega_rod[0][i]) << ","
            << (m_lastResults.cylinder_eps_rod.empty() ? 0.0 : m_lastResults.cylinder_eps_rod[0][i]) << "\n";
    }
}

    // Сохраняем состояние GUI (выбранные индексы и т.д.)
    txt << "[GUI]\n";
    txt << "graphMode=" << static_cast<int>(m_plotPanel->GetMode()) << "\n";
    // Сохраняем выбранные индексы (список через запятую)
    txt << "selectedIndices=";
    std::vector<int> selected = m_plotPanel->GetSelectedIndices(); // нужно добавить метод в KinematicPlotPanel
    for (size_t i = 0; i < selected.size(); ++i)
    {
        if (i > 0) txt << ",";
        txt << wxString::Format("%d", selected[i]);
    }
    txt << "\n";

    return true;
}

bool MainFrame::LoadSessionFromFile(const wxString& filename)
{
    wxFileInputStream in(filename);
    if (!in.IsOk()) return false;
    wxTextInputStream txt(in);

    wxString line;
    EngineParams params;
    bool readingParams = false;
    bool readingResults = false;
    bool readingGUI = false;
    // Временное хранилище фаз, прочитанных из файла
    std::vector<double> loadedPhases;

    // Временные векторы для результатов
    std::vector<double> alpha, stroke, vel, acc, betta, omega, eps;

    while (!in.Eof())
    {
        line = txt.ReadLine();
        if (line.empty()) continue;

        if (line == "[Parameters]")
        {
            readingParams = true;
            readingResults = false;
            readingGUI = false;
            continue;
        }
        else if (line == "[Results]")
        {
            readingParams = false;
            readingResults = true;
            readingGUI = false;
            // Пропускаем заголовок
            line = txt.ReadLine();
            continue;
        }
        else if (line == "[GUI]")
        {
            readingParams = false;
            readingResults = false;
            readingGUI = true;
            continue;
        }

        if (readingParams)
        {
            wxString key, value;
            if (line.Contains("="))
            {
                key = line.BeforeFirst('=');
                value = line.AfterFirst('=');
                double dval;
                long lval;
                if (key == "countCyl" && value.ToLong(&lval)) params.countCyl = lval;
                else if (key == "taktnost" && value.ToLong(&lval)) params.taktnost = lval;
                else if (key == "step_alpha" && value.ToDouble(&dval)) params.step_alpha = dval;
                else if (key == "end_alpha" && value.ToDouble(&dval)) params.end_alpha = dval;
                else if (key == "radcrank" && value.ToDouble(&dval)) params.radcrank = dval;
                else if (key == "lyambda" && value.ToDouble(&dval)) params.lyambda = dval;
                else if (key == "n" && value.ToDouble(&dval)) params.n = dval;
                else if (key == "gamma" && value.ToDouble(&dval)) params.gamma = dval;
                else if (key == "dezaxial" && value.ToDouble(&dval)) params.dezaxial = dval;
                else if (key == "gammaPric" && value.ToDouble(&dval)) params.gammaPric = dval;
                else if (key == "radcrank1" && value.ToDouble(&dval)) params.radcrank1 = dval;
                else if (key == "lengthRod1" && value.ToDouble(&dval)) params.lengthRod1 = dval;
                else if (key == "layout_rows" && value.ToLong(&lval)) params.layout_rows = (int)lval;
                else if (key == "layout_sections" && value.ToLong(&lval)) params.layout_sections = (int)lval;
                else if (key == "cyl_phase_deg")
                {
                    if (!ParseDoubleList(value, loadedPhases))
                    {
                        // если фазы битые — просто не применяем, уйдём в fallback
                        loadedPhases.clear();
                    }
                }
            }
        }
        else if (readingResults)
        {
            // Читаем CSV: alpha,stroke,vel,acc,betta,omega,eps
            wxStringTokenizer tokenizer(line, ",");
            if (tokenizer.CountTokens() >= 7)
            {
                alpha.push_back(wxAtof(tokenizer.GetNextToken()));
                stroke.push_back(wxAtof(tokenizer.GetNextToken()));
                vel.push_back(wxAtof(tokenizer.GetNextToken()));
                acc.push_back(wxAtof(tokenizer.GetNextToken()));
                betta.push_back(wxAtof(tokenizer.GetNextToken()));
                omega.push_back(wxAtof(tokenizer.GetNextToken()));
                eps.push_back(wxAtof(tokenizer.GetNextToken()));
            }
        }
        else if (readingGUI)
        {
            if (line.Contains("="))
            {
                wxString key = line.BeforeFirst('=');
                wxString value = line.AfterFirst('=');
                if (key == "graphMode")
                    {
                        long mode;
                            if (value.ToLong(&mode))
                        {
                            // теперь доступны только 0..2 (перемещение/скорость/ускорение)
                            if (mode < 0) mode = 0;
                            if (mode > 2) mode = 2;

                            if (m_graphTypeChoice)
                            m_graphTypeChoice->SetSelection((int)mode);
                        }
                    }
                else if (key == "selectedIndices")
                {
                    // Парсим список через запятую
                    // Будет обработано позже, когда загрузим данные
                }
            }
        }
    }

    // Применяем фазы цилиндров, если они были в файле
    if (!loadedPhases.empty())
    {
        params.cyl_phase_deg = loadedPhases;
    }

    // После чтения, если есть результаты, восстанавливаем их
    if (!alpha.empty())
{
    CalculationResults res;
    res.alpha = alpha;
    // Заполняем векторы для первого цилиндра
    res.cylinder_stroke_full.push_back(stroke);
    res.cylinder_velocity_full.push_back(vel);
    res.cylinder_acceleration_full.push_back(acc);
    res.cylinder_betta_rod.push_back(betta);
    res.cylinder_omega_rod.push_back(omega);
    res.cylinder_eps_rod.push_back(eps);
    // При необходимости можно добавить боковые цилиндры, но в текущем формате сессии их нет

    m_lastResults = std::move(res);
    m_lastParams = params;
    m_hasResults = true;

    // Восстанавливаем компоновку на UI (таблица фаз)
    if (m_layoutPanelHost)
    {
        auto* lp = static_cast<LayoutPanel*>(m_layoutPanelHost);

        // сначала подстроим под число цилиндров/тактность
        lp->Configure((int)m_lastParams.countCyl, (int)m_lastParams.taktnost);

        // затем выставим rows/sections, если они корректны
        if (m_lastParams.layout_rows > 0 && m_lastParams.layout_sections > 0 &&
            m_lastParams.layout_rows * m_lastParams.layout_sections == (int)m_lastParams.countCyl)
        {
            lp->SetDims(m_lastParams.layout_rows, m_lastParams.layout_sections);
        }

        // и загрузим фазы, если они есть и размер совпадает
        if ((int)m_lastParams.cyl_phase_deg.size() == (int)m_lastParams.countCyl)
        {
            lp->SetInitialPhases(m_lastParams.cyl_phase_deg);
        }
    }

    // Обновляем GUI (остаётся без изменений)
    UpdateCylinderCheckList();
    m_plotPanel->SetData(&m_lastResults);
    m_plotPanel->SetParams(&m_lastParams);
    m_plotPanel->SetMode(static_cast<KinematicPlotPanel::Mode>(m_graphTypeChoice->GetSelection()));

    m_kinematicBook->SetSelection(1);
}

if (!loadedPhases.empty() && (int)loadedPhases.size() != (int)params.countCyl)
    {
        wxLogWarning("Session: cyl_phase_deg size != countCyl, phases ignored.");
    }

    return true;
}

void MainFrame::OnSaveSession(wxCommandEvent&)
{
    if (!m_hasResults)
    {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт."),
                     wxString::FromUTF8("Нет данных"), wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxFileDialog dlg(this, wxString::FromUTF8("Сохранить сессию"), "", "session",
                     "Session files (*.sess)|*.sess", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    if (!SaveSessionToFile(dlg.GetPath()))
    {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить сессию."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
    else
    {
        wxMessageBox(wxString::FromUTF8("Сессия сохранена."),
                     wxString::FromUTF8("Успех"), wxOK | wxICON_INFORMATION, this);
    }
}

void MainFrame::OnLoadSession(wxCommandEvent&)
{
    wxFileDialog dlg(this, wxString::FromUTF8("Загрузить сессию"), "", "",
                     "Session files (*.sess)|*.sess", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    if (!LoadSessionFromFile(dlg.GetPath()))
    {
        wxMessageBox(wxString::FromUTF8("Не удалось загрузить сессию."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnDynamicBackToInput(wxCommandEvent&)
{
    if (!m_dynamicBook) return;
    m_dynamicBook->SetSelection(1); // страница "Углы вспышек"
}


void MainFrame::UpdateDynamicGraph()
{
    if (m_dynResults.total_torque.empty() || m_lastResults.alpha.empty()) return;

    std::vector<int> selected;
if (m_dynCylinderCheckList)
{
    for (unsigned i = 0; i < m_dynCylinderCheckList->GetCount(); ++i)
        if (m_dynCylinderCheckList->IsChecked(i))
            selected.push_back((int)i);
}

if (selected.empty())
{
    if (m_dynPlotResult)
    {
        m_dynPlotResult->Clear();
        m_dynPlotResult->Refresh();
    }
    return;
}

    int selType = m_dynGraphTypeChoice->GetSelection();
    if (selType == wxNOT_FOUND) selType = 0;

    // Суммарный момент (тип 0)
    if (selType == 0)
    {
        std::vector<std::vector<double>> data;
        data.push_back(m_dynResults.total_torque);
        std::vector<wxString> labels;
        labels.push_back(wxString::FromUTF8("Суммарный момент"));
        m_dynPlotResult->SetMultipleCurvesData(m_lastResults.alpha, data, labels,
                                               wxString::FromUTF8("Угол, град"),
                                               wxString::FromUTF8("Крутящий момент, Н·м"));
        return;
    }

    // Данные для выбранных цилиндров
    std::vector<std::vector<double>> data;
    std::vector<wxString> labels;
    int numCyl = (int)m_dynResults.gas_force.size();

    for (int i = 0; i < numCyl; ++i)
    {
        if (m_dynCylinderCheckList && m_dynCylinderCheckList->IsChecked(i))
        {
            switch (selType)
            {
            case 1: data.push_back(m_dynResults.gas_force[i]); break;
            case 2: data.push_back(m_dynResults.inertia_force[i]); break;
            case 3: data.push_back(m_dynResults.total_force[i]); break;
            case 4: data.push_back(m_dynResults.tangential_force[i]); break;
            case 5: data.push_back(m_dynResults.radial_force[i]); break;
            case 6: data.push_back(m_dynResults.cylinder_torque[i]); break;
            default: return;
            }
            labels.push_back(wxString::Format(wxString::FromUTF8("Цил.%d"), i+1));
        }
    }

    if (data.empty()) return; // ничего не выбрано – оставляем график как есть

    wxString yLabel;
    switch (selType)
    {
    case 1: yLabel = wxString::FromUTF8("Сила газов, Н"); break;
    case 2: yLabel = wxString::FromUTF8("Сила инерции, Н"); break;
    case 3: yLabel = wxString::FromUTF8("Суммарная сила, Н"); break;
    case 4: yLabel = wxString::FromUTF8("Тангенциальная сила, Н"); break;
    case 5: yLabel = wxString::FromUTF8("Радиальная сила, Н"); break;
    case 6: yLabel = wxString::FromUTF8("Момент цилиндра, Н·м"); break;
    default: yLabel = wxString::FromUTF8("Y"); break;
    }

    m_dynPlotResult->SetMultipleCurvesData(m_lastResults.alpha, data, labels,
                                           wxString::FromUTF8("Угол, град"), yLabel);
}



static bool ReadDoubleFromText(wxTextCtrl* ctrl, double& out)
{
    if (!ctrl) return false;
    wxString s = ctrl->GetValue();
    s.Replace(",", "."); // на всякий случай под русскую локаль
    return s.ToDouble(&out);
}

DynamicExportInfo MainFrame::BuildDynExportInfo() const
{
    DynamicExportInfo info;

    info.pressure_file = std::string(m_dynPressureFile.ToUTF8().data());

    

    auto readDouble = [](wxTextCtrl* ctrl, double& out)
    {
        if (!ctrl) return;
        wxString s = ctrl->GetValue();
        s.Replace(",", ".");
        s.ToDouble(&out);
    };

    readDouble(m_massPistonInput, info.mass_piston);
    readDouble(m_massRodInput,    info.mass_rod);
    readDouble(m_kRodOscInput,    info.k_rod_osc);
    readDouble(m_dynBoreInput,    info.bore);

    return info;
}

void MainFrame::OnDynSaveCsvWide(wxCommandEvent&)
{
    if (m_dynResults.total_torque.empty() || m_lastResults.alpha.empty())
    {
        wxMessageBox(wxString::FromUTF8("Нет результатов динамики для экспорта. Сначала выполните расчёт динамики."),
                     wxString::FromUTF8("Экспорт"), wxOK | wxICON_WARNING, this);
        return;
    }

    wxFileDialog dlg(this,
                     wxString::FromUTF8("Сохранить CSV (табличный формат)"),
                     "", "dynamic_wide.csv",
                     "CSV files (*.csv)|*.csv|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dlg.ShowModal() != wxID_OK) return;

    DynamicExportInfo info = BuildDynExportInfo();

    const bool ok = DynamicOutput::saveToCSVWide(m_lastResults.alpha, m_dynResults, m_lastParams, info,
                                                std::string(dlg.GetPath().mb_str()));
    if (!ok)
    {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить CSV."), wxString::FromUTF8("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnDynSaveCsvLong(wxCommandEvent&)
{
    if (m_dynResults.total_torque.empty() || m_lastResults.alpha.empty())
    {
        wxMessageBox(wxString::FromUTF8("Нет результатов динамики для экспорта. Сначала выполните расчёт динамики."),
                     wxString::FromUTF8("Экспорт"), wxOK | wxICON_WARNING, this);
        return;
    }

    wxFileDialog dlg(this,
                     wxString::FromUTF8("Сохранить CSV (инженерный формат)"),
                     "", "dynamic_long.csv",
                     "CSV files (*.csv)|*.csv|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dlg.ShowModal() != wxID_OK) return;

    DynamicExportInfo info = BuildDynExportInfo();

    const bool ok = DynamicOutput::saveToCSVLong(m_lastResults.alpha, m_dynResults, m_lastParams, info,
                                                std::string(dlg.GetPath().mb_str()));
    if (!ok)
    {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить CSV."), wxString::FromUTF8("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnDynSaveTxt(wxCommandEvent&)
{
    if (m_dynResults.total_torque.empty() || m_lastResults.alpha.empty())
    {
        wxMessageBox(wxString::FromUTF8("Нет результатов динамики для экспорта. Сначала выполните расчёт динамики."),
                     wxString::FromUTF8("Экспорт"), wxOK | wxICON_WARNING, this);
        return;
    }

    wxFileDialog dlg(this,
                     wxString::FromUTF8("Сохранить TXT"),
                     "", "dynamic_report.txt",
                     "Text files (*.txt)|*.txt|All files (*.*)|*.*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dlg.ShowModal() != wxID_OK) return;

    DynamicExportInfo info = BuildDynExportInfo();

    const bool ok = DynamicOutput::saveToFormattedText(m_lastResults.alpha, m_dynResults, m_lastParams, info,
                                                      std::string(dlg.GetPath().mb_str()));
    if (!ok)
    {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить TXT."), wxString::FromUTF8("Ошибка"),
                     wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnDynCylinderCheckListChanged(wxCommandEvent&)
{
    UpdateDynamicGraph();
}

void MainFrame::OnDynSelectAllDynCylinders(wxCommandEvent&)
{
    if (!m_dynCylinderCheckList) return;
    for (unsigned i = 0; i < m_dynCylinderCheckList->GetCount(); ++i)
        m_dynCylinderCheckList->Check(i, true);

    UpdateDynamicGraph();
}

void MainFrame::OnDynClearAllDynCylinders(wxCommandEvent&)
{
     if (!m_dynCylinderCheckList) return;

    for (unsigned i = 0; i < m_dynCylinderCheckList->GetCount(); ++i)
        m_dynCylinderCheckList->Check(i, false);

    // ✅ Явно очищаем график, если цилиндры не выбраны
    if (m_dynPlotResult)
    {
        m_dynPlotResult->Clear();   // добавим метод, см. ниже
        m_dynPlotResult->Refresh();
    }
}


// =====================================================================
//  Обработчики событий
// =====================================================================






void MainFrame::OnKinematicComplete(wxThreadEvent& evt) {
    m_kinCalcInProgress = false;
    if (m_calcButton) m_calcButton->Enable(true);
    // если поток сообщил ошибку
    if (!evt.GetString().IsEmpty()) {
        wxMessageBox(evt.GetString(),
                     wxString::FromUTF8("Ошибка расчёта"),
                     wxOK | wxICON_ERROR, this);
        return;
    }

    // забираем результат
    auto results = evt.GetPayload<std::shared_ptr<CalculationResults>>();
    if (!results) {
        wxMessageBox(wxString::FromUTF8("Расчёт не вернул результаты (nullptr)."),
                     wxString::FromUTF8("Ошибка расчёта"),
                     wxOK | wxICON_ERROR, this);
        return;
    }

    m_lastResults = std::move(*results);
    m_hasResults = true;

    // --- Настроить таблицу углов вспышки под текущий двигатель ---
if (m_ignitionPanelHost)
{
    auto* ip = static_cast<LayoutPanel*>(m_ignitionPanelHost);

    ip->Configure((int)m_lastParams.countCyl, (int)m_lastParams.taktnost);

    // Автокомпоновка: берём ту же раскладку, что и в шаге 1
    if (m_lastParams.layout_rows > 0 && m_lastParams.layout_sections > 0 &&
        m_lastParams.layout_rows * m_lastParams.layout_sections == (int)m_lastParams.countCyl)
    {
        ip->SetDims(m_lastParams.layout_rows, m_lastParams.layout_sections);
    }

    // Убираем лишнее на странице вспышек
    ip->LockDimensions(true);
    ip->SetTableOnly(true);
}

    // 1) Сначала дать панели данные и параметры
m_plotPanel->SetData(&m_lastResults);
m_plotPanel->SetParams(&m_lastParams);
m_tablePanel->SetData(&m_lastResults, &m_lastParams);

// 2) Заполнить чеклист и выставить дефолтный выбор
UpdateCylinderCheckList();

// 3) Гарантировать, что выбран хотя бы один цилиндр и выбор дошёл до графика
if (m_cylinderCheckList && m_cylinderCheckList->GetCount() > 0) {
    // если вдруг ничего не отмечено — отметим 1-й
    bool anyChecked = false;
    for (unsigned i = 0; i < m_cylinderCheckList->GetCount(); ++i) {
        if (m_cylinderCheckList->IsChecked(i)) { anyChecked = true; break; }
    }
    if (!anyChecked) m_cylinderCheckList->Check(0, true);

    std::vector<int> selected;
    for (unsigned i = 0; i < m_cylinderCheckList->GetCount(); ++i)
        if (m_cylinderCheckList->IsChecked(i))
            selected.push_back((int)i);

    m_plotPanel->SetSelectedIndices(selected);
}

// 4) Показать результаты и принудительно обновить график
m_kinematicBook->SetSelection(1);
m_plotPanel->Refresh();
m_plotPanel->Update();
    UpdateSidebarSteps();
}

void MainFrame::OnDynamicComplete(wxThreadEvent& evt)
{
    if (m_dynCalcBtn) m_dynCalcBtn->Enable(true);

    if (!evt.GetString().IsEmpty()) {
        wxMessageBox(evt.GetString(),
                     wxString::FromUTF8("Ошибка динамики"),
                     wxOK | wxICON_ERROR, this);
          if (m_dynCalcStatus)
    m_dynCalcStatus->SetLabel(wxString::FromUTF8("Ошибка ⚠"));           
        return;
    }

    auto resPtr = evt.GetPayload<std::shared_ptr<DynamicResults>>();
    if (!resPtr) {
        wxMessageBox(wxString::FromUTF8("Динамический расчёт не вернул результаты."),
                     wxString::FromUTF8("Ошибка динамики"),
                     wxOK | wxICON_ERROR, this);
        return;
    }

    m_dynResults = std::move(*resPtr);

    // Обновить список цилиндров под динамику (если он пустой — график будет “пустым”)
    if (m_dynCylinderCheckList) {
        m_dynCylinderCheckList->Clear();
        const int nCyl = (int)m_dynResults.gas_force.size();
        for (int i = 0; i < nCyl; ++i)
            m_dynCylinderCheckList->Append(wxString::Format(wxString::FromUTF8("Цил.%d"), i + 1));
        if (nCyl > 0) m_dynCylinderCheckList->Check(0, true);
    }

    UpdateDynamicControlsState();
    UpdateDynamicGraph();
    UpdateDynamicTable();
    m_hasDynamicResults = !m_dynResults.total_torque.empty();
    m_dynamicBook->SetSelection(2);
    m_rightBook->SetSelection(1);
    UpdateSidebarSteps();

    if (m_dynCalcStatus)
    m_dynCalcStatus->SetLabel(wxString::FromUTF8("Готово ✅"));
}

void MainFrame::OnCalculate(wxCommandEvent&)
{
    if (m_kinCalcInProgress) return; // уже идёт расчёт

    EngineParams params;
    wxString err;
    if (!ReadParamsFromUI(params, err)) {
        wxMessageBox(err, wxString::FromUTF8("Ошибка ввода"), wxOK | wxICON_ERROR, this);
        return;
    }

    m_lastParams = params;

    m_kinCalcInProgress = true;
    if (m_calcButton) m_calcButton->Enable(false);

    // (не обязательно, но полезно) покажи пользователю, что стартанули
    if (m_resultStatus) m_resultStatus->SetLabel(wxString::FromUTF8("Расчет успешен"));

    KinematicThread* thread = new KinematicThread(this, params);

    if (thread->Create() != wxTHREAD_NO_ERROR) {
        m_kinCalcInProgress = false;
        if (m_calcButton) m_calcButton->Enable(true);
        wxMessageBox(wxString::FromUTF8("Не удалось создать поток"),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        delete thread;
        return;
    }

    const auto rc = thread->Run();
    if (rc != wxTHREAD_NO_ERROR) {
        m_kinCalcInProgress = false;
        if (m_calcButton) m_calcButton->Enable(true);

        wxMessageBox(wxString::Format("Не удалось запустить поток. Код: %d", (int)rc),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);

        // если поток не стартанул — его надо удалить вручную (detached сам себя не удалит)
        delete thread;
        return;
    }
}

void MainFrame::OnCalculateDynamic(wxCommandEvent&)
{
    // 1) Нужна кинематика
    if (!m_hasResults) {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт кинематики."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    // 2) Нужны данные давления
    if (m_dynPressureAngles.empty() || m_dynPressureValues.empty()) {
        wxMessageBox(wxString::FromUTF8("Загрузите файл с индикаторной диаграммой."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }
    if (m_dynPressureAngles.size() != m_dynPressureValues.size()) {
        wxMessageBox(wxString::FromUTF8("Данные давления некорректны: angles и pressures разной длины."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    // 3) Читаем числа (с поддержкой запятой)
    double massPiston = 0.0, massRod = 0.0, kRodOsc = 0.0, bore = 0.0;

    if (!ReadDoubleFromText(m_massPistonInput, massPiston) ||
        !ReadDoubleFromText(m_massRodInput,    massRod)    ||
        !ReadDoubleFromText(m_kRodOscInput,    kRodOsc)    ||
        !ReadDoubleFromText(m_dynBoreInput,    bore)       ||
        bore <= 0.0)
    {
        wxMessageBox(wxString::FromUTF8("Проверьте введённые массы и диаметр цилиндра."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    // (опционально, но полезно) простая валидация диапазонов
    if (massPiston <= 0.0 || massRod <= 0.0 || kRodOsc < 0.0 || kRodOsc > 1.0) {
        wxMessageBox(wxString::FromUTF8("Массы должны быть > 0, доля k должна быть в диапазоне [0..1]."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        return;
    }

    if (m_dynCalcStatus)
    m_dynCalcStatus->SetLabel(wxString::FromUTF8("Расчёт динамики..."));

    // 4) Блокируем кнопку, стартуем поток
    if (m_dynCalcBtn) m_dynCalcBtn->Enable(false);


    // -------------------------------------------------
// Углы вспышки (рабочего такта) + допуск ВМТ
// -------------------------------------------------
if (!m_ignitionPanelHost)
{
    wxMessageBox(wxString::FromUTF8("Не найдена таблица углов вспышки (панель)."),
                 wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
    return;
}

auto* ip = static_cast<LayoutPanel*>(m_ignitionPanelHost);

std::vector<double> ignitionAngles;
wxString iErr;
if (!ip->GetPhases(ignitionAngles, iErr))
{
    wxMessageBox(wxString::FromUTF8("Углы вспышки: ") + iErr,
                 wxString::FromUTF8("Ошибка ввода"), wxOK | wxICON_ERROR, this);
    return;
}

if ((int)ignitionAngles.size() != (int)m_lastParams.countCyl)
{
    wxMessageBox(wxString::FromUTF8("Углы вспышки: размер массива не равен числу цилиндров."),
                 wxString::FromUTF8("Ошибка ввода"), wxOK | wxICON_ERROR, this);
    return;
}

// Записываем в параметры двигателя (для DynamicCalculator)
m_lastParams.cyl_cycle_phase_deg = ignitionAngles;

// допуск ВМТ
double tol = 1.0;
if (m_tdcTolInput)
{
    wxString s = m_tdcTolInput->GetValue();
    s.Replace(",", ".");
    if (!s.ToDouble(&tol) || tol <= 0.0) tol = 1.0;
}
m_lastParams.tdc_tolerance_deg = tol;



    auto* thread = new DynamicThread(
        this,
        m_lastParams,
        m_lastResults,
        m_dynPressureAngles,
        m_dynPressureValues,
        massPiston,
        massRod,
        kRodOsc,
        bore
    );

    if (thread->Create() != wxTHREAD_NO_ERROR) {
        if (m_dynCalcBtn) m_dynCalcBtn->Enable(true);
        wxMessageBox(wxString::FromUTF8("Не удалось создать поток для расчёта динамики."),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        delete thread;
        return;
    }

    const auto rc = thread->Run();
    if (rc != wxTHREAD_NO_ERROR) {
        if (m_dynCalcBtn) m_dynCalcBtn->Enable(true);
        wxMessageBox(wxString::Format("Не удалось запустить поток для расчёта динамики. Код: %d", (int)rc),
                     wxString::FromUTF8("Ошибка"), wxOK | wxICON_ERROR, this);
        delete thread; // detached-поток сам себя не удалит, если не стартовал
        return;
    }
}
