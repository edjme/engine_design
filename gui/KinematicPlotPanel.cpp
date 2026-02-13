#include "KinematicPlotPanel.h"

#include <wx/dcbuffer.h>
#include <algorithm>
#include <limits>
#include <cmath>

wxBEGIN_EVENT_TABLE(KinematicPlotPanel, wxPanel)
    EVT_PAINT(KinematicPlotPanel::OnPaint)
wxEND_EVENT_TABLE()

KinematicPlotPanel::KinematicPlotPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    // тёмный фон
    SetBackgroundColour(wxColour(0x20, 0x25, 0x2B));
    
    // Инициализируем цвета для цилиндров
    m_cylinderColors = {
        wxColour(0x3A, 0x7B, 0xD5),  // Синий - цилиндр 1
        wxColour(0x2E, 0xCC, 0x71),  // Зеленый - цилиндр 2
        wxColour(0xE7, 0x4C, 0x3C),  // Красный - цилиндр 3
        wxColour(0xF3, 0x9C, 0x12),  // Оранжевый - цилиндр 4
        wxColour(0x9B, 0x59, 0xB6),  // Фиолетовый - цилиндр 5
        wxColour(0x1A, 0xBC, 0x9C),  // Бирюзовый - цилиндр 6
        wxColour(0x34, 0x98, 0xDB),  // Голубой - цилиндр 7
        wxColour(0xE6, 0x7E, 0x22),  // Темно-оранжевый - цилиндр 8
        wxColour(0x8E, 0x44, 0xAD),  // Темно-фиолетовый - цилиндр 9
        wxColour(0x16, 0xA0, 0x85),  // Темно-бирюзовый - цилиндр 10
        wxColour(0x27, 0xAE, 0x60),  // Светло-зеленый - цилиндр 11
        wxColour(0x29, 0x80, 0xB9),  // Светло-синий - цилиндр 12
        wxColour(0xD3, 0x54, 0x00),  // Коричневый - цилиндр 13
        wxColour(0x7D, 0x3C, 0x98),  // Пурпурный - цилиндр 14
        wxColour(0x13, 0x8D, 0x75),  // Морской волны - цилиндр 15
        wxColour(0xC0, 0x39, 0x2B)   // Алый - цилиндр 16
    };
    
    m_currentCylinder = 0;  // Показываем первый цилиндр по умолчанию
    m_showAllCylinders = false;  // По умолчанию показываем один цилиндр
}

void KinematicPlotPanel::SetData(const CalculationResults* results)
{
    m_results = results;
    Refresh();
}

void KinematicPlotPanel::SetMode(Mode mode)
{
    m_mode = mode;
    Refresh();
}

void KinematicPlotPanel::SetCurrentCylinder(int cylinder)
{
    if (cylinder >= 0) {
        m_currentCylinder = cylinder;
        m_showAllCylinders = false;
        Refresh();
    }
}

void KinematicPlotPanel::SetShowAllCylinders(bool showAll)
{
    m_showAllCylinders = showAll;
    Refresh();
}

int KinematicPlotPanel::GetCurrentCylinder() const
{
    return m_currentCylinder;
}

bool KinematicPlotPanel::GetShowAllCylinders() const
{
    return m_showAllCylinders;
}

// ---------------------------------------------------------------------
//   Отрисовка
// ---------------------------------------------------------------------

void KinematicPlotPanel::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    wxSize sz = GetClientSize();
    if (sz.GetWidth() < 50 || sz.GetHeight() < 50)
        return;

    // Рабочая область графика
    const int marginLeft   = 70;
    const int marginRight  = 20;
    const int marginTop    = 20;
    const int marginBottom = 50;

    wxRect plotRect(
        marginLeft,
        marginTop,
        sz.GetWidth()  - marginLeft - marginRight,
        sz.GetHeight() - marginTop  - marginBottom
    );

    dc.SetTextForeground(wxColour(0xC0, 0xC0, 0xC0));
    dc.SetPen(wxPen(wxColour(0x50, 0x58, 0x60)));

    DrawAxes(dc, plotRect);
    DrawCurve(dc, plotRect);
    DrawLegend(dc, plotRect);
}

