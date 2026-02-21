#include "MainFrame.h"
#include "core\Kinematic\Calculations\KinematicCalculator.h"

#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/tokenzr.h>
#include <wx/progdlg.h>
#include <wx/settings.h>

using namespace std;

// ID'шники для элементов управления
enum
{
    ID_SidebarKinematic = wxID_HIGHEST + 1,
    ID_SidebarDynamic,
    ID_CalcButton,
    ID_BackButton,
    ID_SaveCSVButton,
    ID_SaveTXTButton,
    ID_KSMTypeChoice,
    ID_GraphTypeChoice,
    ID_CylinderCheckList,
    ID_ShowSideCheck,
    ID_SelectAllBtn,
    ID_ClearAllBtn,
    ID_SaveGraphPNG,
    ID_TemplateChoice,
    ID_LoadTemplateBtn,
    ID_SaveSessionBtn,
    ID_LoadSessionBtn,
    ID_ResetZoom
    
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_BUTTON(ID_SidebarKinematic, MainFrame::OnSidebarKinematic)
    EVT_BUTTON(ID_SidebarDynamic,   MainFrame::OnSidebarDynamic)
    EVT_CHOICE(ID_KSMTypeChoice,    MainFrame::OnKSMTypeChanged)
    EVT_BUTTON(ID_CalcButton,       MainFrame::OnCalculate)
    EVT_CHOICE(ID_GraphTypeChoice,  MainFrame::OnGraphTypeChanged)
    EVT_CHECKLISTBOX(ID_CylinderCheckList, MainFrame::OnCylinderCheckListChanged)
    EVT_CHECKBOX(ID_ShowSideCheck,         MainFrame::OnShowSideChecked)
    EVT_BUTTON(ID_SelectAllBtn,            MainFrame::OnSelectAll)
    EVT_BUTTON(ID_ClearAllBtn,             MainFrame::OnClearAll)
    EVT_BUTTON(ID_BackButton,       MainFrame::OnBackToInput)
    EVT_BUTTON(ID_SaveCSVButton,    MainFrame::OnSaveCsv)
    EVT_BUTTON(ID_SaveTXTButton,    MainFrame::OnSaveTxt)
    EVT_BUTTON(ID_SaveGraphPNG,     MainFrame::OnSaveGraphPNG)
    EVT_BUTTON(ID_ResetZoom,        MainFrame::OnResetZoom)
    EVT_BUTTON(ID_LoadTemplateBtn, MainFrame::OnLoadTemplate)
    EVT_BUTTON(ID_SaveSessionBtn, MainFrame::OnSaveSession)
    EVT_BUTTON(ID_LoadSessionBtn, MainFrame::OnLoadSession)
wxEND_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1150, 720))
{
    BuildLayout();
    Centre();
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

    // Страница динамики (заглушка)
    auto* dynPage = new wxPanel(m_rightBook);
    ApplyDarkTheme(dynPage);
    auto* dynSizer = new wxBoxSizer(wxVERTICAL);
    dynSizer->Add(new wxStaticText(dynPage, wxID_ANY, wxString::FromUTF8("РАСЧЁТ ДИНАМИКИ — в разработке")),
                  0, wxALL, 20);
    dynPage->SetSizer(dynSizer);
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

    m_btnKinematic = new wxButton(parent, ID_SidebarKinematic,
                                   wxString::FromUTF8("РАСЧЁТ КИНЕМАТИКИ"),
                                   wxDefaultPosition, wxSize(220, 40), wxBORDER_NONE);
    m_btnDynamic = new wxButton(parent, ID_SidebarDynamic,
                                 wxString::FromUTF8("РАСЧЁТ ДИНАМИКИ"),
                                 wxDefaultPosition, wxSize(220, 40), wxBORDER_NONE);
    ApplyDarkTheme(m_btnKinematic);
    ApplyDarkTheme(m_btnDynamic);

    vbox->Add(m_btnKinematic, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    vbox->Add(m_btnDynamic,   0, wxLEFT | wxRIGHT | wxTOP, 10);

    vbox->AddStretchSpacer();
    parent->SetSizer(vbox);
}

void MainFrame::OnSidebarKinematic(wxCommandEvent&) { m_rightBook->SetSelection(0); }
void MainFrame::OnSidebarDynamic(wxCommandEvent&)   { m_rightBook->SetSelection(1); }

void MainFrame::BuildKinematicPages(wxPanel* parent)
{
    auto* vbox = new wxBoxSizer(wxVERTICAL);

    // Заголовок с выбором типа КШМ
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* title = new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("РАСЧЁТ КИНЕМАТИКИ"));
    title->SetForegroundColour(*wxWHITE);
    title->SetFont(wxFontInfo(16).Bold());
    headerSizer->Add(title, 0, wxALIGN_CENTER_VERTICAL);
    headerSizer->AddStretchSpacer();

    headerSizer->Add(new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Тип КШМ:")),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    wxString types[] = {
        wxString::FromUTF8("Аксиальный"),
        wxString::FromUTF8("Дезаксиальный"),
        wxString::FromUTF8("V-образный"),
        wxString::FromUTF8("V-образный + дезаксиал"),
        wxString::FromUTF8("V-образный с прицепным шатуном"),
        wxString::FromUTF8("V-образный с прицепным шатуном + дезаксиал")
    };
    m_ksmTypeChoice = new wxChoice(parent, ID_KSMTypeChoice, wxDefaultPosition, wxSize(260, -1), WXSIZEOF(types), types);
    m_ksmTypeChoice->SetSelection(0);
    headerSizer->Add(m_ksmTypeChoice, 0, wxALIGN_CENTER_VERTICAL);
    vbox->Add(headerSizer, 0, wxALL | wxEXPAND, 10);
    vbox->Add(new wxStaticLine(parent), 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

    // --- Панель выбора шаблона ---
auto* templateSizer = new wxBoxSizer(wxHORIZONTAL);
templateSizer->Add(new wxStaticText(parent, wxID_ANY, wxString::FromUTF8("Шаблоны:")), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

wxArrayString templateNames;
templateNames.Add(wxString::FromUTF8("ВАЗ-2106"));
templateNames.Add(wxString::FromUTF8("ЗМЗ-406"));
templateNames.Add(wxString::FromUTF8("V8 5.0 (аксиальный)"));
// Добавьте другие шаблоны по желанию

m_templateChoice = new wxChoice(parent, ID_TemplateChoice, wxDefaultPosition, wxSize(200, -1), templateNames);
m_templateChoice->SetSelection(0);
ApplyDarkTheme(m_templateChoice);
templateSizer->Add(m_templateChoice, 0, wxRIGHT, 5);

m_loadTemplateBtn = new wxButton(parent, ID_LoadTemplateBtn, wxString::FromUTF8("Загрузить"));
ApplyDarkTheme(m_loadTemplateBtn);
templateSizer->Add(m_loadTemplateBtn, 0);

vbox->Add(templateSizer, 0, wxALL, 10);

    // Книжка ввод/результат
    m_kinematicBook = new wxSimplebook(parent, wxID_ANY);
    ApplyDarkTheme(m_kinematicBook);

    // ========== СТРАНИЦА ВВОДА ПАРАМЕТРОВ ==========
    m_kinInputPanel = new wxPanel(m_kinematicBook);
    ApplyDarkTheme(m_kinInputPanel);

    auto* grid = new wxFlexGridSizer(0, 2, 8, 12);
    grid->AddGrowableCol(1, 1);

    wxToolTip::Enable(true);
wxToolTip::SetDelay(500); // задержка в миллисекундах
wxToolTip::SetAutoPop(10000); // время отображения

    auto addRow = [&](const wxString& label, wxTextCtrl*& ctrl, const wxString& def, 
                  double minVal, double maxVal, const wxString& tooltip, bool allowZero = false, bool positiveOnly = true) {
    auto* lbl = new wxStaticText(m_kinInputPanel, wxID_ANY, label);
    grid->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);
    ctrl = new wxTextCtrl(m_kinInputPanel, wxID_ANY, def);
    ctrl->SetToolTip(tooltip);
    grid->Add(ctrl, 1, wxEXPAND);
    ApplyDarkTheme(lbl);
    ApplyDarkTheme(ctrl);
    
    // Сохраняем параметры валидации в самом поле через клиентские данные? 
    // Но проще привязать обработчик с захватом этих параметров.
    ctrl->Bind(wxEVT_TEXT, [this, ctrl, minVal, maxVal, allowZero, positiveOnly](wxCommandEvent&) {
        ValidateField(ctrl, minVal, maxVal, allowZero, positiveOnly);
        UpdateCalculateButtonState();
    });
};

    // Количество цилиндров
    wxArrayString cylinderOptionsInline;
    cylinderOptionsInline.Add("1"); cylinderOptionsInline.Add("2"); cylinderOptionsInline.Add("3");
    cylinderOptionsInline.Add("4"); cylinderOptionsInline.Add("5"); cylinderOptionsInline.Add("6");
    cylinderOptionsInline.Add("8"); cylinderOptionsInline.Add("10"); cylinderOptionsInline.Add("12");
    cylinderOptionsInline.Add("16");

    wxArrayString cylinderOptionsV;
    cylinderOptionsV.Add("2"); cylinderOptionsV.Add("4"); cylinderOptionsV.Add("6");
    cylinderOptionsV.Add("8"); cylinderOptionsV.Add("10"); cylinderOptionsV.Add("12");
    cylinderOptionsV.Add("16");

    auto* cylLabel = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Количество цилиндров:"));
    grid->Add(cylLabel, 0, wxALIGN_CENTER_VERTICAL);

    m_CountCylChoiceInline = new wxChoice(m_kinInputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, cylinderOptionsInline);
    m_CountCylChoiceInline->SetSelection(3); // 4 по умолчанию

    m_CountCylChoiceV = new wxChoice(m_kinInputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, cylinderOptionsV);
    m_CountCylChoiceV->SetSelection(1); // 4 по умолчанию
    m_CountCylChoiceV->Hide();

    auto* cylChoiceSizer = new wxBoxSizer(wxHORIZONTAL);
    cylChoiceSizer->Add(m_CountCylChoiceInline, 1, wxEXPAND);
    cylChoiceSizer->Add(m_CountCylChoiceV, 1, wxEXPAND);
    grid->Add(cylChoiceSizer, 1, wxEXPAND);

    ApplyDarkTheme(cylLabel);
    ApplyDarkTheme(m_CountCylChoiceInline);
    ApplyDarkTheme(m_CountCylChoiceV);

    m_CountCylChoice = m_CountCylChoiceInline; // по умолчанию рядный

    // Тактность
    wxArrayString taktOptions;
    taktOptions.Add("2"); taktOptions.Add("4");
    auto* taktLabel = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Тактность:"));
    grid->Add(taktLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_TaktChoice = new wxChoice(m_kinInputPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, taktOptions);
    m_TaktChoice->SetSelection(0); // 2 такта по умолчанию
    grid->Add(m_TaktChoice, 1, wxEXPAND);
    ApplyDarkTheme(taktLabel);
    ApplyDarkTheme(m_TaktChoice);

    // Базовые параметры
    addRow(wxString::FromUTF8("Шаг α, град:"),             m_stepAlphaInput, "1.0", 0.01, 360.0,
    wxString::FromUTF8("Шаг изменения угла поворота коленвала (обычно 1° для плавных графиков)"), false, true);

    addRow(wxString::FromUTF8("Предел α, град:"),          m_endAlphaInput,  "360.0", 0.0, 720.0,
    wxString::FromUTF8("Максимальный угол расчёта (360° для 2-тактных, 720° для 4-тактных)"), false, true);

    addRow(wxString::FromUTF8("Радиус кривошипа r, м:"),   m_radcrankInput,  "0.020", 0.0, 1.0,
    wxString::FromUTF8("Радиус кривошипа = половина хода поршня"), false, true);

    addRow(wxString::FromUTF8("Геометрическая характеристика λ:"), m_lambdaInput, "0.3", 0.0, 1.0,
    wxString::FromUTF8("λ = r/L (отношение радиуса кривошипа к длине шатуна)"), false, true);

    addRow(wxString::FromUTF8("Частота вращения n, об/мин:"), m_nInput, "4800", 0.0, 20000.0,
    wxString::FromUTF8("Частота вращения коленвала в оборотах в минуту"), false, true);

    // Дополнительные параметры
    m_gammaLabel = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Угол развала γ, град:"));
    grid->Add(m_gammaLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_gammaInput = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "60.0");
    grid->Add(m_gammaInput, 1, wxEXPAND);
    ApplyDarkTheme(m_gammaLabel); ApplyDarkTheme(m_gammaInput);
    m_gammaInput->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
    ValidateField(m_gammaInput, 0.0, 180.0, true, false);
    UpdateCalculateButtonState();
});
m_gammaInput->SetToolTip(wxString::FromUTF8("Угол между осями цилиндров V-образного двигателя (обычно 60° или 90°)"));

    m_dezaxLabel = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Дезаксиал e, м:"));
    grid->Add(m_dezaxLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_dezaxInput = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.0");
    grid->Add(m_dezaxInput, 1, wxEXPAND);
    ApplyDarkTheme(m_dezaxLabel); ApplyDarkTheme(m_dezaxInput);
    m_dezaxInput->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
    ValidateField(m_dezaxInput, 0.0, 1.0, true, false);
    UpdateCalculateButtonState();
});
m_dezaxInput->SetToolTip(wxString::FromUTF8("Смещение оси цилиндра относительно оси коленвала (может быть 0 для аксиального КШМ)"));

    m_gammaPricLabel = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Угол прицепного шатуна γp, град:"));
    grid->Add(m_gammaPricLabel, 0, wxALIGN_CENTER_VERTICAL);
    m_gammaPricInput = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.0");
    grid->Add(m_gammaPricInput, 1, wxEXPAND);
    ApplyDarkTheme(m_gammaPricLabel); ApplyDarkTheme(m_gammaPricInput);
    m_gammaPricInput->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
    ValidateField(m_gammaPricInput, 0.0, 180.0, true, false);
    UpdateCalculateButtonState();
});
m_gammaPricInput->SetToolTip(wxString::FromUTF8("Угол между главным и прицепным шатунами"));

    m_radcrank1Label = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Радиус кривошипа прицепного r1, м:"));
    grid->Add(m_radcrank1Label, 0, wxALIGN_CENTER_VERTICAL);
    m_radcrank1Input = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.02");
    grid->Add(m_radcrank1Input, 1, wxEXPAND);
    ApplyDarkTheme(m_radcrank1Label); ApplyDarkTheme(m_radcrank1Input);
    m_radcrank1Input->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
    ValidateField(m_radcrank1Input, 0.0, 1.0, true, false);
    UpdateCalculateButtonState();
});
m_radcrank1Input->SetToolTip(wxString::FromUTF8("Радиус кривошипа для прицепного шатуна"));

    m_lengthRod1Label = new wxStaticText(m_kinInputPanel, wxID_ANY, wxString::FromUTF8("Длина прицепного шатуна L1, м:"));
    grid->Add(m_lengthRod1Label, 0, wxALIGN_CENTER_VERTICAL);
    m_lengthRod1Input = new wxTextCtrl(m_kinInputPanel, wxID_ANY, "0.10");
    grid->Add(m_lengthRod1Input, 1, wxEXPAND);
    ApplyDarkTheme(m_lengthRod1Label); ApplyDarkTheme(m_lengthRod1Input);
    m_lengthRod1Input->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
    ValidateField(m_lengthRod1Input, 0.0, 1.0, true, false);
    UpdateCalculateButtonState();
});
m_lengthRod1Input->SetToolTip(wxString::FromUTF8("Длина прицепного шатуна"));


    auto* inputSizer = new wxBoxSizer(wxVERTICAL);
    inputSizer->Add(grid, 0, wxALL | wxEXPAND, 20);

    m_calcButton = new wxButton(m_kinInputPanel, ID_CalcButton, wxString::FromUTF8("РАССЧИТАТЬ"));
    ApplyDarkTheme(m_calcButton);
    inputSizer->Add(m_calcButton, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 20);
    m_kinInputPanel->SetSizer(inputSizer);

    auto* sessionSizer = new wxBoxSizer(wxHORIZONTAL);
