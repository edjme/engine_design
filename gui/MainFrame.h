#pragma once

#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/grid.h>
#include <wx/spinctrl.h>

#include "core/common/common_types.h"
#include "core/Kinematic/output_data/kinematic_output.h"
#include "KinematicPlotPanel.h"
#include "KinematicTablePanel.h"
#include "core/Kinematic/Calculations/models/KSMModel.h"
#include "core/Dynamic/output_data/dynamic_output.h"
class LayoutPanel;
class CrankConfigPanel;

// Предварительные объявления классов потоков (определены в .cpp)
class KinematicThread;
class DynamicThread;

// Объявления пользовательских событий (определены в .cpp)
wxDECLARE_EVENT(wxEVT_KINEMATIC_COMPLETE, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_DYNAMIC_COMPLETE, wxThreadEvent);

class MainFrame : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);

private:
    // --- общие данные расчёта ---
    EngineParams       m_lastParams{};          // последние использованные параметры
    CalculationResults m_lastResults{};         // последние результаты расчёта
    bool               m_hasResults = false;        // есть результаты кинематики
    bool               m_hasDynamicResults = false; // есть результаты динамики
    KSMType            m_currentType = KSMType::Axial;
    // Порядок работы / фазы цилиндров (задаются через диалог)
    std::vector<double> m_customCylPhasesDeg; // size = countCyl, или пусто
    int m_layoutRows = 1;
    int m_layoutSections = 1;

    // --- левая панель (меню) ---
    wxPanel*  m_sidebarPanel   = nullptr;

    // Мастер-шаги слева (маршрут использования)
    wxButton*     m_btnStep1Params   = nullptr;
    wxStaticText* m_step1Status      = nullptr;

    wxButton*     m_btnStep2Pressure = nullptr;
    wxStaticText* m_step2Status      = nullptr;

    wxButton*     m_btnStep3Results  = nullptr;
    wxStaticText* m_step3Status      = nullptr;

    // --- правая колонка: книжка разделов ---
    wxSimplebook* m_rightBook = nullptr;

    // --- раздел "Кинематика": книжка (ввод / результат) ---
    wxSimplebook* m_kinematicBook = nullptr;

    // страница ввода
    wxPanel*     m_kinInputPanel  = nullptr;
    wxChoice*    m_ksmTypeChoice  = nullptr;
    wxChoice*    m_CountCylChoice = nullptr;
    wxChoice*    m_TaktChoice     = nullptr;
    wxTextCtrl*  m_stepAlphaInput = nullptr;
    wxTextCtrl*  m_radcrankInput  = nullptr;
    wxTextCtrl*  m_lambdaInput    = nullptr;
    wxTextCtrl*  m_nInput         = nullptr;
    wxStaticText* m_ksmTypeText = nullptr;

    wxChoice* m_CountCylChoiceInline;
    wxChoice* m_CountCylChoiceV;

    wxStaticText* m_gammaLabel     = nullptr;
    wxTextCtrl*   m_gammaInput     = nullptr;
    wxStaticText* m_dezaxLabel     = nullptr;
    wxTextCtrl*   m_dezaxInput     = nullptr;
    wxStaticText* m_gammaPricLabel = nullptr;
    wxTextCtrl*   m_gammaPricInput = nullptr;
    wxStaticText* m_radcrank1Label = nullptr;
    wxTextCtrl*   m_radcrank1Input = nullptr;
    wxStaticText* m_lengthRod1Label = nullptr;
    wxTextCtrl*   m_lengthRod1Input = nullptr;

    wxButton*    m_calcButton     = nullptr;

    // страница результата
    wxPanel*            m_kinResultPanel       = nullptr;
    wxStaticText*       m_resultStatus         = nullptr;
    wxChoice*           m_graphTypeChoice      = nullptr;
    KinematicPlotPanel* m_plotPanel            = nullptr;
    KinematicTablePanel* m_tablePanel          = nullptr;
    wxButton*           m_backButton           = nullptr;
    wxButton*           m_saveCSVButton        = nullptr;
    wxButton*           m_saveTXTButton        = nullptr;
    wxNotebook*         m_resultBook           = nullptr;
    wxButton* m_resetZoomBtn;

    wxCheckListBox* m_cylinderCheckList = nullptr;
    wxButton*       m_selectAllBtn      = nullptr;
    wxButton*       m_clearAllBtn       = nullptr;
    wxButton* m_saveGraphBtn = nullptr;

    // --- раздел "Динамика" ---
    wxSimplebook* m_dynamicBook = nullptr;
    wxPanel* m_dynInputPage = nullptr;
    wxPanel* m_dynResultPage = nullptr;

    std::vector<double> m_dynPressureAngles;
    std::vector<double> m_dynPressureValues;
    wxString m_dynPressureFile;

    wxChoice* m_dynUnitChoice = nullptr ;
    wxButton* m_dynLoadBtn = nullptr ;
    wxStaticText* m_dynFileLabel;
    KinematicPlotPanel* m_dynPreviewPlot;
    wxButton* m_dynCalcBtn = nullptr;

    wxTextCtrl* m_massPistonInput = nullptr;
    wxTextCtrl* m_massRodInput = nullptr;
    wxTextCtrl* m_kRodOscInput = nullptr;
    wxTextCtrl* m_dynBoreInput = nullptr;

    DynamicResults m_dynResults;

    wxNotebook* m_dynResultBook;
    wxPanel* m_dynGraphPage = nullptr;
    wxPanel* m_dynTablePage = nullptr;
    wxGrid* m_dynGrid;
    wxChoice* m_dynGraphTypeChoice = nullptr;
    wxCheckListBox* m_dynCylinderCheckList;
    KinematicPlotPanel* m_dynPlotResult;
    wxStaticText* m_dynCalcStatus = nullptr;

    // --- шаблоны и сессии ---
    wxChoice* m_templateChoice = nullptr;
    wxButton* m_saveSessionBtn = nullptr;
    wxButton* m_loadSessionBtn = nullptr;

    wxButton* m_sessionMenuBtn = nullptr;     // кинематика: Сессия ▾
    wxButton* m_exportKinBtn   = nullptr;     // кинематика-результаты: Экспорт ▾
    wxButton* m_exportDynBtn   = nullptr;     // динамика-результаты: Экспорт ▾

    wxButton* m_goToDynamicBtn = nullptr; // на странице результатов кинематики

    wxStaticText* m_dynLoadStatus = nullptr; // статус: загружено/ошибка/точек/диапазон

    LayoutPanel* m_layoutPanelHost = nullptr; // контейнер компоновки на странице ввода

    CrankConfigPanel* m_crankPanel = nullptr;

    wxWindow* m_ignitionPanelHost = nullptr;   // LayoutPanel для углов вспышки на странице давления
    wxTextCtrl* m_tdcTolInput = nullptr;       // допуск ВМТ, град (1.0 по умолчанию)

    wxPanel* m_dynPressurePage = nullptr;