void KinematicPlotPanel::DrawAxes(wxDC& dc, const wxRect& rect)
{
    // Ось X (снизу) и ось Y (слева)
    dc.DrawLine(rect.GetLeft(),  rect.GetBottom(),
                rect.GetRight(), rect.GetBottom());
    dc.DrawLine(rect.GetLeft(),  rect.GetTop(),
                rect.GetLeft(),  rect.GetBottom());

    // Подпись оси X
    dc.DrawText(
        wxString::FromUTF8("Угол поворота коленвала, градусы"),
        rect.GetLeft() + rect.GetWidth()/2 - 100,
        rect.GetBottom() + 10
    );

    // Подпись оси Y зависит от типа графика
    wxString yLabel;
    switch (m_mode)
    {
    case Mode::Displacement:
    case Mode::DisplacementMain:
        yLabel = wxString::FromUTF8("Перемещение поршня, м");
        break;
    case Mode::Velocity:
    case Mode::VelocityMain:
        yLabel = wxString::FromUTF8("Скорость поршня, м/с");
        break;
    case Mode::Acceleration:
    case Mode::AccelerationMain:
        yLabel = wxString::FromUTF8("Ускорение поршня, м/с²");
        break;
    case Mode::DisplacementSide:
        yLabel = wxString::FromUTF8("Перемещение бокового поршня, м");
        break;
    case Mode::VelocitySide:
        yLabel = wxString::FromUTF8("Скорость бокового поршня, м/с");
        break;
    case Mode::AccelerationSide:
        yLabel = wxString::FromUTF8("Ускорение бокового поршня, м/с²");
        break;
    }

    dc.DrawRotatedText(yLabel,
                       rect.GetLeft() - 50,
                       rect.GetTop() + rect.GetHeight() / 2,
                       90);
}