m_saveSessionBtn = new wxButton(m_kinInputPanel, ID_SaveSessionBtn, wxString::FromUTF8("Сохранить сессию"));
m_loadSessionBtn = new wxButton(m_kinInputPanel, ID_LoadSessionBtn, wxString::FromUTF8("Загрузить сессию"));
ApplyDarkTheme(m_saveSessionBtn);
ApplyDarkTheme(m_loadSessionBtn);
sessionSizer->Add(m_saveSessionBtn, 0, wxRIGHT, 10);
sessionSizer->Add(m_loadSessionBtn, 0);
inputSizer->Add(sessionSizer, 0, wxALIGN_LEFT | wxTOP, 10);

    // ========== СТРАНИЦА РЕЗУЛЬТАТА ==========
    m_kinResultPanel = new wxPanel(m_kinematicBook);
    ApplyDarkTheme(m_kinResultPanel);
    auto* resSizer = new wxBoxSizer(wxVERTICAL);

    m_resultStatus = new wxStaticText(m_kinResultPanel, wxID_ANY, wxString::FromUTF8("Расчёт ещё не выполнялся."));
    ApplyDarkTheme(m_resultStatus);
    resSizer->Add(m_resultStatus, 0, wxALL, 10);

    // Книжка вкладок "График" / "Таблица"
    m_resultBook = new wxNotebook(m_kinResultPanel, wxID_ANY);
    ApplyDarkTheme(m_resultBook);
    m_resultBook->SetBackgroundColour(wxColour(0x25, 0x2A, 0x30));

    // --- ВКЛАДКА "ГРАФИК" ---
    auto* graphPage = new wxPanel(m_resultBook);
    ApplyDarkTheme(graphPage);
    auto* graphSizer = new wxBoxSizer(wxVERTICAL);
    auto* topResSizer = new wxBoxSizer(wxHORIZONTAL);

    // Левая колонка управления
    auto* graphCtrlSizer = new wxBoxSizer(wxVERTICAL);

    // Выбор типа графика
    auto* graphLabel = new wxStaticText(graphPage, wxID_ANY, wxString::FromUTF8("Тип графика:"));
    ApplyDarkTheme(graphLabel);
    graphCtrlSizer->Add(graphLabel, 0, wxBOTTOM, 5);

    wxArrayString graphTypes;
    graphTypes.Add(wxString::FromUTF8("Перемещение (основной)"));
    graphTypes.Add(wxString::FromUTF8("Скорость (основной)"));
    graphTypes.Add(wxString::FromUTF8("Ускорение (основной)"));
    graphTypes.Add(wxString::FromUTF8("Перемещение (боковой)"));
    graphTypes.Add(wxString::FromUTF8("Скорость (боковой)"));
    graphTypes.Add(wxString::FromUTF8("Ускорение (боковой)"));

    m_graphTypeChoice = new wxChoice(graphPage, ID_GraphTypeChoice, wxDefaultPosition, wxSize(180, -1), graphTypes);
    m_graphTypeChoice->SetSelection(0);
    ApplyDarkTheme(m_graphTypeChoice);
    graphCtrlSizer->Add(m_graphTypeChoice, 0, wxBOTTOM, 10);

    // Метка "Отобразить:"
    auto* selectLabel = new wxStaticText(graphPage, wxID_ANY, wxString::FromUTF8("Отобразить:"));
    ApplyDarkTheme(selectLabel);
    graphCtrlSizer->Add(selectLabel, 0, wxBOTTOM, 5);

    // Список цилиндров/рядов
    m_cylinderCheckList = new wxCheckListBox(graphPage, ID_CylinderCheckList,
                                             wxDefaultPosition, wxSize(180, 150));
    ApplyDarkTheme(m_cylinderCheckList);
    graphCtrlSizer->Add(m_cylinderCheckList, 1, wxEXPAND | wxBOTTOM, 10);

    // Чекбокс "Показать боковые цилиндры"
    m_showSideCheck = new wxCheckBox(graphPage, ID_ShowSideCheck,
                                      wxString::FromUTF8("Показать боковые цилиндры"));
    ApplyDarkTheme(m_showSideCheck);
    m_showSideCheck->Enable(false);
    graphCtrlSizer->Add(m_showSideCheck, 0, wxBOTTOM, 10);

    // Кнопки "Выбрать все" и "Очистить всё"
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    m_selectAllBtn = new wxButton(graphPage, ID_SelectAllBtn, wxString::FromUTF8("Выбрать все"),
                                  wxDefaultPosition, wxSize(90, -1));
    m_clearAllBtn = new wxButton(graphPage, ID_ClearAllBtn, wxString::FromUTF8("Очистить всё"),
                                 wxDefaultPosition, wxSize(90, -1));
    ApplyDarkTheme(m_selectAllBtn);
    ApplyDarkTheme(m_clearAllBtn);
    btnSizer->Add(m_selectAllBtn, 1, wxRIGHT, 5);
    btnSizer->Add(m_clearAllBtn, 1);
    graphCtrlSizer->Add(btnSizer, 0, wxEXPAND | wxBOTTOM, 10);

    // Кнопка сброса зума
    m_resetZoomBtn = new wxButton(graphPage, ID_ResetZoom, wxString::FromUTF8("Сброс зума"),
                                  wxDefaultPosition, wxSize(180, -1));
    ApplyDarkTheme(m_resetZoomBtn);
    graphCtrlSizer->Add(m_resetZoomBtn, 0, wxEXPAND | wxBOTTOM, 10);

    // Кнопка сохранения графика
    m_saveGraphBtn = new wxButton(graphPage, ID_SaveGraphPNG, wxString::FromUTF8("Сохранить график как PNG"),
                                  wxDefaultPosition, wxSize(180, -1));
    ApplyDarkTheme(m_saveGraphBtn);
    graphCtrlSizer->Add(m_saveGraphBtn, 0, wxEXPAND | wxBOTTOM, 10);

    // Добавляем левую колонку в основной горизонтальный sizer
    topResSizer->Add(graphCtrlSizer, 0, wxALL | wxALIGN_TOP, 10);

    // Панель графика
    m_plotPanel = new KinematicPlotPanel(graphPage);
    topResSizer->Add(m_plotPanel, 1, wxEXPAND | wxALL, 10);

    graphSizer->Add(topResSizer, 1, wxEXPAND);
    graphPage->SetSizer(graphSizer);

    // --- ВКЛАДКА "ТАБЛИЦА" ---
    auto* tablePage = new wxPanel(m_resultBook);
    ApplyDarkTheme(tablePage);
    auto* tableSizer = new wxBoxSizer(wxVERTICAL);
    m_tablePanel = new KinematicTablePanel(tablePage);
    tableSizer->Add(m_tablePanel, 1, wxEXPAND | wxALL, 5);
    tablePage->SetSizer(tableSizer);

    m_resultBook->AddPage(graphPage, wxString::FromUTF8("График"), true);
    m_resultBook->AddPage(tablePage, wxString::FromUTF8("Таблица"), false);

    resSizer->Add(m_resultBook, 1, wxEXPAND | wxALL, 5);

    // Нижние кнопки
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_backButton = new wxButton(m_kinResultPanel, ID_BackButton, wxString::FromUTF8("← Назад"));
    m_saveCSVButton = new wxButton(m_kinResultPanel, ID_SaveCSVButton, wxString::FromUTF8("\n Сохранить результаты \n в CSV \n"));
    m_saveTXTButton = new wxButton(m_kinResultPanel, ID_SaveTXTButton, wxString::FromUTF8("\n Сохранить результаты \n в TXT \n"));
    ApplyDarkTheme(m_backButton); ApplyDarkTheme(m_saveCSVButton); ApplyDarkTheme(m_saveTXTButton);
    bottomSizer->Add(m_backButton, 0, wxRIGHT, 10);
    bottomSizer->Add(m_saveCSVButton, 0);
    bottomSizer->Add(m_saveTXTButton, 0);
    resSizer->Add(bottomSizer, 0, wxALIGN_LEFT | wxALL, 10);

    m_kinResultPanel->SetSizer(resSizer);

    m_graphTypeChoice->SetToolTip(wxString::FromUTF8("Выберите тип отображаемой зависимости"));
