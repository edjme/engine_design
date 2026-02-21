#pragma once

#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>          // для wxCheckListBox

#include "core/common/common_types.h"
#include "core/Kinematic/Calculations/kinematic_formulas.h"
#include "core/Kinematic/output_data/kinematic_output.h"
#include "KinematicPlotPanel.h"
#include "KinematicTablePanel.h"
#include "core/Kinematic/Calculations/models/KSMModel.h"



class MainFrame : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);

private:
    // --- общие данные расчёта ---
    EngineParams       m_lastParams{};          // последние использованные параметры
    CalculationResults m_lastResults{};         // последние результаты расчёта
    bool               m_hasResults = false;    // флаг наличия результатов
    KSMType            m_currentType = KSMType::Axial; // текущий тип КШМ

    // --- левая панель (меню) ---
    wxPanel*  m_sidebarPanel   = nullptr;
    wxButton* m_btnKinematic   = nullptr;       // кнопка перехода к кинематике
    wxButton* m_btnDynamic     = nullptr;       // кнопка перехода к динамике (заглушка)

    // --- правая колонка: книжка разделов ---
    wxSimplebook* m_rightBook = nullptr;        // 0 - кинематика, 1 - динамика

    // --- раздел "Кинематика": книжка (ввод / результат) ---
    wxSimplebook* m_kinematicBook = nullptr;    // 0 - ввод, 1 - результат

    // страница ввода
    wxPanel*     m_kinInputPanel  = nullptr;
    wxChoice*    m_ksmTypeChoice  = nullptr;    // выбор типа КШМ

    wxChoice*    m_CountCylChoice = nullptr;    // текущий активный выбор количества цилиндров
    wxChoice*    m_TaktChoice     = nullptr;    // тактность (2 или 4)
    wxTextCtrl*  m_stepAlphaInput = nullptr;    // шаг угла α
    wxTextCtrl*  m_endAlphaInput  = nullptr;    // конечный угол α
    wxTextCtrl*  m_radcrankInput  = nullptr;    // радиус кривошипа r
    wxTextCtrl*  m_lambdaInput    = nullptr;    // геометрическая характеристика λ
    wxTextCtrl*  m_nInput         = nullptr;    // частота вращения n

    // два варианта выбора количества цилиндров (рядные / V-образные)
    wxChoice* m_CountCylChoiceInline;            // для рядных (1,2,3,4,5,6,8,10,12,16)
    wxChoice* m_CountCylChoiceV;                 // для V-образных (2,4,6,8,10,12,16)

    // дополнительные параметры (зависят от типа КШМ)
    wxStaticText* m_gammaLabel     = nullptr;    // метка "Угол развала γ"
    wxTextCtrl*   m_gammaInput     = nullptr;    // поле ввода γ
    wxStaticText* m_dezaxLabel     = nullptr;    // метка "Дезаксиал e"
    wxTextCtrl*   m_dezaxInput     = nullptr;    // поле ввода e
    wxStaticText* m_gammaPricLabel = nullptr;    // метка "Угол прицепного шатуна γp"
    wxTextCtrl*   m_gammaPricInput = nullptr;    // поле ввода γp
    wxStaticText* m_radcrank1Label = nullptr;    // метка "Радиус кривошипа прицепного r1"
    wxTextCtrl*   m_radcrank1Input = nullptr;    // поле ввода r1
    wxStaticText* m_lengthRod1Label = nullptr;   // метка "Длина прицепного шатуна L1"
    wxTextCtrl*   m_lengthRod1Input = nullptr;   // поле ввода L1

    wxButton*    m_calcButton     = nullptr;     // кнопка "Рассчитать"

    // страница результата
    wxPanel*            m_kinResultPanel       = nullptr;
    wxStaticText*       m_resultStatus         = nullptr;   // строка статуса после расчёта
    wxChoice*           m_graphTypeChoice      = nullptr;   // выбор типа графика
    KinematicPlotPanel* m_plotPanel            = nullptr;   // панель отображения графиков
    KinematicTablePanel* m_tablePanel          = nullptr;   // панель таблицы данных
    wxButton*           m_backButton           = nullptr;   // кнопка "Назад" к вводу
    wxButton*           m_saveCSVButton        = nullptr;   // сохранение в CSV
    wxButton*           m_saveTXTButton        = nullptr;   // сохранение в TXT
    wxNotebook*         m_resultBook           = nullptr;   // книжка вкладок (График / Таблица)
    wxButton* m_resetZoomBtn;

    // --- новые элементы управления графиком (множественный выбор) ---
    wxCheckListBox* m_cylinderCheckList = nullptr;   // список с флажками для выбора цилиндров/рядов
    wxCheckBox*     m_showSideCheck     = nullptr;   // чекбокс "Показать боковые цилиндры"
    wxButton*       m_selectAllBtn      = nullptr;   // кнопка "Выбрать все"
    wxButton*       m_clearAllBtn       = nullptr;   // кнопка "Очистить всё"
    wxButton* m_saveGraphBtn = nullptr;

    // --- построение интерфейса ---
    void BuildLayout();
    void BuildSidebar(wxPanel* parent);
    void BuildKinematicPages(wxPanel* parent);

    void ApplyDarkTheme(wxWindow* w);                // рекурсивное применение тёмной темы

    void UpdateKSMTypeFromChoice();                  // обновить тип КШМ при изменении выбора
    void UpdateParameterVisibility();                 // показать/скрыть дополнительные параметры
    bool ReadParamsFromUI(EngineParams& outParams, wxString& errorMessage); // чтение параметров из полей

    // --- новые вспомогательные методы ---
    void UpdateCylinderCheckList();                   // заполнить список цилиндров/рядов после расчёта
    
    void OnResetZoom(wxCommandEvent& evt);
    
    // --- обработчики событий ---
    void OnSidebarKinematic(wxCommandEvent& evt);
    void OnSidebarDynamic(wxCommandEvent& evt);
    void OnKSMTypeChanged(wxCommandEvent& evt);
    void OnCalculate(wxCommandEvent& evt);
    void OnGraphTypeChanged(wxCommandEvent& evt);
    void OnBackToInput(wxCommandEvent& evt);
    void OnSaveCsv(wxCommandEvent& evt);
    void OnSaveTxt(wxCommandEvent& evt);
    void OnSaveGraphPNG(wxCommandEvent& evt);

    // --- новые обработчики для управления графиками ---
    void OnCylinderCheckListChanged(wxCommandEvent& evt);   // изменение отмеченных элементов
    void OnShowSideChecked(wxCommandEvent& evt);            // изменение чекбокса "Показать боковые"
    void OnSelectAll(wxCommandEvent& evt);                  // кнопка "Выбрать все"
    void OnClearAll(wxCommandEvent& evt);                   // кнопка "Очистить всё"

    // Валидация конкретного поля
    bool ValidateField(wxTextCtrl* field, double minVal, double maxVal, 
                       bool allowZero = false, bool positiveOnly = true);
    
    // Проверка всех видимых полей и обновление состояния кнопки "Рассчитать"
    void UpdateCalculateButtonState();

    wxChoice* m_templateChoice;
    wxButton* m_loadTemplateBtn;

