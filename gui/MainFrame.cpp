#include "MainFrame.h"

#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>

using namespace std;

// ID'шники
enum
{
    ID_SidebarKinematic = wxID_HIGHEST + 1,
    ID_SidebarDynamic,
    ID_CalcButton,
    ID_BackButton,
    ID_SaveCSVButton,
    ID_SaveTXTButton,
    ID_KSMTypeChoice,
    ID_GraphTypeChoice
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_SidebarKinematic, MainFrame::OnSidebarKinematic)
    EVT_BUTTON(ID_SidebarDynamic,   MainFrame::OnSidebarDynamic)
    EVT_CHOICE(ID_KSMTypeChoice,    MainFrame::OnKSMTypeChanged)
    EVT_BUTTON(ID_CalcButton,       MainFrame::OnCalculate)
    EVT_CHOICE(ID_GraphTypeChoice,  MainFrame::OnGraphTypeChanged)
    EVT_BUTTON(ID_BackButton,       MainFrame::OnBackToInput)
    EVT_BUTTON(ID_SaveCSVButton,       MainFrame::OnSaveCsv)
    EVT_BUTTON(ID_SaveTXTButton,       MainFrame::OnSaveTxt)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1150, 720))
{
    BuildLayout();
    Centre();
}

// =====================================================================
//  Построение общей компоновки
// =====================================================================

void MainFrame::BuildLayout()
{
    auto* mainPanel = new wxPanel(this);
    ApplyDarkTheme(mainPanel);

    auto* hbox = new wxBoxSizer(wxHORIZONTAL);

    // --- левая панель ---
    m_sidebarPanel = new wxPanel(mainPanel);
    ApplyDarkTheme(m_sidebarPanel);
    BuildSidebar(m_sidebarPanel);

    // --- правая колонка ---
    auto* rightPanel = new wxPanel(mainPanel);
    ApplyDarkTheme(rightPanel);

    m_rightBook = new wxSimplebook(rightPanel, wxID_ANY);
    ApplyDarkTheme(m_rightBook);

    // страница "Кинематика"
    auto* kinPage = new wxPanel(m_rightBook);
    ApplyDarkTheme(kinPage);
    BuildKinematicPages(kinPage);
    m_rightBook->AddPage(kinPage, wxString::FromUTF8("Расчёт кинематики"), true);

    // страница "Динамика" (заглушка)
    auto* dynPage = new wxPanel(m_rightBook);
    ApplyDarkTheme(dynPage);
    auto* dynSizer = new wxBoxSizer(wxVERTICAL);
    dynSizer->Add(
        new wxStaticText(dynPage, wxID_ANY,
                         wxString::FromUTF8("РАСЧЁТ ДИНАМИКИ — в разработке")),
        0, wxALL, 20);
    dynPage->SetSizer(dynSizer);
    m_rightBook->AddPage( dynPage, wxString::FromUTF8("Расчёт динамики"), false);

    auto* rightSizer = new wxBoxSizer(wxVERTICAL);
    rightSizer->Add(m_rightBook, 1, wxEXPAND | wxALL, 10);
    rightPanel->SetSizer(rightSizer);

    hbox->Add(m_sidebarPanel, 0, wxEXPAND);
    hbox->Add(rightPanel, 1, wxEXPAND);

    mainPanel->SetSizer(hbox);
}

// =====================================================================
//  Левая панель
// =====================================================================

