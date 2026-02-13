#ifndef KINEMATICPLOTPANEL_H
#define KINEMATICPLOTPANEL_H

#include <wx/wx.h>
#include "core/common/common_types.h"

class KinematicPlotPanel : public wxPanel
{
public:
    enum class Mode {
        Displacement,       // Перемещение (для обратной совместимости)
        Velocity,           // Скорость (для обратной совместимости)
        Acceleration,       // Ускорение (для обратной совместимости)
        DisplacementMain,   // Перемещение главного цилиндра
        VelocityMain,       // Скорость главного цилиндра
        AccelerationMain,   // Ускорение главного цилиндра
        DisplacementSide,   // Перемещение бокового цилиндра
        VelocitySide,       // Скорость бокового цилиндра
        AccelerationSide    // Ускорение бокового цилиндра
    };
    
    KinematicPlotPanel(wxWindow* parent);
    
    void SetData(const CalculationResults* results);
    void SetMode(Mode mode);
    
    // Управление отображением цилиндров
    void SetCurrentCylinder(int cylinder);
    void SetShowAllCylinders(bool showAll);
    int GetCurrentCylinder() const;
    bool GetShowAllCylinders() const;
    void SetParams(const EngineParams* params) { m_params = params; }
    
private:
    const CalculationResults* m_results = nullptr;
    Mode m_mode = Mode::Displacement;
    
    // Управление отображением цилиндров
    int m_currentCylinder = 0;
    bool m_showAllCylinders = false;
    std::vector<wxColour> m_cylinderColors;
    
    // Методы отрисовки
    void OnPaint(wxPaintEvent& evt);
    void DrawAxes(wxDC& dc, const wxRect& rect);
    void DrawCurve(wxDC& dc, const wxRect& rect);
    void DrawLegend(wxDC& dc, const wxRect& rect);
    const EngineParams* m_params = nullptr;
    
    // Вспомогательные методы
    const std::vector<std::vector<double>>* GetYDataForMode() const;
    int GetAvailableCylinderCount() const;
    
    wxDECLARE_EVENT_TABLE();
};

#endif // KINEMATICPLOTPANEL_H