#include "MainFrame.h"
#include <wx/sizer.h>
#include <wx/statline.h>

enum
{
    ID_CalcButton = wxID_HIGHEST + 1,
    ID_SaveCsvButton
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_CalcButton,    MainFrame::OnCalculate)
    EVT_BUTTON(ID_SaveCsvButton, MainFrame::OnSaveCsv)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(900, 600))
{
    auto* notebook = new wxNotebook(this, wxID_ANY);

    // ---------- Вкладка "Кинематика" ----------
    auto* panelKinematic = new wxPanel(notebook, wxID_ANY);
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(5, 2, 5, 5);
    grid->AddGrowableCol(1, 1);

    // Шаг α
    grid->Add(new wxStaticText(panelKinematic, wxID_ANY,
                               wxString::FromUTF8("Шаг α, град:")),
              0, wxALIGN_CENTER_VERTICAL);
    m_stepAlphaInput = new wxTextCtrl(panelKinematic, wxID_ANY, "1.0");
    grid->Add(m_stepAlphaInput, 1, wxEXPAND);

    // Предел α
    grid->Add(new wxStaticText(panelKinematic, wxID_ANY,
                               wxString::FromUTF8("Предел α, град:")),
              0, wxALIGN_CENTER_VERTICAL);
    m_endAlphaInput = new wxTextCtrl(panelKinematic, wxID_ANY, "360.0");
    grid->Add(m_endAlphaInput, 1, wxEXPAND);

    // Радиус кривошипа
    grid->Add(new wxStaticText(panelKinematic, wxID_ANY,
                               wxString::FromUTF8("Радиус кривошипа r, м:")),
              0, wxALIGN_CENTER_VERTICAL);
    m_radcrankInput = new wxTextCtrl(panelKinematic, wxID_ANY, "0.020");
    grid->Add(m_radcrankInput, 1, wxEXPAND);

    // λ
    grid->Add(new wxStaticText(panelKinematic, wxID_ANY,
                               wxString::FromUTF8("Геометрическая характеристика λ:")),
              0, wxALIGN_CENTER_VERTICAL);
    m_lambdaInput = new wxTextCtrl(panelKinematic, wxID_ANY, "0.3");
    grid->Add(m_lambdaInput, 1, wxEXPAND);

    // n
    grid->Add(new wxStaticText(panelKinematic, wxID_ANY,
                               wxString::FromUTF8("Частота вращения n, об/мин:")),
              0, wxALIGN_CENTER_VERTICAL);
    m_nInput = new wxTextCtrl(panelKinematic, wxID_ANY, "4800");
    grid->Add(m_nInput, 1, wxEXPAND);

    vbox->Add(grid, 0, wxALL | wxEXPAND, 10);

    vbox->Add(new wxStaticLine(panelKinematic, wxID_ANY),
              0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10);

    // Кнопки
    auto* hboxButtons = new wxBoxSizer(wxHORIZONTAL);
    auto* calcButton  = new wxButton(panelKinematic, ID_CalcButton,
                                     wxString::FromUTF8("Рассчитать кинематику"));
    auto* saveButton  = new wxButton(panelKinematic, ID_SaveCsvButton,
                                     wxString::FromUTF8("Сохранить в CSV"));

    hboxButtons->Add(calcButton, 0, wxRIGHT, 10);
    hboxButtons->Add(saveButton, 0);

    vbox->Add(hboxButtons, 0, wxALL, 10);

    // Поле результатов
    m_resultBox = new wxTextCtrl(
        panelKinematic,
        wxID_ANY,
        wxString::FromUTF8("Расчёт ещё не выполнялся."),
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY
    );
    vbox->Add(m_resultBox, 1, wxALL | wxEXPAND, 10);

    panelKinematic->SetSizer(vbox);
    notebook->AddPage(panelKinematic, wxString::FromUTF8("Кинематика КШМ"), true);

    // ---------- Вкладка "Настройки" ----------
    auto* panelSettings = new wxPanel(notebook, wxID_ANY);
    auto* settingsSizer = new wxBoxSizer(wxVERTICAL);
    settingsSizer->Add(
        new wxStaticText(panelSettings, wxID_ANY,
                         wxString::FromUTF8("Здесь позже будут общие настройки программы.")),
        0, wxALL, 10
    );
    panelSettings->SetSizer(settingsSizer);
    notebook->AddPage(panelSettings, wxString::FromUTF8("Настройки"), false);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(notebook, 1, wxEXPAND);
    SetSizer(mainSizer);

    Centre();
}

