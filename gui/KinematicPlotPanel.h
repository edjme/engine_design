#ifndef KINEMATICPLOTPANEL_H
#define KINEMATICPLOTPANEL_H

#include <wx/wx.h>
#include "core/common/common_types.h"

// Панель для отображения графиков кинематики
class KinematicPlotPanel : public wxPanel
{
public:
    // Режимы отображения (тип данных и основной/боковой)
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

    enum class DisplayMode {
    Kinematic,  // обычный режим с выбором цилиндров из m_results
    Preview,    // предпросмотр одной кривой (давление и т.п.)
    Multi       // несколько кривых (динамика, выбор цилиндров)
};

void SetMultipleCurvesData(const std::vector<double>& x,
                           const std::vector<std::vector<double>>& y,
                           const std::vector<wxString>& labels,
                           const wxString& xLabel,
                           const wxString& yLabel);

// Новые члены данных
std::vector<double> m_multiX;
std::vector<std::vector<double>> m_multiY;
std::vector<wxString> m_curveLabels;
wxString m_multiXLabel;
wxString m_multiYLabel;
DisplayMode m_displayMode;

    KinematicPlotPanel(wxWindow* parent);
    
    // Установка данных для отображения
    void SetData(const CalculationResults* results);
    void SetParams(const EngineParams* params) { m_params = params; }
    
    // Управление режимом графика
    void SetMode(Mode mode);
    
    // Управление множественным выбором
    void SetSelectedIndices(const std::vector<int>& indices);
    void SetShowSide(bool show);

    // Сохранение изображения графика
    bool SaveAsPNG(const wxString& filename);
    
    // Сброс к полному обзору
    void ResetView();

    Mode GetMode() const { return m_mode; }
    const std::vector<int>& GetSelectedIndices() const { return m_selectedIndices; }

    void SetPreviewData(const std::vector<double>& x, const std::vector<double>& y,
                    const wxString& xLabel = wxString::FromUTF8("Угол, град"),
                    const wxString& yBaseLabel = wxString::FromUTF8("Давление"));

    void Clear();
                    

    
    
private:
    const CalculationResults* m_results = nullptr;
    const EngineParams* m_params = nullptr;
    Mode m_mode = Mode::Displacement;
    
    std::vector<int> m_selectedIndices;      // выбранные индексы
    bool m_showSide = false;                  // показывать ли боковые цилиндры
    
    std::vector<wxColour> m_cylinderColors;   // цвета для линий
    
    // Текущие границы отображения
    double m_viewMinX, m_viewMaxX, m_viewMinY, m_viewMaxY;
    // Полные границы данных
    double m_dataMinX, m_dataMaxX, m_dataMinY, m_dataMaxY;
    bool m_hasValidData;

    // Для панорамирования
    bool m_dragging;
    wxPoint m_dragLastPos;
    double m_dragStartMinX, m_dragStartMaxX, m_dragStartMinY, m_dragStartMaxY;

    // Отрисовка
    void OnPaint(wxPaintEvent& evt);
    void DrawAxes(wxDC& dc, const wxRect& rect);
    void DrawCurve(wxDC& dc, const wxRect& rect);
    void DrawLegend(wxDC& dc, const wxRect& rect);
    wxRect GetPlotRect() const;
    
    // Вспомогательные методы получения данных
    const std::vector<std::vector<double>>* GetYDataForMode(Mode mode) const;
    Mode GetSideModeForCurrent() const;

    // Обновление полных границ данных
    void UpdateDataBounds();

    // Преобразования координат
    inline double MapX(double x, const wxRect& rect) const;
    inline double MapY(double y, const wxRect& rect) const;
    inline wxPoint MapPoint(double x, double y, const wxRect& rect) const;

    std::vector<double> m_previewX;
std::vector<double> m_previewY;
wxString m_previewXLabel;
wxString m_previewYLabel;
bool m_previewMode;
wxString m_previewYBaseLabel;

void DrawPreview(wxDC& dc, const wxRect& rect);

void DrawMultiCurves(wxDC& dc, const wxRect& rect);

void DrawAxesCustom(wxDC& dc, const wxRect& rect, const wxString& xLabel, const wxString& yLabel);
void DrawLegendMulti(wxDC& dc, const wxRect& rect);



    
    wxDECLARE_EVENT_TABLE();
};

#endif // KINEMATICPLOTPANEL_H