m_cylinderCheckList->SetToolTip(wxString::FromUTF8("Выберите цилиндры или ряды для отображения"));
m_showSideCheck->SetToolTip(wxString::FromUTF8("Показать на графике также боковые цилиндры (для V-образных двигателей)"));
m_selectAllBtn->SetToolTip(wxString::FromUTF8("Выбрать все цилиндры/ряды"));
m_clearAllBtn->SetToolTip(wxString::FromUTF8("Снять выбор со всех цилиндров/рядов"));
m_resetZoomBtn->SetToolTip(wxString::FromUTF8("Сбросить масштаб графика к полному обзору"));
m_saveGraphBtn->SetToolTip(wxString::FromUTF8("Сохранить текущий график как PNG изображение"));

    // Добавляем страницы в основную книжку
    m_kinematicBook->AddPage(m_kinInputPanel, wxString::FromUTF8("Ввод параметров"), true);
    m_kinematicBook->AddPage(m_kinResultPanel, wxString::FromUTF8("Результаты"), false);

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
    case KSMType::Axial: break;
    case KSMType::Deaxial: showDezax = true; break;
    case KSMType::VShaped: showGamma = true; break;
    case KSMType::VShapedDeaxial: showGamma = true; showDezax = true; break;
    case KSMType::VShapedAttached: showGamma = true; showGammaPric = true; showR1 = true; showL1 = true; break;
    case KSMType::VShapedAttachedDeaxial: showGamma = true; showGammaPric = true; showR1 = true; showL1 = true; showDezax = true; break;
    }

    if (m_CountCylChoiceInline && m_CountCylChoiceV)
    {
        int currentValue = 4;
        if (m_CountCylChoice) {
            int currentSel = m_CountCylChoice->GetSelection();
            if (currentSel != wxNOT_FOUND) {
                wxString valStr = m_CountCylChoice->GetString(currentSel);
                long val;
                if (valStr.ToLong(&val)) currentValue = (int)val;
            }
        }

        if (showGamma) {
            m_CountCylChoiceInline->Hide();
            m_CountCylChoiceV->Show();
            m_CountCylChoice = m_CountCylChoiceV;
            if (currentValue % 2 != 0) currentValue = 4;
            if (currentValue < 2) currentValue = 4;
            if (currentValue > 16) currentValue = 16;
            wxString targetStr = wxString::Format("%d", currentValue);
            int newSel = m_CountCylChoiceV->FindString(targetStr);
            if (newSel != wxNOT_FOUND) m_CountCylChoiceV->SetSelection(newSel);
        } else {
            m_CountCylChoiceInline->Show();
            m_CountCylChoiceV->Hide();
            m_CountCylChoice = m_CountCylChoiceInline;
            wxString targetStr = wxString::Format("%d", currentValue);
            int newSel = m_CountCylChoiceInline->FindString(targetStr);
            if (newSel != wxNOT_FOUND) m_CountCylChoiceInline->SetSelection(newSel);
        }
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

    if (m_graphTypeChoice) {
        int currentSel = m_graphTypeChoice->GetSelection();
        m_graphTypeChoice->Clear();
        if (m_currentType == KSMType::Axial || m_currentType == KSMType::Deaxial) {
            m_graphTypeChoice->Append(wxString::FromUTF8("Перемещение (основной)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Скорость (основной)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Ускорение (основной)"));
            if (currentSel >= 3) currentSel = 0;
        } else {
            m_graphTypeChoice->Append(wxString::FromUTF8("Перемещение (основной)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Скорость (основной)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Ускорение (основной)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Перемещение (боковой)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Скорость (боковой)"));
            m_graphTypeChoice->Append(wxString::FromUTF8("Ускорение (боковой)"));
        }
        if (currentSel >= 0 && currentSel < (int)m_graphTypeChoice->GetCount())
            m_graphTypeChoice->SetSelection(currentSel);
        else
            m_graphTypeChoice->SetSelection(0);
    }

    m_kinInputPanel->Layout();
    if (m_kinResultPanel) m_kinResultPanel->Layout();
    UpdateCalculateButtonState();
}

