#pragma once

#include <wx/wx.h>
#include <wx/notebook.h>

#include "core/common/common_types.h"
#include "core/Kinematic/Calculations/kinematic_formulas.h"
#include "core/Kinematic/output_data/kinematic_output.h"

// Тестовый GUI для расчёта кинематики КШМ
class MainFrame : public wxFrame
{
public:
    explicit MainFrame(const wxString& title);

private:
    // Поля ввода
    wxTextCtrl*   m_stepAlphaInput;   // шаг α
    wxTextCtrl*   m_endAlphaInput;    // предел α
    wxTextCtrl*   m_radcrankInput;    // радиус кривошипа
    wxTextCtrl*   m_lambdaInput;      // λ
    wxTextCtrl*   m_nInput;           // n, об/мин

    // Вывод статуса/результатов
    wxTextCtrl*   m_resultBox;

    // Последний расчёт (для сохранения)
    EngineParams       m_lastParams;
    CalculationResults m_lastResults;
    bool               m_hasResults = false;

    // Обработчики
    void OnCalculate(wxCommandEvent& event);
    void OnSaveCsv(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};