wxPanel* m_dynIgnitionPage = nullptr;

wxButton* m_dynNextBtn = nullptr;
wxButton* m_dynPrevBtn = nullptr;

    // --- методы ---
    void BuildLayout();
    void BuildSidebar(wxPanel* parent);
    void UpdateSidebarSteps();
    void BuildKinematicPages(wxPanel* parent);
    void BuildDynamicPages(wxPanel* parent);
    void BuildDynamicInputPage(wxPanel* parent);
    void BuildDynamicResultPage(wxPanel* parent);

    void ApplyDarkTheme(wxWindow* w);
    void UpdateKSMTypeFromChoice();
    void UpdateParameterVisibility();
    bool ReadParamsFromUI(EngineParams& outParams, wxString& errorMessage);
    void UpdateCylinderCheckList();
    bool ValidateField(wxTextCtrl* field, double minVal, double maxVal,
                       bool allowZero = false, bool positiveOnly = true);
    void UpdateCalculateButtonState();

    void UpdateDynamicGraph();
    void UpdateDynamicTable();
    DynamicExportInfo BuildDynExportInfo() const;

    // --- обработчики событий ---
    void OnSidebarKinematic(wxCommandEvent& evt);      // Шаг 1
    void OnSidebarStep2Pressure(wxCommandEvent& evt);  // Шаг 2
    void OnSidebarStep3Results(wxCommandEvent& evt);   // Шаг 3
    void OnKSMTypeChanged(wxCommandEvent& evt);
    void OnCalculate(wxCommandEvent& evt);
    void OnGraphTypeChanged(wxCommandEvent& evt);
    void OnBackToInput(wxCommandEvent& evt);
    void OnSaveCsv(wxCommandEvent& evt);
    void OnSaveTxt(wxCommandEvent& evt);
    void OnSaveGraphPNG(wxCommandEvent& evt);
    void OnResetZoom(wxCommandEvent& evt);

    void OnCylinderCheckListChanged(wxCommandEvent& evt);
    void OnSelectAll(wxCommandEvent& evt);
    void OnClearAll(wxCommandEvent& evt);

    void OnSaveSession(wxCommandEvent& evt);
    void OnLoadSession(wxCommandEvent& evt);
    bool SaveSessionToFile(const wxString& filename);
    bool LoadSessionFromFile(const wxString& filename);

    void OnLoadPressure(wxCommandEvent& evt);
    void OnCalculateDynamic(wxCommandEvent& evt);
    void OnDynGraphTypeChanged(wxCommandEvent& evt);
    void OnDynamicBackToInput(wxCommandEvent& evt);
    void OnDynCylinderCheckListChanged(wxCommandEvent& evt);
    void OnDynSelectAllDynCylinders(wxCommandEvent& evt);
    void OnDynClearAllDynCylinders(wxCommandEvent& evt);
    void OnDynSaveCsvWide(wxCommandEvent& evt);
    void OnDynSaveCsvLong(wxCommandEvent& evt);
    void OnDynSaveTxt(wxCommandEvent& evt);
    void OnGoToDynamic(wxCommandEvent& evt);

    // --- обработчики событий завершения потоков ---
    void OnKinematicComplete(wxThreadEvent& evt);
    void OnDynamicComplete(wxThreadEvent& evt);

    void UpdateDynamicControlsState();

    void OnSessionMenu(wxCommandEvent& evt);
    void OnExportKinematicMenu(wxCommandEvent& evt);
    void OnExportDynamicMenu(wxCommandEvent& evt);

    void UpdateKSMTypeText();

    void OnDynNext(wxCommandEvent&);
    void OnDynPrev(wxCommandEvent&);

    void BuildDynamicPressurePage(wxPanel* parent);
void BuildDynamicIgnitionPage(wxPanel* parent);

    bool m_kinCalcInProgress = false;

    wxDECLARE_EVENT_TABLE();
};