void MainFrame::BuildSidebar(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* title = new wxStaticText(
        parent, wxID_ANY,
        wxString::FromUTF8("РАСЧЁТ ДВИГАТЕЛЯ"));
    title->SetForegroundColour(*wxLIGHT_GREY);
    title->SetFont(wxFontInfo(12).Bold());

    vbox->Add(title, 0, wxALL, 20);

    m_btnKinematic = new wxButton(
        parent, ID_SidebarKinematic,
        wxString::FromUTF8("РАСЧЁТ КИНЕМАТИКИ"),
        wxDefaultPosition, wxSize(220, 40),
        wxBORDER_NONE);
    m_btnDynamic = new wxButton(
        parent, ID_SidebarDynamic,
        wxString::FromUTF8("РАСЧЁТ ДИНАМИКИ"),
        wxDefaultPosition, wxSize(220, 40),
        wxBORDER_NONE);

    ApplyDarkTheme(m_btnKinematic);
    ApplyDarkTheme(m_btnDynamic);

    vbox->Add(m_btnKinematic, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    vbox->Add(m_btnDynamic,   0, wxLEFT | wxRIGHT | wxTOP, 10);

    vbox->AddStretchSpacer();
    parent->SetSizer(vbox);
}

void MainFrame::OnSidebarKinematic(wxCommandEvent&)
{
    m_rightBook->SetSelection(0);
}

void MainFrame::OnSidebarDynamic(wxCommandEvent&)
{
    m_rightBook->SetSelection(1);
}

// =====================================================================
//  Страницы раздела "Кинематика"
// =====================================================================

void MainFrame::BuildKinematicPages(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // --- заголовок ---
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* title = new wxStaticText(
        parent, wxID_ANY,
        wxString::FromUTF8("РАСЧЁТ КИНЕМАТИКИ"));
    title->SetForegroundColour(*wxWHITE);
    title->SetFont(wxFontInfo(16).Bold());

    headerSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL);

    headerSizer->AddStretchSpacer();

    headerSizer->Add(
        new wxStaticText(parent, wxID_ANY,
                         wxString::FromUTF8("Тип КШМ:")),
        0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    wxString types[] = {
        wxString::FromUTF8("Аксиальный"),
        wxString::FromUTF8("Дезаксиальный"),
        wxString::FromUTF8("V-образный"),
        wxString::FromUTF8("V-образный + дезаксиал"),
        wxString::FromUTF8("V-образный с прицепным шатуном"),
        wxString::FromUTF8("V-образный с прицепным шатуном + дезаксиал")
    };

    m_ksmTypeChoice = new wxChoice(parent, ID_KSMTypeChoice,
                                   wxDefaultPosition, wxSize(260, -1),
                                   WXSIZEOF(types), types);
    m_ksmTypeChoice->SetSelection(0);

    headerSizer->Add(m_ksmTypeChoice, 0, wxALIGN_CENTER_VERTICAL);

    vbox->Add(headerSizer, 0, wxALL | wxEXPAND, 10);
    vbox->Add(new wxStaticLine(parent), 0,
              wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

    // --- книжка "ввод / результат" ---
    m_kinematicBook = new wxSimplebook(parent, wxID_ANY);
    ApplyDarkTheme(m_kinematicBook);

    // ===== страница ввода =====
    m_kinInputPanel = new wxPanel(m_kinematicBook);
    ApplyDarkTheme(m_kinInputPanel);

    auto* grid = new wxFlexGridSizer(0, 2, 8, 12);
    grid->AddGrowableCol(1, 1);

    auto addRow = [&](const wxString& label, wxTextCtrl*& ctrl,
                      const wxString& def = "0.0") {
        auto* lbl = new wxStaticText(m_kinInputPanel, wxID_ANY, label);
        grid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
        ctrl = new wxTextCtrl(m_kinInputPanel, wxID_ANY, def);
        grid->Add(ctrl, 1, wxEXPAND);
        ApplyDarkTheme(lbl);
        ApplyDarkTheme(ctrl);
    };

    addRow(wxString::FromUTF8("Шаг α, град:"),             m_stepAlphaInput, "1.0");
    addRow(wxString::FromUTF8("Предел α, град:"),          m_endAlphaInput,  "360.0");
    addRow(wxString::FromUTF8("Радиус кривошипа r, м:"),   m_radcrankInput,  "0.020");
    addRow(wxString::FromUTF8("Геометрическая характеристика λ:"),
           m_lambdaInput, "0.3");
    addRow(wxString::FromUTF8("Частота вращения n, об/мин:"), m_nInput, "4800");

    // доп. параметры
    m_gammaLabel = new wxStaticText(m_kinInputPanel, wxID_ANY,
                                    wxString::FromUTF8("Угол развала γ, град:"));
    grid->Add(m_gammaLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_gammaInput = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "60.0");
    grid->Add(m_gammaInput, 1, wxEXPAND);
    ApplyDarkTheme(m_gammaLabel);
    ApplyDarkTheme(m_gammaInput);

    m_dezaxLabel = new wxStaticText(m_kinInputPanel, wxID_ANY,
                                    wxString::FromUTF8("Дезаксиал e, м:"));
    grid->Add(m_dezaxLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_dezaxInput = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.0");
    grid->Add(m_dezaxInput, 1, wxEXPAND);
    ApplyDarkTheme(m_dezaxLabel);
    ApplyDarkTheme(m_dezaxInput);

    m_gammaPricLabel = new wxStaticText(m_kinInputPanel, wxID_ANY,
                                        wxString::FromUTF8("Угол прицепного шатуна γp, град:"));
    grid->Add(m_gammaPricLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_gammaPricInput = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.0");
    grid->Add(m_gammaPricInput, 1, wxEXPAND);
    ApplyDarkTheme(m_gammaPricLabel);
    ApplyDarkTheme(m_gammaPricInput);

    m_radcrank1Label = new wxStaticText(m_kinInputPanel, wxID_ANY,
                                        wxString::FromUTF8("Радиус кривошипа прицепного r1, м:"));
    grid->Add(m_radcrank1Label, 0, wxALIGN_CENTER_VERTICAL);
    m_radcrank1Input = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.02");
    grid->Add(m_radcrank1Input, 1, wxEXPAND);
    ApplyDarkTheme(m_radcrank1Label);
    ApplyDarkTheme(m_radcrank1Input);

    m_lengthRod1Label = new wxStaticText(m_kinInputPanel, wxID_ANY,
                                         wxString::FromUTF8("Длина прицепного шатуна L1, м:"));
    grid->Add(m_lengthRod1Label, 0, wxALIGN_CENTER_VERTICAL);
    m_lengthRod1Input = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.10");
    grid->Add(m_lengthRod1Input, 1, wxEXPAND);
    ApplyDarkTheme(m_lengthRod1Label);
    ApplyDarkTheme(m_lengthRod1Input);

    auto* inputSizer = new wxBoxSizer(wxVERTICAL);
    inputSizer->Add(grid, 0, wxALL | wxEXPAND, 20);

    m_calcButton = new wxButton(m_kinInputPanel, ID_CalcButton,
                                wxString::FromUTF8("РАССЧИТАТЬ"));
    ApplyDarkTheme(m_calcButton);
    inputSizer->Add(m_calcButton, 0,
                    wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 20);

    m_kinInputPanel->SetSizer(inputSizer);

    // ===== страница результата =====
    m_kinResultPanel = new wxPanel(m_kinematicBook);
    ApplyDarkTheme(m_kinResultPanel);

    auto* resSizer = new wxBoxSizer(wxVERTICAL);

    m_resultStatus = new wxStaticText(
        m_kinResultPanel, wxID_ANY,
        wxString::FromUTF8("Расчёт ещё не выполнялся."));
    ApplyDarkTheme(m_resultStatus);
    resSizer->Add(m_resultStatus, 0, wxALL, 10);

    auto* topResSizer = new wxBoxSizer(wxHORIZONTAL);

    // слева — выбор типа графика
    auto* graphCtrlSizer = new wxBoxSizer(wxVERTICAL);
    auto* graphLabel = new wxStaticText(
        m_kinResultPanel, wxID_ANY,
        wxString::FromUTF8("Тип графика:"));
    ApplyDarkTheme(graphLabel);
    graphCtrlSizer->Add(graphLabel, 0, wxBOTTOM, 5);

    wxArrayString graphTypes;


    if (m_gammaInput != 0) {
        graphTypes.Add(wxString::FromUTF8("Перемещение Главный цилиндр"));
        graphTypes.Add(wxString::FromUTF8("Скорость Главный цилиндр"));
        graphTypes.Add(wxString::FromUTF8("Ускорение Главный цилиндр"));
        graphTypes.Add(wxString::FromUTF8("Перемещение Боковой цилиндр"));
        graphTypes.Add(wxString::FromUTF8("Скорость Боковой цилиндр"));
        graphTypes.Add(wxString::FromUTF8("Ускорение Боковой цилиндр"));
    }
    else {
        graphTypes.Add(wxString::FromUTF8("Перемещение"));
        graphTypes.Add(wxString::FromUTF8("Скорость"));
        graphTypes.Add(wxString::FromUTF8("Ускорение"));
    }
    
    
    
    m_graphTypeChoice = new wxChoice(m_kinResultPanel, ID_GraphTypeChoice,
                                     wxDefaultPosition, wxSize(180, -1), graphTypes);
    m_graphTypeChoice->SetSelection(0);
    ApplyDarkTheme(m_graphTypeChoice);
    graphCtrlSizer->Add(m_graphTypeChoice, 0, wxBOTTOM, 10);

    topResSizer->Add(graphCtrlSizer, 0, wxALL | wxALIGN_TOP, 10);

    // справа — сам график
    m_plotPanel = new KinematicPlotPanel(m_kinResultPanel);
    topResSizer->Add(m_plotPanel, 1, wxEXPAND | wxALL, 10);

    resSizer->Add(topResSizer, 1, wxEXPAND);

    // нижние кнопки
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_backButton = new wxButton(m_kinResultPanel, ID_BackButton,
                                wxString::FromUTF8("← Назад"));
    m_saveCSVButton = new wxButton(m_kinResultPanel,  ID_SaveCSVButton,
                                wxString::FromUTF8("\n Сохранить результаты \n в CSV \n"));
    m_saveTXTButton = new wxButton(m_kinResultPanel,  ID_SaveTXTButton,
                                wxString::FromUTF8("\n Сохранить результаты \n в TXT \n"));
    ApplyDarkTheme(m_backButton);
    ApplyDarkTheme(m_saveCSVButton);
    ApplyDarkTheme(m_saveTXTButton);

    bottomSizer->Add(m_backButton, 0, wxRIGHT, 10);
    bottomSizer->Add(m_saveCSVButton, 0);
    bottomSizer->Add(m_saveTXTButton, 0);

    resSizer->Add(bottomSizer, 0, wxALIGN_LEFT | wxALL, 10);

    m_kinResultPanel->SetSizer(resSizer);

    // добавляем страницы в книжку
    m_kinematicBook->AddPage( m_kinInputPanel, wxString::FromUTF8("Ввод параметров"), true);

    m_kinematicBook->AddPage( m_kinResultPanel, wxString::FromUTF8("Результаты"), false);

    vbox->Add(m_kinematicBook, 1, wxEXPAND | wxALL, 5);
    parent->SetSizer(vbox);

    UpdateKSMTypeFromChoice();
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

void MainFrame::UpdateKSMTypeFromChoice()
{
    int sel = m_ksmTypeChoice->GetSelection();
    if (sel < 0) sel = 0;
    m_currentType = static_cast<KSMType>(sel);
    UpdateParameterVisibility();
}

void MainFrame::UpdateParameterVisibility()
{
    bool showGamma      = false;
    bool showDezax      = false;
    bool showGammaPric  = false;
    bool showR1         = false;
    bool showL1         = false;

    switch (m_currentType)
    {
    case KSMType::Axial:
        break;
    case KSMType::Deaxial:
        showDezax = true;
        break;
    case KSMType::VShaped:
        showGamma = true;
        break;
    case KSMType::VShapedDeaxial:
        showGamma = true;
        showDezax = true;
        break;
    case KSMType::VShapedAttached:
        showGamma     = true;
        showGammaPric = true;
        showR1        = true;
        showL1        = true;
        break;
    case KSMType::VShapedAttachedDeaxial:
        showGamma     = true;
        showGammaPric = true;
        showR1        = true;
        showL1        = true;
        showDezax     = true;
        break;
    }

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

    m_kinInputPanel->Layout();
}

bool MainFrame::ReadParamsFromUI(EngineParams& p, wxString& err)
{
    double stepA, endA, r, lam, n;
    double gamma = 0.0, dez = 0.0, gammaP = 0.0, r1 = 0.0, L1 = 0.0;

    auto parse = [&](wxTextCtrl* ctrl, double& out) -> bool {
        wxString s = ctrl->GetValue();
        return s.ToDouble(&out);
    };

    if (!parse(m_stepAlphaInput, stepA) ||
        !parse(m_endAlphaInput,  endA)  ||
        !parse(m_radcrankInput,  r)     ||
        !parse(m_lambdaInput,    lam)   ||
        !parse(m_nInput,         n))
    {
        err = wxString::FromUTF8("Некоторые базовые параметры введены неверно.");
        return false;
    }

    if (stepA <= 0 || stepA > 360 ||
        endA  <= 0 || endA > 720 ||
        r <= 0 ||
        lam <= 0 || lam >= 1 ||
        n <= 0)
    {
        err = wxString::FromUTF8("Проверьте шаг/предел α, радиус, λ и n.");
        return false;
    }

    if (m_gammaInput->IsShown() && !parse(m_gammaInput, gamma))
    {
        err = wxString::FromUTF8("Неверное значение угла γ.");
        return false;
    }
    if (m_dezaxInput->IsShown() && !parse(m_dezaxInput, dez))
    {
        err = wxString::FromUTF8("Неверное значение дезаксиала e.");
        return false;
    }
    if (m_gammaPricInput->IsShown() && !parse(m_gammaPricInput, gammaP))
    {
        err = wxString::FromUTF8("Неверное значение угла γp.");
        return false;
    }
    if (m_radcrank1Input->IsShown() && !parse(m_radcrank1Input, r1))
    {
        err = wxString::FromUTF8("Неверное значение r1.");
        return false;
    }
    if (m_lengthRod1Input->IsShown() && !parse(m_lengthRod1Input, L1))
    {
        err = wxString::FromUTF8("Неверное значение длины L1.");
        return false;
    }

    p = EngineParams{};
    p.step_alpha = stepA;
    p.end_alpha  = endA;
    p.radcrank   = r;
    p.lyambda    = lam;
    p.n          = n;

    switch (m_currentType)
    {
    case KSMType::Axial:
        p.gamma      = 0.0;
        p.gammaPric  = 0.0;
        p.dezaxial   = 0.0;
        p.radcrank1  = 0.0;
        p.lengthRod1 = 0.0;
        break;
    case KSMType::Deaxial:
        p.gamma      = 0.0;
        p.gammaPric  = 0.0;
        p.dezaxial   = dez;
        p.radcrank1  = 0.0;
        p.lengthRod1 = 0.0;
        break;
    case KSMType::VShaped:
        p.gamma      = gamma;
        p.gammaPric  = 0.0;
        p.dezaxial   = 0.0;
        p.radcrank1  = 0.0;
        p.lengthRod1 = 0.0;
        break;
    case KSMType::VShapedDeaxial:
        p.gamma      = gamma;
        p.gammaPric  = 0.0;
        p.dezaxial   = dez;
        p.radcrank1  = 0.0;
        p.lengthRod1 = 0.0;
        break;
    case KSMType::VShapedAttached:
        p.gamma      = gamma;
        p.gammaPric  = gammaP;
        p.dezaxial   = 0.0;
        p.radcrank1  = r1;
        p.lengthRod1 = L1;
        break;
    case KSMType::VShapedAttachedDeaxial:
        p.gamma      = gamma;
        p.gammaPric  = gammaP;
        p.dezaxial   = dez;
        p.radcrank1  = r1;
        p.lengthRod1 = L1;
        break;
    }

    return true;
}

// =====================================================================
//  Обработчики событий
// =====================================================================

void MainFrame::OnKSMTypeChanged(wxCommandEvent&)
{
    UpdateKSMTypeFromChoice();
}

void MainFrame::OnCalculate(wxCommandEvent&)
{
    EngineParams params;
    wxString err;
    if (!ReadParamsFromUI(params, err))
    {
        wxMessageBox(err,
                     wxString::FromUTF8("Ошибка ввода"),
                     wxOK | wxICON_ERROR, this);
        return;
    }

    try
    {
        CalculationResults res = calcCylinderKinematics(params);
        m_lastParams  = params;
        m_lastResults = std::move(res);
        m_hasResults  = true;

        wxString st;
        st << wxString::FromUTF8("Расчёт выполнен успешно.\n");
        st << wxString::FromUTF8("Точек: ")
           << static_cast<unsigned long>(m_lastResults.alpha.size());
        m_resultStatus->SetLabel(st);

        m_plotPanel->SetData(&m_lastResults);
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::Displacement);
        m_graphTypeChoice->SetSelection(0);

        m_kinematicBook->SetSelection(1);
    }
    catch (...)
    {
        wxMessageBox(wxString::FromUTF8("Во время расчёта произошла ошибка."),
                     wxString::FromUTF8("Ошибка расчёта"),
                     wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnGraphTypeChanged(wxCommandEvent&)
{
    int sel = m_graphTypeChoice->GetSelection();
    if (sel == 1)
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::Velocity);
    else if (sel == 2)
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::Acceleration);
    else if (sel == 0)
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::Displacement);
    else if (sel == 3 )
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementSide);
    else if (sel == 4 )
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::VelocitySide);
    else if (sel == 5 )
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::AccelerationSide);
}

void MainFrame::OnBackToInput(wxCommandEvent& evt)
{
    if (m_hasResults)
    {
        int ans = wxMessageBox(
            wxString::FromUTF8(
                "Сохранить результаты перед возвратом к вводу параметров?"),
            wxString::FromUTF8("Сохранить в CSV"),
            wxYES_NO | wxCANCEL | wxICON_QUESTION,
            this);

        if (ans == wxCANCEL)
            return;
        if (ans == wxYES)
            OnSaveCsv(evt);
    }

    
    m_kinematicBook->SetSelection(0);
}

void MainFrame::OnSaveCsv(wxCommandEvent&)
{   
    if (!m_hasResults)
    {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт."),
                     wxString::FromUTF8("Нет данных"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    
    wxFileDialog dlg(
        this,
        wxString::FromUTF8("Сохранить"),
        "",
        "ksm_results",
        "CSV|*.csv",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    // Получаем текущее время
    wxDateTime now = wxDateTime::Now();
    // Форматируем время в нужный формат: 19.24.50__22.12.2004
    wxString timestamp = now.Format("%H.%M.%S__%d.%m.%Y");
    
    dlg.SetFilename("ksm_results_" + timestamp+".csv");


    if (dlg.ShowModal() != wxID_OK)
        return;

    std::string filename = dlg.GetPath().ToUTF8().data();

    bool ok = KinematicOutput::saveToCSV(m_lastResults, m_lastParams, filename);
    if (ok)
    {
        wxMessageBox(wxString::FromUTF8("Файл успешно сохранён."),
                     wxString::FromUTF8("Сохранение завершено"),
                     wxOK | wxICON_INFORMATION, this);
    }
    else
    {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить файл."),
                     wxString::FromUTF8("Ошибка сохранения"),
                     wxOK | wxICON_ERROR, this);
    }
}

    void MainFrame::OnSaveTxt(wxCommandEvent&)
{   
    if (!m_hasResults)
    {
        wxMessageBox(wxString::FromUTF8("Сначала выполните расчёт."),
                     wxString::FromUTF8("Нет данных"),
                     wxOK | wxICON_INFORMATION, this);
        return;
    }
    
    wxFileDialog dlg(
        this,
        wxString::FromUTF8("Сохранить"),
        "",
        "ksm_results",
        "TXT|*.txt",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    // Получаем текущее время
    wxDateTime now = wxDateTime::Now();
    // Форматируем время в нужный формат: 19.24.50__22.12.2004
    wxString timestamp = now.Format("%H.%M.%S__%d.%m.%Y");
    
    dlg.SetFilename("ksm_results_" + timestamp+".txt");


    if (dlg.ShowModal() != wxID_OK)
        return;

    std::string filename = dlg.GetPath().ToUTF8().data();

    bool ok = KinematicOutput::saveToFormattedText(m_lastResults, m_lastParams, filename);
    if (ok)
    {
        wxMessageBox(wxString::FromUTF8("Файл успешно сохранён."),
                     wxString::FromUTF8("Сохранение завершено"),
                     wxOK | wxICON_INFORMATION, this);
    }
    else
    {
        wxMessageBox(wxString::FromUTF8("Не удалось сохранить файл."),
                     wxString::FromUTF8("Ошибка сохранения"),
                     wxOK | wxICON_ERROR, this);
    }
}
