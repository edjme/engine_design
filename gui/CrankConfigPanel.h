#pragma once

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/sizer.h> 

#include <vector>

enum class RodPairType
{
    SideBySide,     // рядом сидящие
    Articulated     // прицепной
};

struct CrankConfig
{
    int  taktnost = 4;                 // 2 или 4
    int  crank_count = 4;              // число кривошипов (колен)
    int  cyl_per_crankpin = 1;         // 1 или 2
    RodPairType rod_pair = RodPairType::SideBySide;

    // Фазы кривошипов относительно 1-го (мех. система, 0..360)
    std::vector<double> crank_phase_deg {0, 90, 180, 270};

    // Геометрия (активируется по необходимости)
    double gamma_deg = 0.0;            // угол развала (когда 2 цилиндра на шейку)
    double dezaxial_m = 0.0;           // дезаксиал

    // Прицепной шатун
    double gamma_pric_deg = 0.0;
    double radcrank1_m = 0.0;
    double lengthRod1_m = 0.0;

    // На будущее (пока просто хранить)
    bool full_support_bearings = true; // полноопорный / неполноопорный

    double step_alpha_deg = 1.0;   // шаг α
    double r_m = 0.02;             // радиус кривошипа
    double lambda = 0.3;           // λ = r/L
    double n_rpm = 4800.0;         // обороты
};

class CrankConfigPanel : public wxPanel
{
public:
    explicit CrankConfigPanel(wxWindow* parent);

    bool GetConfig(CrankConfig& out, wxString& err) const;
    void SetConfig(const CrankConfig& cfg);

private:
    static void NormalizePhases(std::vector<double>& a, double cycle);

    void RebuildPhaseGrid(int crankCount);
    void FillDefaultPhases();
    bool ReadPhaseGrid(std::vector<double>& out, wxString& err) const;

    void UpdateVisibility();

private:
    wxChoice*   m_taktChoice = nullptr;
    wxSpinCtrl* m_crankCount = nullptr;
    wxChoice*   m_cylPerPin  = nullptr;

    wxStaticText*  m_phaseLbl = nullptr;
    wxGrid*        m_phaseGrid = nullptr;
    wxStaticText*  m_hint = nullptr;

    wxStaticBoxSizer* m_rodBox = nullptr;
    wxRadioButton*    m_rodSideBySide = nullptr;
    wxRadioButton*    m_rodArticulated = nullptr;

    wxRadioButton* m_fullSupport = nullptr;
    wxRadioButton* m_semiSupport = nullptr;

    wxTextCtrl* m_gamma = nullptr;
    wxTextCtrl* m_dezaxial = nullptr;
    wxTextCtrl* m_stepAlpha = nullptr;
    wxTextCtrl* m_n = nullptr;
    wxTextCtrl* m_lambda = nullptr;
    wxTextCtrl* m_r = nullptr;


    wxStaticBoxSizer* m_attachedBox = nullptr;
    wxTextCtrl* m_gammaPric = nullptr;
    wxTextCtrl* m_r1 = nullptr;
    wxTextCtrl* m_L1 = nullptr;

    wxTextCtrl* m_stepAlphaCtrl = nullptr;
    wxTextCtrl* m_rCtrl         = nullptr;
    wxTextCtrl* m_lambdaCtrl    = nullptr;
    wxTextCtrl* m_nCtrl         = nullptr;

    wxSizerItem*      m_rodBoxItem = nullptr;

    wxSizerItem*      m_attachedBoxItem = nullptr;
};