void KinematicPlotPanel::DrawCurve(wxDC& dc, const wxRect& rect)
{
    if (!m_results || m_results->alpha.empty())
    {
        dc.DrawText(wxString::FromUTF8("Нет данных для отображения"),
                    rect.GetLeft() + 10, rect.GetTop() + 10);
        return;
    }

    const auto& alpha = m_results->alpha;
    if (alpha.empty()) return;

    // Определяем, сколько цилиндров нужно отображать
    int numCylindersToShow = 0;
    int startCylinder = 0;
    int endCylinder = 0;
    
    if (m_showAllCylinders) {
        // Определяем максимальное количество цилиндров из доступных данных
        numCylindersToShow = GetAvailableCylinderCount();
        startCylinder = 0;
        endCylinder = numCylindersToShow - 1;
    } else {
        // Показываем только текущий цилиндр
        numCylindersToShow = 1;
        startCylinder = m_currentCylinder;
        endCylinder = m_currentCylinder;
    }

    // Проверяем, что у нас есть данные для отображения
    if (numCylindersToShow == 0) {
        dc.DrawText(wxString::FromUTF8("Нет данных цилиндров для отображения"),
                    rect.GetLeft() + 10, rect.GetTop() + 10);
        return;
    }

    // Получаем данные для графика
    const std::vector<std::vector<double>>* yData = GetYDataForMode();
    if (!yData || yData->empty()) {
        dc.DrawText(wxString::FromUTF8("Нет данных для выбранного типа графика"),
                    rect.GetLeft() + 10, rect.GetTop() + 10);
        return;
    }

    // Находим общие диапазоны для всех отображаемых цилиндров
    double minX = alpha.front();
    double maxX = alpha.back();
    
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (int cyl = startCylinder; cyl <= endCylinder; ++cyl) {
        if (cyl >= static_cast<int>(yData->size())) continue;
        
        const auto& cylData = (*yData)[cyl];
        if (cylData.size() != alpha.size() || cylData.empty()) continue;
        
        auto [minIt, maxIt] = std::minmax_element(cylData.begin(), cylData.end());
        minY = std::min(minY, *minIt);
        maxY = std::max(maxY, *maxIt);
    }

    if (std::abs(maxY - minY) < 1e-12) {
        maxY += 1.0;
        minY -= 1.0;
    }

    // Добавляем немного запаса по Y для лучшего отображения
    double yRange = maxY - minY;
    maxY += yRange * 0.05;

    const double xScale = (maxX - minX) == 0.0
                            ? 1.0
                            : static_cast<double>(rect.GetWidth()) / (maxX - minX);
    const double yScale = (maxY - minY) == 0.0
                            ? 1.0
                            : static_cast<double>(rect.GetHeight()) / (maxY - minY);

    auto mapX = [&](double x) {
        return rect.GetLeft() +
               static_cast<int>((x - minX) * xScale);
    };
    auto mapY = [&](double y) {
        return rect.GetBottom() -
               static_cast<int>((y - minY) * yScale);
    };

    // Сетка (несколько вертикальных и горизонтальных линий)
    dc.SetPen(wxPen(wxColour(0x35, 0x3B, 0x44), 1, wxPENSTYLE_DOT));

    const int gridLines = 5;
    for (int i = 1; i < gridLines; ++i)
    {
        int gx = rect.GetLeft() + i * rect.GetWidth() / gridLines;
        dc.DrawLine(gx, rect.GetTop(), gx, rect.GetBottom());

        int gy = rect.GetTop() + i * rect.GetHeight() / gridLines;
        dc.DrawLine(rect.GetLeft(), gy, rect.GetRight(), gy);
    }

    // Отрисовка кривых для каждого цилиндра
    for (int cyl = startCylinder; cyl <= endCylinder; ++cyl) {
        if (cyl >= static_cast<int>(yData->size())) continue;
        
        const auto& cylData = (*yData)[cyl];
        if (cylData.size() != alpha.size() || cylData.empty()) continue;
        
        // Выбираем цвет для цилиндра
        wxColour color = m_cylinderColors[cyl % m_cylinderColors.size()];
        dc.SetPen(wxPen(color, m_showAllCylinders ? 1 : 2));
        
        // Рисуем линию графика
        wxPoint prev(mapX(alpha[0]), mapY(cylData[0]));
        for (size_t i = 1; i < alpha.size(); ++i) {
            wxPoint cur(mapX(alpha[i]), mapY(cylData[i]));
            dc.DrawLine(prev, cur);
            prev = cur;
        }
        
        // Для текущего цилиндра (или если показываем один) добавляем маркеры min/max
        if (!m_showAllCylinders || cyl == m_currentCylinder) {
            auto maxIt = std::max_element(cylData.begin(), cylData.end());
            auto minIt = std::min_element(cylData.begin(), cylData.end());
            
            if (maxIt != cylData.end() && minIt != cylData.end()) {
                size_t idxMax = std::distance(cylData.begin(), maxIt);
                size_t idxMin = std::distance(cylData.begin(), minIt);
                
                double xAtMax = alpha[idxMax];
                double xAtMin = alpha[idxMin];
                
                // Маркеры max/min
                auto drawMarker = [&](double xVal, double yVal, const wxColour& markerColor)
                {
                    int px = mapX(xVal);
                    int py = mapY(yVal);
                    dc.SetBrush(wxBrush(markerColor));
                    dc.SetPen(wxPen(markerColor, 1));
                    dc.DrawCircle(px, py, 4);
                };
                
                drawMarker(xAtMax, *maxIt, wxColour(0, 180, 255));   // max — голубой
                drawMarker(xAtMin, *minIt, wxColour(255, 120, 120)); // min — красный
                
                // Подписи max / min
                dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
                
                wxString cylinderText = m_showAllCylinders 
                    ? wxString::FromUTF8("Цил.%d: ", cyl + 1)
                    : wxString(wxString::FromUTF8(""));
                
                wxString maxText = wxString::Format(
                   wxString::FromUTF8("max = %.3f при %.1f°"), *maxIt, xAtMax);
                wxString minText = wxString::Format(
                   wxString::FromUTF8("min = %.3f при %.1f°"), *minIt, xAtMin);
                
                int yOffset = cyl * 20;  // Смещение для каждого цилиндра
                dc.DrawText(maxText, rect.GetLeft() + 5, rect.GetTop() + 5 + yOffset);
                dc.DrawText(minText, rect.GetLeft() + 5, rect.GetTop() + 25 + yOffset);
            }
        }
    }
}