// ---------- Обработчик "Рассчитать" ----------
void MainFrame::OnCalculate(wxCommandEvent& event)
{
    double stepAlpha = 0.0;
    double endAlpha  = 0.0;
    double radcrank  = 0.0;
    double lambda    = 0.0;
    double n         = 0.0;

    if (!m_stepAlphaInput->GetValue().ToDouble(&stepAlpha) ||
        !m_endAlphaInput->GetValue().ToDouble(&endAlpha)   ||
        !m_radcrankInput->GetValue().ToDouble(&radcrank)   ||
        !m_lambdaInput->GetValue().ToDouble(&lambda)       ||
        !m_nInput->GetValue().ToDouble(&n))
    {
        wxMessageBox(wxString::FromUTF8(
                         "Некоторые параметры не удалось преобразовать в число."),
                     wxString::FromUTF8("Ошибка ввода"),
                     wxOK | wxICON_ERROR, this);
        return;
    }

    if (stepAlpha <= 0 || stepAlpha > 360 ||
        endAlpha <= 0  || endAlpha > 720 ||
        radcrank <= 0  ||
        lambda <= 0    || lambda >= 1 ||
        n <= 0)
    {
        wxMessageBox(wxString::FromUTF8(
                         "Некорректные параметры.\n"
                         "Проверьте шаг/предел угла, радиус, λ и n."),
                     wxString::FromUTF8("Ошибка параметров"),
                     wxOK | wxICON_ERROR, this);
        return;
    }

    EngineParams params{};
    params.step_alpha = stepAlpha;
    params.end_alpha  = endAlpha;
    params.radcrank   = radcrank;
    params.lyambda    = lambda;
    params.n          = n;

    params.gamma      = 0.0;
    params.gammaPric  = 0.0;
    params.dezaxial   = 0.0;
    params.radcrank1  = 0.0;
    params.lengthRod1 = 0.0;

    CalculationResults results = calcCylinderKinematics(params);

    m_lastParams  = params;
    m_lastResults = results;
    m_hasResults  = true;

    wxString info;
    info << wxString::FromUTF8("Расчёт выполнен.\n");
    info << wxString::FromUTF8("Тип КШМ: аксиальный (e = 0, γ = 0, γp = 0).\n");
    info << wxString::FromUTF8("Количество точек: ")
         << static_cast<unsigned long>(results.alpha.size()) << "\n\n";
    info << wxString::FromUTF8(
        "Теперь можно сохранить результаты в CSV и дальше работать с ними в Excel/Matlab.");

    m_resultBox->SetValue(info);
}

// ---------- Обработчик "Сохранить в CSV" ----------
void MainFrame::OnSaveCsv(wxCommandEvent& event)
{
    // 1) Проверяем, есть ли что сохранять
    if (!m_hasResults)   // или твой флаг
    {
        wxMessageBox(
            wxString::FromUTF8("Сначала выполните расчёт кинематики."),
            wxString::FromUTF8("Нет данных для сохранения"),
            wxOK | wxICON_INFORMATION,
            this
        );
        return;
    }

    // 2) Диалог выбора файла
    wxFileDialog saveDlg(
        this,
        wxString::FromUTF8("Сохранить результаты в CSV"),
        "",                             // каталог по умолчанию
        "ksm_results.csv",              // имя по умолчанию
        wxString::FromUTF8("CSV файлы (*.csv)|*.csv|Все файлы (*.*)|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

    if (saveDlg.ShowModal() != wxID_OK)
        return; // пользователь нажал Cancel

    wxString wxPath = saveDlg.GetPath();

    // 3) Конвертация пути в std::string (UTF-8)
    std::string filename = wxPath.ToUTF8().data();

    // 4) Сохранение через твой уже готовый класс
    bool ok = KinematicOutput::saveToCSV(
        m_lastResults,   // CalculationResults
        m_lastParams,    // EngineParams
        filename         // выбранный пользователем путь
    );

    // 5) Сообщение пользователю
    if (ok)
    {
        wxMessageBox(
            wxString::FromUTF8("Файл успешно сохранён:\n") + wxPath,
            wxString::FromUTF8("Сохранение завершено"),
            wxOK | wxICON_INFORMATION,
            this
        );
    }
    else
    {
        wxMessageBox(
            wxString::FromUTF8("Не удалось сохранить файл.\nПодробности смотри в консоли."),
            wxString::FromUTF8("Ошибка сохранения"),
            wxOK | wxICON_ERROR,
            this
        );
    }
}
