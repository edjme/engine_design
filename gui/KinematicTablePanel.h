#pragma once

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include "core/common/common_types.h"

class KinematicTablePanel : public wxPanel {
public:
    KinematicTablePanel(wxWindow* parent);
    void SetData(const CalculationResults* results, const EngineParams* params);
    void Clear();

private:
    const CalculationResults* m_results = nullptr;
    const EngineParams* m_params = nullptr;

    wxGrid* m_grid = nullptr;
    wxStaticText* m_cylinderLabel = nullptr;   // метка "Цилиндр:" или "Ряд:"
    wxChoice* m_cylinderChoice = nullptr;
    wxCheckBox* m_showSideCheck = nullptr;
    wxStaticText* m_rowCountText = nullptr;

    void UpdateTable();
    wxString FormatDouble(double value, int precision = 6);

    void OnCylinderChanged(wxCommandEvent& evt);
    void OnShowSideChanged(wxCommandEvent& evt);
    void OnGridCellClick(wxGridEvent& evt);
    void OnGridLabelClick(wxGridEvent& evt);

    wxDECLARE_EVENT_TABLE();
};