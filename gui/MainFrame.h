#pragma once

#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/checkbox.h>

#include "core/common/common_types.h"
#include "core/Kinematic/Calculations/kinematic_formulas.h"
#include "core/Kinematic/output_data/kinematic_output.h"
#include "KinematicPlotPanel.h"

// Типы КШМ
enum class KSMType
{
    Axial = 0,
    Deaxial,
    VShaped,
    VShapedDeaxial,
    VShapedAttached,
    VShapedAttachedDeaxial
};

class MainFrame : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);

private:
    // --- общие данные расчёта ---
    EngineParams       m_lastParams{};
    CalculationResults m_lastResults{};
    bool               m_hasResults = false;
    KSMType            m_currentType = KSMType::Axial;

    // --- левая панель (меню) ---
    wxPanel*  m_sidebarPanel   = nullptr;
    wxButton* m_btnKinematic   = nullptr;
    wxButton* m_btnDynamic     = nullptr;

    // --- правая колонка: книжка разделов ---
    wxSimplebook* m_rightBook = nullptr; // 0 - кинематика, 1 - динамика

    // --- раздел "Кинематика": книжка (ввод / результат) ---
    wxSimplebook* m_kinematicBook = nullptr; // 0 - ввод, 1 - результат

    // страница ввода
    wxPanel*     m_kinInputPanel  = nullptr;
    wxChoice*    m_ksmTypeChoice  = nullptr;

    wxChoice*    m_CountCylChoice = nullptr;
    wxChoice*    m_TaktChoice     = nullptr;
    wxTextCtrl*  m_stepAlphaInput = nullptr;
    wxTextCtrl*  m_endAlphaInput  = nullptr;
    wxTextCtrl*  m_radcrankInput  = nullptr;
    wxTextCtrl*  m_lambdaInput    = nullptr;
    wxTextCtrl*  m_nInput         = nullptr;
    wxChoice* m_CountCylChoiceInline;  // Для рядных двигателей
wxChoice* m_CountCylChoiceV;       // Для V-образных

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
    wxChoice*           m_cylinderChoice       = nullptr;  // Выбор цилиндра для отображения
    wxCheckBox*         m_showAllCylindersCheck = nullptr; // Чекбокс "Показать все цилиндры"
    KinematicPlotPanel* m_plotPanel            = nullptr;
    wxButton*           m_backButton           = nullptr;
    wxButton*           m_saveCSVButton        = nullptr;
    wxButton*           m_saveTXTButton        = nullptr;
    wxStaticText* m_cylinderLabel = nullptr;  // Метка "Цилиндр:" или "Ряд:"

    // --- построение интерфейса ---
    void BuildLayout();
    void BuildSidebar(wxPanel* parent);
    void BuildKinematicPages(wxPanel* parent);

    void ApplyDarkTheme(wxWindow* w);

    void UpdateKSMTypeFromChoice();
    void UpdateParameterVisibility();
    bool ReadParamsFromUI(EngineParams& outParams, wxString& errorMessage);

    // --- обработчики событий ---
    void OnSidebarKinematic(wxCommandEvent& evt);
    void OnSidebarDynamic(wxCommandEvent& evt);

    void OnKSMTypeChanged(wxCommandEvent& evt);
    void OnCalculate(wxCommandEvent& evt);
    void OnGraphTypeChanged(wxCommandEvent& evt);
    void OnCylinderChanged(wxCommandEvent& evt);        // Обработчик выбора цилиндра
    void OnShowAllCylindersChanged(wxCommandEvent& evt); // Обработчик чекбокса "Показать все"
    void OnBackToInput(wxCommandEvent& evt);
    void OnSaveCsv(wxCommandEvent& evt);
    void OnSaveTxt(wxCommandEvent& evt);

    wxDECLARE_EVENT_TABLE();
};