void MainFrame::UpdateCylinderCheckList()
{
    if (!m_cylinderCheckList) return;

    m_cylinderCheckList->Clear();

    bool isVType = (m_lastParams.gamma != 0.0 || m_lastParams.gammaPric != 0.0);
    int count = isVType ? static_cast<int>(m_lastParams.countCyl) / 2 : static_cast<int>(m_lastParams.countCyl);

    wxString prefix = isVType ? wxString::FromUTF8("Ряд ") : wxString::FromUTF8("Цилиндр ");
    for (int i = 0; i < count; ++i) {
        wxString label = prefix + wxString::Format("%d", i + 1);
        m_cylinderCheckList->Append(label);
    }

    if (count > 0) m_cylinderCheckList->Check(0);

    m_showSideCheck->Enable(isVType);
    if (!isVType) m_showSideCheck->SetValue(false);

    std::vector<int> selected;
    for (int i = 0; i < count; ++i) {
        if (m_cylinderCheckList->IsChecked(i))
            selected.push_back(i);
    }
    m_plotPanel->SetSelectedIndices(selected);
    m_plotPanel->SetShowSide(m_showSideCheck->IsChecked());
}

bool MainFrame::ReadParamsFromUI(EngineParams& p, wxString& err)
{
    double stepA, endA, r, lam, n;
    double gamma = 0.0, dez = 0.0, gammaP = 0.0, r1 = 0.0, L1 = 0.0;

    auto parse = [&](wxTextCtrl* ctrl, double& out) -> bool {
        wxString s = ctrl->GetValue();
        return s.ToDouble(&out);
    };

    int cylinderIndex = m_CountCylChoice->GetSelection();
    if (cylinderIndex == wxNOT_FOUND) {
        p.countCyl = 4;
    } else {
        wxString cylStr = m_CountCylChoice->GetString(cylinderIndex);
        long cylValue;
        if (cylStr.ToLong(&cylValue)) {
            p.countCyl = (int)cylValue;
        } else {
            p.countCyl = 4;
        }
    }

    int taktCount = m_TaktChoice->GetSelection();
    if (taktCount == wxNOT_FOUND) taktCount = 0;

    if (!parse(m_stepAlphaInput, stepA) || !parse(m_endAlphaInput, endA) ||
        !parse(m_radcrankInput, r) || !parse(m_lambdaInput, lam) || !parse(m_nInput, n))
    {
        err = wxString::FromUTF8("Некоторые базовые параметры введены неверно.");
        return false;
    }

    if (stepA <= 0 || stepA > 360 || endA <= 0 || endA > 720 || r <= 0 || lam <= 0 || lam >= 1 || n <= 0)
    {
        err = wxString::FromUTF8("Проверьте шаг/предел α, радиус, λ и n.");
        return false;
    }

    if (m_gammaInput->IsShown() && !parse(m_gammaInput, gamma)) { err = wxString::FromUTF8("Неверное значение угла γ."); return false; }
    if (m_dezaxInput->IsShown() && !parse(m_dezaxInput, dez)) { err = wxString::FromUTF8("Неверное значение дезаксиала e."); return false; }
    if (m_gammaPricInput->IsShown() && !parse(m_gammaPricInput, gammaP)) { err = wxString::FromUTF8("Неверное значение угла γp."); return false; }
    if (m_radcrank1Input->IsShown() && !parse(m_radcrank1Input, r1)) { err = wxString::FromUTF8("Неверное значение r1."); return false; }
    if (m_lengthRod1Input->IsShown() && !parse(m_lengthRod1Input, L1)) { err = wxString::FromUTF8("Неверное значение длины L1."); return false; }

    p.step_alpha = stepA;
    p.end_alpha  = endA;
    p.radcrank   = r;
    p.lyambda    = lam;
    p.n          = n;
    p.taktnost   = (taktCount == 0) ? 2 : 4;

    switch (m_currentType)
    {
    case KSMType::Axial:
        p.gamma = p.gammaPric = p.dezaxial = p.radcrank1 = p.lengthRod1 = 0.0; break;
    case KSMType::Deaxial:
        p.gamma = p.gammaPric = p.radcrank1 = p.lengthRod1 = 0.0; p.dezaxial = dez; break;
    case KSMType::VShaped:
        p.gamma = gamma; p.gammaPric = p.dezaxial = p.radcrank1 = p.lengthRod1 = 0.0; break;
    case KSMType::VShapedDeaxial:
        p.gamma = gamma; p.dezaxial = dez; p.gammaPric = p.radcrank1 = p.lengthRod1 = 0.0; break;
    case KSMType::VShapedAttached:
        p.gamma = gamma; p.gammaPric = gammaP; p.radcrank1 = r1; p.lengthRod1 = L1; p.dezaxial = 0.0; break;
    case KSMType::VShapedAttachedDeaxial:
        p.gamma = gamma; p.gammaPric = gammaP; p.dezaxial = dez; p.radcrank1 = r1; p.lengthRod1 = L1; break;
    }

    return true;
}

