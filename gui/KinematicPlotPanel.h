#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include "core/common/common_types.h"

// Простой кастомный график: по оси X — alpha, по оси Y — выбранная величина
class KinematicPlotPanel : public wxPanel
{
public:
    enum class Mode
    {
        Displacement, // перемещение
        Velocity,     // скорость
        Acceleration  // ускорение
    };

    explicit KinematicPlotPanel(wxWindow* parent);

    void SetData(const CalculationResults* results); // не владеет памятью
    void SetMode(Mode mode);

private:
    const CalculationResults* m_results = nullptr;
    Mode                      m_mode    = Mode::Displacement;

    void OnPaint(wxPaintEvent& evt);
    void DrawAxes(wxDC& dc, const wxRect& rect);
    void DrawCurve(wxDC& dc, const wxRect& rect);
    std::vector<double> m_x;
    std::vector<double> m_y;

    double m_yMin = 0.0;
    double m_yMax = 0.0;
    double m_xAtMin = 0.0;
    double m_xAtMax = 0.0;

    wxString m_yLabel;  // подпись оси Y

    void FindExtrema();

    wxDECLARE_EVENT_TABLE();
};