void OnLoadTemplate(wxCommandEvent& evt);

wxButton* m_saveSessionBtn;
wxButton* m_loadSessionBtn;

void OnSaveSession(wxCommandEvent& evt);
void OnLoadSession(wxCommandEvent& evt);

// Сериализация
bool SaveSessionToFile(const wxString& filename);
bool LoadSessionFromFile(const wxString& filename);


// Параметры динамики
std::vector<double> m_dynPressureAngles;
std::vector<double> m_dynPressureValues;
wxString m_dynPressureFile;

// Элементы управления динамикой
wxChoice* m_dynUnitChoice;
wxButton* m_dynLoadBtn;
wxStaticText* m_dynFileLabel;
KinematicPlotPanel* m_dynPreviewPlot;
wxButton* m_dynCalcBtn;

wxTextCtrl* m_massPistonInput;
wxTextCtrl* m_massRodInput;
wxTextCtrl* m_kRodOscInput;

// Методы
void BuildDynamicPages(wxPanel* parent);
void OnLoadPressure(wxCommandEvent& evt);
void OnCalculateDynamic(wxCommandEvent& evt);

DynamicResults m_dynResults;

wxSimplebook* m_dynamicBook;
wxPanel* m_dynInputPage;
wxPanel* m_dynResultPage;

void BuildDynamicInputPage(wxPanel* parent);
void BuildDynamicResultPage(wxPanel* parent);

KinematicPlotPanel* m_dynPlotResult;

wxNotebook* m_dynResultBook;
wxPanel* m_dynGraphPage;
wxPanel* m_dynTablePage;
wxGrid* m_dynGrid;
wxChoice* m_dynGraphTypeChoice;

void OnDynGraphTypeChanged(wxCommandEvent& evt);

wxTextCtrl* m_dynBoreInput;
void OnDynamicBackToInput(wxCommandEvent& evt);



wxDECLARE_EVENT_TABLE();
};