// =====================================================================
//  Обработчики событий
// =====================================================================

void MainFrame::OnResetZoom(wxCommandEvent&)
{
    if (m_plotPanel)
        m_plotPanel->ResetView();
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

void MainFrame::OnKSMTypeChanged(wxCommandEvent&) { UpdateKSMTypeFromChoice(); }

void MainFrame::OnCalculate(wxCommandEvent&)
{
    EngineParams params;
    wxString err;
    if (!ReadParamsFromUI(params, err))
    {
        wxMessageBox(err, wxString::FromUTF8("Ошибка ввода"), wxOK | wxICON_ERROR, this);
        return;
    }

    try
    {
        KinematicCalculator calculator(params);
        wxBusyCursor busy;
        CalculationResults res = calculator.calculateAll();

            m_lastParams = params;
        m_lastResults = std::move(res);
        m_hasResults = true;

        wxString st;
        st << wxString::FromUTF8("Расчёт выполнен успешно.\n")
           << wxString::FromUTF8("Точек: ") << static_cast<unsigned long>(m_lastResults.alpha.size());
        m_resultStatus->SetLabel(st);

        m_plotPanel->SetData(&m_lastResults);
        m_plotPanel->SetParams(&m_lastParams);
        m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementMain);
        m_graphTypeChoice->SetSelection(0);

        m_tablePanel->SetData(&m_lastResults, &m_lastParams);

        UpdateCylinderCheckList();

        m_kinematicBook->SetSelection(1);
    }
    catch (...)
    {
        wxMessageBox(wxString::FromUTF8("Во время расчёта произошла ошибка."),
                     wxString::FromUTF8("Ошибка расчёта"), wxOK | wxICON_ERROR, this);
    }
}