void KinematicPlotPanel::DrawLegend(wxDC& dc, const wxRect& rect)
{
    
    if (!m_results || !m_showAllCylinders) return;
    
    int numCylinders = GetAvailableCylinderCount();
    if (numCylinders <= 1) return;  // Легенда не нужна для одного цилиндра

    bool hasSideCylinder = false;
    if (m_params) {
        hasSideCylinder = (m_params->gamma != 0.0);
    } else {
        // Fallback - проверяем векторы
        hasSideCylinder = !m_results->cylinder_stroke_full_side.empty();
    }
    // Создаем прямоугольник для легенды
    int legendWidth = 120;
    int legendHeight = numCylinders;
    int legendX = rect.GetRight() - legendWidth - 10;
    int legendY = rect.GetTop() + 10;
    
    // Фон легенды
    dc.SetBrush(wxBrush(wxColour(0x30, 0x35, 0x3B, 200)));  // Полупрозрачный
    dc.SetPen(wxPen(wxColour(0x50, 0x58, 0x60)));
    if (hasSideCylinder == false) {
        dc.DrawRectangle(legendX, legendY, legendWidth, legendHeight * 20 + 20);
    } else {
        dc.DrawRectangle(legendX, legendY, legendWidth, legendHeight * 10 + 20);
    }
    
    // Заголовок легенды
    dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
    if (hasSideCylinder == false) {
    dc.DrawText(wxString::FromUTF8("Цилиндры:"), legendX + 5, legendY + 5);
    }
    else {
    dc.DrawText(wxString::FromUTF8("Ряды:"), legendX + 5, legendY + 5);   
    }

    // Элементы легенды
    if (hasSideCylinder == false) {
    for (int i = 0; i < numCylinders; ++i) {
        int yPos = legendY + 25 + i * 20;
        
        // Цветной квадратик
        wxColour color = m_cylinderColors[i % m_cylinderColors.size()];
        dc.SetBrush(wxBrush(color));
        dc.SetPen(wxPen(color));
        dc.DrawRectangle(legendX + 10, yPos, 10, 10);
        
        // Текст с номером цилиндра
        
        wxString label = wxString::Format(wxString::FromUTF8("Цилиндр %d"), i + 1);
        dc.DrawText(label, legendX + 25, yPos - 3);
    }
    }
    else {
    for (int i = 0; i < numCylinders / 2; ++i) {
        int yPos = legendY + 25 + i * 20;
        
        // Цветной квадратик
        wxColour color = m_cylinderColors[i % m_cylinderColors.size()];
        dc.SetBrush(wxBrush(color));
        dc.SetPen(wxPen(color));
        dc.DrawRectangle(legendX + 10, yPos, 10, 10);
        
        // Текст с номером цилиндра/ряда
                wxString label = wxString::Format(wxString::FromUTF8("Ряд %d"), i + 1);
        dc.DrawText(label, legendX + 25, yPos - 3);
    }
    }

    
}

const std::vector<std::vector<double>>* KinematicPlotPanel::GetYDataForMode() const
{
    if (!m_results) return nullptr;
    
    switch (m_mode)
    {
    case Mode::Displacement:
    case Mode::DisplacementMain:
        return &m_results->cylinder_stroke_full;
    case Mode::Velocity:
    case Mode::VelocityMain:
        return &m_results->cylinder_velocity_full;
    case Mode::Acceleration:
    case Mode::AccelerationMain:
        return &m_results->cylinder_acceleration_full;
    case Mode::DisplacementSide:
        return &m_results->cylinder_stroke_full_side;
    case Mode::VelocitySide:
        return &m_results->cylinder_velocity_full_side;
    case Mode::AccelerationSide:
        return &m_results->cylinder_acceleration_full_side;
    default:
        return nullptr;
    }
}

int KinematicPlotPanel::GetAvailableCylinderCount() const
{
    if (!m_results) return 0;
    
    if (m_params) {
        bool isVType = (m_params->gamma != 0.0 || m_params->gammaPric != 0.0);
        if (isVType) {
            // Для V-образных возвращаем количество рядов
            return m_params->countCyl / 2;
        }
    }
    
    // Для рядных или по умолчанию
    if (!m_results->cylinder_stroke_full.empty()) {
        return static_cast<int>(m_results->cylinder_stroke_full.size());
    }

    return 0;
}

// ---------------------------------------------------------------------
//   Обновленный enum Mode в заголовочном файле
// ---------------------------------------------------------------------
/*
// В KinematicPlotPanel.h нужно добавить новые значения в enum Mode:
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
*/