void MainFrame::OnGraphTypeChanged(wxCommandEvent&)
{
    int sel = m_graphTypeChoice->GetSelection();
    switch (sel) {
    case 0: m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementMain); break;
    case 1: m_plotPanel->SetMode(KinematicPlotPanel::Mode::VelocityMain); break;
    case 2: m_plotPanel->SetMode(KinematicPlotPanel::Mode::AccelerationMain); break;
    case 3: m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementSide); break;
    case 4: m_plotPanel->SetMode(KinematicPlotPanel::Mode::VelocitySide); break;
    case 5: m_plotPanel->SetMode(KinematicPlotPanel::Mode::AccelerationSide); break;
    default: m_plotPanel->SetMode(KinematicPlotPanel::Mode::DisplacementMain);
    }
}

void MainFrame::OnCylinderCheckListChanged(wxCommandEvent& evt)
{
    int count = m_cylinderCheckList->GetCount();
    std::vector<int> selected;
    for (int i = 0; i < count; ++i) {
        if (m_cylinderCheckList->IsChecked(i))
            selected.push_back(i);
    }
    m_plotPanel->SetSelectedIndices(selected);
}

void MainFrame::OnShowSideChecked(wxCommandEvent& evt)
{
    m_plotPanel->SetShowSide(m_showSideCheck->IsChecked());
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

void MainFrame::OnBackToInput(wxCommandEvent& evt)
{
    if (m_hasResults)
    {
        int ans = wxMessageBox(wxString::FromUTF8("Сохранить результаты перед возвратом к вводу параметров?"),
                                wxString::FromUTF8("Сохранить"), wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
        if (ans == wxCANCEL) return;
        if (ans == wxYES) OnSaveCsv(evt);
    }
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
    // Проверяем все видимые поля ввода
    bool allValid = true;
    
    // Базовые параметры (всегда видны)
    allValid = allValid && ValidateField(m_stepAlphaInput, 0.0, 360.0, false, true);
    allValid = allValid && ValidateField(m_endAlphaInput, 0.0, 720.0, false, true);
    allValid = allValid && ValidateField(m_radcrankInput, 0.0, 1.0, false, true);
    allValid = allValid && ValidateField(m_lambdaInput, 0.0, 1.0, false, true); // λ ∈ (0,1)
    allValid = allValid && ValidateField(m_nInput, 0.0, 20000.0, false, true);
    
    // Дополнительные, если видны
    if (m_gammaInput->IsShown())
        allValid = allValid && ValidateField(m_gammaInput, 0.0, 180.0, true, false); // γ может быть 0, но неотрицательное
    if (m_dezaxInput->IsShown())
        allValid = allValid && ValidateField(m_dezaxInput, 0.0, 1.0, true, false); // e может быть 0
    if (m_gammaPricInput->IsShown())
        allValid = allValid && ValidateField(m_gammaPricInput, 0.0, 180.0, true, false);
    if (m_radcrank1Input->IsShown())
        allValid = allValid && ValidateField(m_radcrank1Input, 0.0, 1.0, true, false);
    if (m_lengthRod1Input->IsShown())
        allValid = allValid && ValidateField(m_lengthRod1Input, 0.0, 1.0, true, false);
    
    m_calcButton->Enable(allValid);
}
void MainFrame::OnLoadTemplate(wxCommandEvent& evt)
{
    int sel = m_templateChoice->GetSelection();
    if (sel == wxNOT_FOUND) return;

    // Сбрасываем все поля к значениям по умолчанию (опционально)
    // Но проще устанавливать только нужные.

    if (sel == 0) // ВАЗ-2106
    {
        m_ksmTypeChoice->SetSelection(0); // Аксиальный
        UpdateKSMTypeFromChoice();

        m_stepAlphaInput->SetValue("1.0");
        m_endAlphaInput->SetValue("360.0");
        m_radcrankInput->SetValue("0.040"); // ход 80 мм -> r=0.040
        m_lambdaInput->SetValue("0.285");
        m_nInput->SetValue("5600");
        m_TaktChoice->SetSelection(1); // 4 такта

        // Количество цилиндров: 4 (рядный)
        int idx = m_CountCylChoiceInline->FindString("4");
        if (idx != wxNOT_FOUND) m_CountCylChoiceInline->SetSelection(idx);

        // Дополнительные параметры (если они вдруг видны) сбрасываем в 0
        m_gammaInput->SetValue("0.0");
        m_dezaxInput->SetValue("0.0");
        m_gammaPricInput->SetValue("0.0");
        m_radcrank1Input->SetValue("0.0");
        m_lengthRod1Input->SetValue("0.0");
    }
    else if (sel == 1) // ЗМЗ-406
    {
        m_ksmTypeChoice->SetSelection(0); // Аксиальный
        UpdateKSMTypeFromChoice();

        m_stepAlphaInput->SetValue("1.0");
        m_endAlphaInput->SetValue("360.0");
        m_radcrankInput->SetValue("0.043"); // ход 86 мм
        m_lambdaInput->SetValue("0.26");
        m_nInput->SetValue("5200");
        m_TaktChoice->SetSelection(1); // 4 такта

        int idx = m_CountCylChoiceInline->FindString("4");
        if (idx != wxNOT_FOUND) m_CountCylChoiceInline->SetSelection(idx);

        m_gammaInput->SetValue("0.0");
        m_dezaxInput->SetValue("0.0");
        m_gammaPricInput->SetValue("0.0");
        m_radcrank1Input->SetValue("0.0");
        m_lengthRod1Input->SetValue("0.0");
    }
    else if (sel == 2) // V8 5.0 (аксиальный V-образный)
    {
        m_ksmTypeChoice->SetSelection(2); // V-образный (без прицепного)
        UpdateKSMTypeFromChoice();

        m_stepAlphaInput->SetValue("1.0");
        m_endAlphaInput->SetValue("360.0"); // для V-образных обычно полный цикл 720°
        m_radcrankInput->SetValue("0.044"); // ход 88 мм
        m_lambdaInput->SetValue("0.25");
        m_nInput->SetValue("6000");
        m_TaktChoice->SetSelection(1); // 4 такта
        m_gammaInput->SetValue("90.0"); // угол развала

        int idx = m_CountCylChoiceV->FindString("8");
        if (idx != wxNOT_FOUND) m_CountCylChoiceV->SetSelection(idx);

        m_dezaxInput->SetValue("0.0");
        m_gammaPricInput->SetValue("0.0");
        m_radcrank1Input->SetValue("0.0");
        m_lengthRod1Input->SetValue("0.0");
    }

    // Запускаем валидацию всех полей
    UpdateCalculateButtonState();

    // Обновляем отображение полей (возможно, лишнее, но для надёжности)
    m_kinInputPanel->Layout();
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

    // Если есть результаты, записываем их
    if (m_hasResults)
    {
        txt << "[Results]\n";
        // Заголовок CSV (для первого цилиндра, можно расширить позже)
        txt << "alpha,stroke_full,velocity_full,acceleration_full,betta_rod,omega_rod,eps_rod\n";
        for (size_t i = 0; i < m_lastResults.alpha.size(); ++i)
        {
            txt << m_lastResults.alpha[i] << ","
                << m_lastResults.stroke_full[i] << ","
                << m_lastResults.velocity_full[i] << ","
                << m_lastResults.acceleration_full[i] << ","
                << m_lastResults.betta_rod[i] << ","
                << m_lastResults.omega_rod[i] << ","
                << m_lastResults.eps_rod[i] << "\n";
        }
    }

    // Сохраняем состояние GUI (выбранные индексы и т.д.)
    txt << "[GUI]\n";
    txt << "graphMode=" << static_cast<int>(m_plotPanel->GetMode()) << "\n";
    txt << "showSide=" << (m_showSideCheck->IsChecked() ? "1" : "0") << "\n";
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
                        m_graphTypeChoice->SetSelection(mode);
                }
                else if (key == "showSide")
                {
                    m_showSideCheck->SetValue(value == "1");
                }
                else if (key == "selectedIndices")
                {
                    // Парсим список через запятую
                    // Будет обработано позже, когда загрузим данные
                }
            }
        }
    }

    // После чтения, если есть результаты, восстанавливаем их
    if (!alpha.empty())
    {
        // Создаем структуру результатов (для одного цилиндра)
        CalculationResults res;
        res.alpha = alpha;
        res.stroke_full = stroke;
        res.velocity_full = vel;
        res.acceleration_full = acc;
        res.betta_rod = betta;
        res.omega_rod = omega;
        res.eps_rod = eps;

        // Копируем в основные векторы для одного цилиндра (для обратной совместимости)
        res.cylinder_stroke_full.push_back(stroke);
        res.cylinder_velocity_full.push_back(vel);
        res.cylinder_acceleration_full.push_back(acc);
        res.cylinder_betta_rod.push_back(betta);
        res.cylinder_omega_rod.push_back(omega);
        res.cylinder_eps_rod.push_back(eps);

        m_lastResults = std::move(res);
        m_lastParams = params;
        m_hasResults = true;

        // Обновляем GUI
        UpdateCylinderCheckList(); // заполнит список цилиндров (надо будет доработать, чтобы использовать m_lastParams)
        m_plotPanel->SetData(&m_lastResults);
        m_plotPanel->SetParams(&m_lastParams);
        // Восстанавливаем режим графика из сохранённого
        // (уже установлено через m_graphTypeChoice)
        m_plotPanel->SetMode(static_cast<KinematicPlotPanel::Mode>(m_graphTypeChoice->GetSelection()));

        // Восстанавливаем выбранные индексы (если они были сохранены)
        // Нужно доработать парсинг selectedIndices

        // Переключаем на страницу результатов
        m_kinematicBook->SetSelection(1);
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