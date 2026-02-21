#include "KinematicPlotPanel.h"
#include <wx/dcbuffer.h>
#include <algorithm>
#include <limits>
#include <cmath>

wxBEGIN_EVENT_TABLE(KinematicPlotPanel, wxPanel)
    EVT_PAINT(KinematicPlotPanel::OnPaint)
    EVT_MOUSEWHEEL(KinematicPlotPanel::OnMouseWheel)
    EVT_LEFT_DOWN(KinematicPlotPanel::OnMouseLeftDown)
    EVT_LEFT_UP(KinematicPlotPanel::OnMouseLeftUp)
    EVT_MOTION(KinematicPlotPanel::OnMouseMove)
    EVT_MOUSE_CAPTURE_LOST(KinematicPlotPanel::OnMouseCaptureLost)
wxEND_EVENT_TABLE()

// Вычисление подходящего шага меток на оси
static double niceTickStep(double range, int maxTicks = 8) {
    double roughStep = range / maxTicks;
    double magnitude = pow(10.0, floor(log10(roughStep)));
    double normalized = roughStep / magnitude;
    if (normalized < 1.5)      return magnitude * 1.0;
    else if (normalized < 3.0) return magnitude * 2.0;
    else if (normalized < 7.0) return magnitude * 5.0;
    else                       return magnitude * 10.0;
}

KinematicPlotPanel::KinematicPlotPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
              {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(0x20, 0x25, 0x2B));

    // Цвета для линий цилиндров
    m_cylinderColors = {
        wxColour(0x3A, 0x7B, 0xD5),
        wxColour(0x2E, 0xCC, 0x71),
        wxColour(0xE7, 0x4C, 0x3C),
        wxColour(0xF3, 0x9C, 0x12),
        wxColour(0x9B, 0x59, 0xB6),
        wxColour(0x1A, 0xBC, 0x9C),
        wxColour(0x34, 0x98, 0xDB),
        wxColour(0xE6, 0x7E, 0x22),
        wxColour(0x8E, 0x44, 0xAD),
        wxColour(0x16, 0xA0, 0x85),
        wxColour(0x27, 0xAE, 0x60),
        wxColour(0x29, 0x80, 0xB9),
        wxColour(0xD3, 0x54, 0x00),
        wxColour(0x7D, 0x3C, 0x98),
        wxColour(0x13, 0x8D, 0x75),
        wxColour(0xC0, 0x39, 0x2B)
    };

    m_selectedIndices.clear();
    m_showSide = false;
    m_hasValidData = false;
    m_dragging = false;
    m_previewMode = false;
}

void KinematicPlotPanel::SetPreviewData(const std::vector<double>& x, const std::vector<double>& y,
                                        const wxString& xLabel, const wxString& yBaseLabel)
{
    m_previewX = x;
    m_previewY = y;
    m_previewXLabel = xLabel;
    m_previewYBaseLabel = yBaseLabel;
    m_previewMode = true;
    Refresh();
}

void KinematicPlotPanel::DrawPreview(wxDC& dc, const wxRect& rect)
{
    if (m_previewX.empty() || m_previewY.empty() || m_previewX.size() != m_previewY.size())
        return;

    // Вычисляем границы
    double minX = *std::min_element(m_previewX.begin(), m_previewX.end());
    double maxX = *std::max_element(m_previewX.begin(), m_previewX.end());
    double minY = *std::min_element(m_previewY.begin(), m_previewY.end());
    double maxY = *std::max_element(m_previewY.begin(), m_previewY.end());

    if (std::abs(maxY - minY) < 1e-12) {
        maxY += 1.0;
        minY -= 1.0;
    }
    double yRange = maxY - minY;
    maxY += yRange * 0.05; // небольшой отступ

    // Определяем масштаб для давления
    double scaleFactor = 1.0;
    wxString unitStr = wxString::FromUTF8("Па");
    if (maxY >= 1e6)
    {
        scaleFactor = 1e-6;
        unitStr = wxString::FromUTF8("МПа");
    }
    else if (maxY >= 1e3)
    {
        scaleFactor = 1e-3;
        unitStr = wxString::FromUTF8("кПа");
    }
    // Иначе оставляем Па

    wxString yLabel = m_previewYBaseLabel + " (" + unitStr + ")";

    // Сохраняем старые границы и временно устанавливаем новые
    double oldMinX = m_viewMinX, oldMaxX = m_viewMaxX, oldMinY = m_viewMinY, oldMaxY = m_viewMaxY;
    bool oldValid = m_hasValidData;

    m_viewMinX = minX; m_viewMaxX = maxX; m_viewMinY = minY; m_viewMaxY = maxY;
    m_hasValidData = true;

    // --- Оси ---
    dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90), 2));
    // Ось X (горизонтальная)
    int yZero = static_cast<int>(MapY(0.0, rect) + 0.5);
    if (m_viewMinY <= 0.0 && m_viewMaxY >= 0.0)
        dc.DrawLine(rect.GetLeft(), yZero, rect.GetRight(), yZero);
    else
        dc.DrawLine(rect.GetLeft(), rect.GetBottom(), rect.GetRight(), rect.GetBottom());
    
    // Ось Y (вертикальная)
    int xZero = static_cast<int>(MapX(0.0, rect) + 0.5);
    if (m_viewMinX <= 0.0 && m_viewMaxX >= 0.0)
        dc.DrawLine(xZero, rect.GetTop(), xZero, rect.GetBottom());
    else
        dc.DrawLine(rect.GetLeft(), rect.GetTop(), rect.GetLeft(), rect.GetBottom());

    // Стрелки на концах осей (упрощённо)
    int arrowSize = 6;
    if (m_viewMinY <= 0.0 && m_viewMaxY >= 0.0) {
        dc.DrawLine(rect.GetRight(), yZero, rect.GetRight() - arrowSize, yZero - arrowSize/2);
        dc.DrawLine(rect.GetRight(), yZero, rect.GetRight() - arrowSize, yZero + arrowSize/2);
    } else {
        dc.DrawLine(rect.GetRight(), rect.GetBottom(), rect.GetRight() - arrowSize, rect.GetBottom() - arrowSize/2);
        dc.DrawLine(rect.GetRight(), rect.GetBottom(), rect.GetRight() - arrowSize, rect.GetBottom() + arrowSize/2);
    }
    if (m_viewMinX <= 0.0 && m_viewMaxX >= 0.0) {
        dc.DrawLine(xZero, rect.GetTop(), xZero - arrowSize/2, rect.GetTop() + arrowSize);
        dc.DrawLine(xZero, rect.GetTop(), xZero + arrowSize/2, rect.GetTop() + arrowSize);
    } else {
        dc.DrawLine(rect.GetLeft(), rect.GetTop(), rect.GetLeft() - arrowSize/2, rect.GetTop() + arrowSize);
        dc.DrawLine(rect.GetLeft(), rect.GetTop(), rect.GetLeft() + arrowSize/2, rect.GetTop() + arrowSize);
    }

    // Подписи осей
    dc.SetTextForeground(wxColour(0xC0, 0xC0, 0xC0));
    dc.DrawText(m_previewXLabel, rect.GetLeft() + rect.GetWidth()/2 - 50, rect.GetBottom() + 20);
    dc.DrawRotatedText(yLabel, rect.GetLeft() - 70, rect.GetTop() + rect.GetHeight()/2, 90);

    // --- Сетка и метки ---
    dc.SetPen(wxPen(wxColour(0x45, 0x4B, 0x54), 1, wxPENSTYLE_DOT));

    // Метки по оси X
    double xTickStep = niceTickStep(maxX - minX, 8);
    double xStart = ceil(minX / xTickStep) * xTickStep;
    for (double x = xStart; x <= maxX + 1e-9; x += xTickStep)
    {
        int px = static_cast<int>(MapX(x, rect) + 0.5);
        if (px < rect.GetLeft() || px > rect.GetRight()) continue;
        // Вертикальная линия сетки
        dc.DrawLine(px, rect.GetTop(), px, rect.GetBottom());
        // Засечка
        dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90)));
        int tickY = (m_viewMinY <= 0.0 && m_viewMaxY >= 0.0) ? yZero : rect.GetBottom();
        dc.DrawLine(px, tickY - 4, px, tickY + 4);
        // Подпись
        wxString label = wxString::Format("%.0f", x);
        wxSize textSize = dc.GetTextExtent(label);
        dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
        int labelY = (tickY == rect.GetBottom()) ? tickY + 5 : tickY - textSize.GetHeight() - 5;
        dc.DrawText(label, px - textSize.GetWidth() / 2, labelY);
    }

    // Метки по оси Y (с масштабированием)
    double yTickStep = niceTickStep(maxY - minY, 8);
    double yStart = ceil(minY / yTickStep) * yTickStep;
    for (double y = yStart; y <= maxY; y += yTickStep)
    {
        int py = static_cast<int>(MapY(y, rect) + 0.5);
        if (py < rect.GetTop() || py > rect.GetBottom()) continue;
        // Горизонтальная линия сетки
        dc.SetPen(wxPen(wxColour(0x45, 0x4B, 0x54), 1, wxPENSTYLE_DOT));
        dc.DrawLine(rect.GetLeft(), py, rect.GetRight(), py);
        // Засечка
        dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90)));
        int tickX = (m_viewMinX <= 0.0 && m_viewMaxX >= 0.0) ? xZero : rect.GetLeft();
        dc.DrawLine(tickX - 4, py, tickX + 4, py);
        // Подпись (масштабированная)
        double displayValue = y * scaleFactor;
        wxString label;
        if (yTickStep * scaleFactor >= 0.1)
            label = wxString::Format("%.2f", displayValue);
        else
            label = wxString::Format("%.3f", displayValue);
        wxSize textSize = dc.GetTextExtent(label);
        dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
        int labelX = (tickX == rect.GetLeft()) ? tickX - textSize.GetWidth() - 8 : tickX + 8;
        dc.DrawText(label, labelX, py - textSize.GetHeight() / 2);
    }

    // --- Кривая давления (масштабирование не требуется, т.к. координаты уже в данных) ---
    dc.SetPen(wxPen(wxColour(0xFF, 0xAA, 0x00), 2));
    wxPoint prev = MapPoint(m_previewX[0], m_previewY[0], rect);
    for (size_t i = 1; i < m_previewX.size(); ++i)
    {
        wxPoint cur = MapPoint(m_previewX[i], m_previewY[i], rect);
        dc.DrawLine(prev, cur);
        prev = cur;
    }

    // Рамка
    dc.SetPen(wxPen(wxColour(0xB0, 0xB8, 0xC0), 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(rect);

    // Восстанавливаем старые границы
    m_viewMinX = oldMinX; m_viewMaxX = oldMaxX; m_viewMinY = oldMinY; m_viewMaxY = oldMaxY;
    m_hasValidData = oldValid;
}

void KinematicPlotPanel::SetData(const CalculationResults* results)
{
    m_results = results;
    Refresh();
}

void KinematicPlotPanel::SetMode(Mode mode)
{
    m_mode = mode;
    UpdateDataBounds();
    ResetView();
}

void KinematicPlotPanel::SetSelectedIndices(const std::vector<int>& indices)
{
    m_selectedIndices = indices;
    UpdateDataBounds();
    ResetView();
}

void KinematicPlotPanel::SetShowSide(bool show)
{
    m_showSide = show;
    UpdateDataBounds();
    ResetView();
}

// =====================================================================
//  Отрисовка
// =====================================================================

void KinematicPlotPanel::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();

    

    wxSize sz = GetClientSize();
    if (sz.GetWidth() < 50 || sz.GetHeight() < 50)
        return;

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

    if (m_previewMode)
{
    DrawPreview(dc, plotRect);
    return;
}

    dc.SetTextForeground(wxColour(0xC0, 0xC0, 0xC0));
    dc.SetPen(wxPen(wxColour(0x50, 0x58, 0x60)));

    DrawAxes(dc, plotRect);
    
    // Ограничиваем рисование кривых только областью графика
    dc.SetClippingRegion(plotRect);
    DrawCurve(dc, plotRect);
    dc.DestroyClippingRegion();
    
    DrawLegend(dc, plotRect);
}

void KinematicPlotPanel::DrawAxes(wxDC& dc, const wxRect& rect)
{
    if (!m_hasValidData) {
        // Если данных нет, рисуем только рамку и подписи
        dc.SetPen(wxPen(wxColour(0xB0, 0xB8, 0xC0), 2));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
        return;
    }

    // Определяем, где проходят оси через ноль (если ноль в пределах видимой области)
    bool drawXAxisAtZero = (m_viewMinY <= 0.0 && m_viewMaxY >= 0.0);
    bool drawYAxisAtZero = (m_viewMinX <= 0.0 && m_viewMaxX >= 0.0);

    int axisXPos = drawXAxisAtZero ? static_cast<int>(MapY(0.0, rect) + 0.5) : rect.GetBottom();
    int axisYPos = drawYAxisAtZero ? static_cast<int>(MapX(0.0, rect) + 0.5) : rect.GetLeft();

    // Ось X (горизонтальная)
    dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90), 2));
    dc.DrawLine(rect.GetLeft(), axisXPos, rect.GetRight(), axisXPos);

    // Ось Y (вертикальная)
    dc.DrawLine(axisYPos, rect.GetTop(), axisYPos, rect.GetBottom());

    // Стрелки на концах осей (только если оси не совпадают с краями)
    int arrowSize = 6;
    if (drawXAxisAtZero) {
        // Стрелка справа
        dc.DrawLine(rect.GetRight(), axisXPos,
                    rect.GetRight() - arrowSize, axisXPos - arrowSize/2);
        dc.DrawLine(rect.GetRight(), axisXPos,
                    rect.GetRight() - arrowSize, axisXPos + arrowSize/2);
    }
    if (drawYAxisAtZero) {
        // Стрелка сверху
        dc.DrawLine(axisYPos, rect.GetTop(),
                    axisYPos - arrowSize/2, rect.GetTop() + arrowSize);
        dc.DrawLine(axisYPos, rect.GetTop(),
                    axisYPos + arrowSize/2, rect.GetTop() + arrowSize);
    }

    // Подпись оси X
    dc.SetTextForeground(wxColour(0xC0, 0xC0, 0xC0));
    dc.DrawText(
        wxString::FromUTF8("Угол поворота коленвала, градусы"),
        rect.GetLeft() + rect.GetWidth()/2 - 100,
        rect.GetBottom() + 20
    );

    // Подпись оси Y
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
                       rect.GetLeft() - 70,
                       rect.GetTop() + rect.GetHeight() / 2,
                       90);

  // --- Метки по оси X (шаг 45°) ---
double xTickStep = 45.0;
double xStart = ceil(m_viewMinX / xTickStep) * xTickStep;
for (double x = xStart; x <= m_viewMaxX + 1e-6; x += xTickStep)
{
    int px = static_cast<int>(MapX(x, rect) + 0.5);
    if (px < rect.GetLeft() || px > rect.GetRight())
        continue;
    dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90)));
    int tickY = axisXPos;
    dc.DrawLine(px, tickY - 4, px, tickY + 4);
    wxString label = wxString::Format("%.0f", x);
    wxSize textSize = dc.GetTextExtent(label);
    dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
    int labelY = (axisXPos == rect.GetBottom()) ? axisXPos + 5 : axisXPos - textSize.GetHeight() - 5;
    dc.DrawText(label, px - textSize.GetWidth() / 2, labelY);
}

// Гарантированно рисуем метку для максимального угла (обычно 360°)
if (m_dataMaxX >= m_viewMinX - 1e-6 && m_dataMaxX <= m_viewMaxX + 1e-6)
{
    int px = static_cast<int>(MapX(m_dataMaxX, rect) + 0.5);
    // Ограничиваем позицию, чтобы метка не выходила за рамку
    px = std::max(rect.GetLeft(), std::min(px, rect.GetRight()));
    dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90)));
    int tickY = axisXPos;
    dc.DrawLine(px, tickY - 4, px, tickY + 4);
    wxString label = wxString::Format("%.0f", m_dataMaxX);
    wxSize textSize = dc.GetTextExtent(label);
    dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
    int labelY = (axisXPos == rect.GetBottom()) ? axisXPos + 5 : axisXPos - textSize.GetHeight() - 5;
    dc.DrawText(label, px - textSize.GetWidth() / 2, labelY);
}

    // Засечки и подписи по оси Y
    double yTickStep = niceTickStep(m_viewMaxY - m_viewMinY, 8);
    double yStart = ceil(m_viewMinY / yTickStep) * yTickStep;
    for (double y = yStart; y <= m_viewMaxY; y += yTickStep)
    {
        int py = static_cast<int>(MapY(y, rect) + 0.5);
        if (py < rect.GetTop() || py > rect.GetBottom())
            continue;
        dc.SetPen(wxPen(wxColour(0x80, 0x88, 0x90)));
        // Засечка слева или справа от оси Y
        int tickX = axisYPos;
        dc.DrawLine(tickX - 4, py, tickX + 4, py);
        wxString label;
        if (yTickStep >= 0.1)
            label = wxString::Format("%.2f", y);
        else
            label = wxString::Format("%.3f", y);
        wxSize textSize = dc.GetTextExtent(label);
        dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
        // Подпись слева от оси (или справа, если ось слева)
        int labelX = (axisYPos == rect.GetLeft()) ? axisYPos - textSize.GetWidth() - 8 : axisYPos + 8;
        dc.DrawText(label, labelX, py - textSize.GetHeight() / 2);
    }

    // Рамка вокруг графика (поверх всего, но после засечек)
    dc.SetPen(wxPen(wxColour(0xB0, 0xB8, 0xC0), 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(rect);
}

bool KinematicPlotPanel::SaveAsPNG(const wxString& filename) {
    wxSize sz = GetClientSize();
    if (sz.GetWidth() < 50 || sz.GetHeight() < 50)
        return false;

    wxBitmap bitmap(sz.GetWidth(), sz.GetHeight());
    wxMemoryDC memDC;
    memDC.SelectObject(bitmap);

    memDC.SetBackground(GetBackgroundColour());
    memDC.Clear();

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

    DrawAxes(memDC, plotRect);
    memDC.SetClippingRegion(plotRect);
    DrawCurve(memDC, plotRect);
    memDC.DestroyClippingRegion();
    DrawLegend(memDC, plotRect);

    memDC.SelectObject(wxNullBitmap);
    return bitmap.SaveFile(filename, wxBITMAP_TYPE_PNG);
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

    if (m_selectedIndices.empty())
    {
        dc.DrawText(wxString::FromUTF8("Нет выбранных цилиндров для отображения"),
                    rect.GetLeft() + 10, rect.GetTop() + 10);
        return;
    }

    const auto* mainData = GetYDataForMode(m_mode);
    if (!mainData || mainData->empty())
    {
        dc.DrawText(wxString::FromUTF8("Нет данных для выбранного типа графика"),
                    rect.GetLeft() + 10, rect.GetTop() + 10);
        return;
    }

    const std::vector<std::vector<double>>* sideData = nullptr;
    if (m_showSide)
    {
        Mode sideMode = GetSideModeForCurrent();
        sideData = GetYDataForMode(sideMode);
    }

    if (!m_hasValidData)
        return;

    // Сетка (опционально, можно убрать, если мешает)
    dc.SetPen(wxPen(wxColour(0x45, 0x4B, 0x54), 1, wxPENSTYLE_DOT));
    double xTickStep = 45.0;
    double xStart = ceil(m_viewMinX / xTickStep) * xTickStep;
    for (double x = xStart; x <= m_viewMaxX + 1e-9; x += xTickStep)
    {
        int gx = static_cast<int>(MapX(x, rect) + 0.5);
        if (gx >= rect.GetLeft() && gx <= rect.GetRight())
            dc.DrawLine(gx, rect.GetTop(), gx, rect.GetBottom());
    }
    double yTickStep = niceTickStep(m_viewMaxY - m_viewMinY, 8);
    double yStart = ceil(m_viewMinY / yTickStep) * yTickStep;
    for (double y = yStart; y <= m_viewMaxY + 1e-9; y += yTickStep)
    {
        int gy = static_cast<int>(MapY(y, rect) + 0.5);
        if (gy >= rect.GetTop() && gy <= rect.GetBottom())
            dc.DrawLine(rect.GetLeft(), gy, rect.GetRight(), gy);
    }

    // Линия нуля (если не совпадает с осью)
    if (m_viewMinY <= 0.0 && m_viewMaxY >= 0.0)
    {
        int yZero = static_cast<int>(MapY(0.0, rect) + 0.5);
        // Если ось X уже нарисована, не рисуем поверх неё, иначе рисуем пунктиром
        if (std::abs(yZero - rect.GetBottom()) > 2 && std::abs(yZero - rect.GetTop()) > 2) {
            dc.SetPen(wxPen(*wxWHITE, 1, wxPENSTYLE_DOT));
            dc.DrawLine(rect.GetLeft(), yZero, rect.GetRight(), yZero);
        }
    }

    // Отрисовка кривых
    for (int idx : m_selectedIndices)
    {
        if (idx < 0 || idx >= (int)mainData->size()) continue;
        const auto& mainVec = (*mainData)[idx];
        if (mainVec.size() != alpha.size()) continue;

        wxColour color = m_cylinderColors[idx % m_cylinderColors.size()];

        // Основная кривая (сплошная)
        dc.SetPen(wxPen(color, 2));
        wxPoint prev = MapPoint(alpha[0], mainVec[0], rect);
        for (size_t i = 1; i < alpha.size(); ++i)
        {
            wxPoint cur = MapPoint(alpha[i], mainVec[i], rect);
            dc.DrawLine(prev, cur);
            prev = cur;
        }

        // Боковая кривая (пунктирная)
        if (m_showSide && sideData && idx < (int)sideData->size())
        {
            const auto& sideVec = (*sideData)[idx];
            if (sideVec.size() == alpha.size())
            {
                dc.SetPen(wxPen(color, 2));
                bool draw = true;
                double dashLength = 5.0;
                double gapLength = 8.0;

                for (size_t i = 1; i < alpha.size(); ++i)
                {
                    wxPoint p1 = MapPoint(alpha[i-1], sideVec[i-1], rect);
                    wxPoint p2 = MapPoint(alpha[i], sideVec[i], rect);

                    double dx = p2.x - p1.x;
                    double dy = p2.y - p1.y;
                    double length = sqrt(dx*dx + dy*dy);
                    if (length < 0.1) continue;

                    double stepX = dx / length;
                    double stepY = dy / length;

                    double pos = 0.0;
                    while (pos < length)
                    {
                        if (draw)
                        {
                            double segEnd = std::min(pos + dashLength, length);
                            wxPoint segP1(p1.x + stepX * pos, p1.y + stepY * pos);
                            wxPoint segP2(p1.x + stepX * segEnd, p1.y + stepY * segEnd);
                            dc.DrawLine(segP1, segP2);
                            pos = segEnd + gapLength;
                        }
                        else
                        {
                            pos += gapLength;
                        }
                        draw = !draw;
                    }
                }
            }
        }
    }
}

void KinematicPlotPanel::DrawLegend(wxDC& dc, const wxRect& rect)
{
    if (!m_results || m_selectedIndices.empty()) return;

    bool isVType = (m_params && (m_params->gamma != 0.0 || m_params->gammaPric != 0.0));

    int numItems = (int)m_selectedIndices.size();
    int legendWidth = 140;
    int legendHeight = numItems * 20 + 30;
    int legendX = rect.GetRight() - legendWidth - 10;
    int legendY = rect.GetTop() + 10;

    dc.SetBrush(wxBrush(wxColour(0x30, 0x35, 0x3B, 200)));
    dc.SetPen(wxPen(wxColour(0x50, 0x58, 0x60)));
    dc.DrawRectangle(legendX, legendY, legendWidth, legendHeight);

    dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));
    dc.DrawText(wxString::FromUTF8("Отображаемые:"), legendX + 5, legendY + 5);

    int yPos = legendY + 25;
    for (int idx : m_selectedIndices)
    {
        wxColour color = m_cylinderColors[idx % m_cylinderColors.size()];

        // Квадратик цвета
        dc.SetBrush(wxBrush(color));
        dc.SetPen(wxPen(color));
        dc.DrawRectangle(legendX + 10, yPos, 10, 10);

        if (m_showSide)
        {
            dc.SetPen(wxPen(color, 1, wxPENSTYLE_LONG_DASH));
            dc.DrawLine(legendX + 25, yPos + 5, legendX + 40, yPos + 5);
            wxString label = wxString::Format(isVType ? wxString::FromUTF8("Ряд %d + бок") : wxString::FromUTF8("Цил.%d + бок"), idx + 1);
            dc.DrawText(label, legendX + 45, yPos - 3);
        }
        else
        {
            wxString label = wxString::Format(isVType ? wxString::FromUTF8("Ряд %d") : wxString::FromUTF8("Цил.%d"), idx + 1);
            dc.DrawText(label, legendX + 25, yPos - 3);
        }
        yPos += 20;
    }
}

// =====================================================================
//  Вспомогательные методы
// =====================================================================

void KinematicPlotPanel::UpdateDataBounds()
{
    if (!m_results || m_results->alpha.empty() || m_selectedIndices.empty()) {
        m_hasValidData = false;
        return;
    }

    const auto& alpha = m_results->alpha;
    m_dataMinX = alpha.front();
    m_dataMaxX = alpha.back();

    const auto* mainData = GetYDataForMode(m_mode);
    if (!mainData || mainData->empty()) {
        m_hasValidData = false;
        return;
    }

    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (int idx : m_selectedIndices) {
        if (idx < 0 || idx >= (int)mainData->size()) continue;
        const auto& vec = (*mainData)[idx];
        if (vec.size() != alpha.size()) continue;
        auto minmax = std::minmax_element(vec.begin(), vec.end());
        minY = std::min(minY, *minmax.first);
        maxY = std::max(maxY, *minmax.second);
    }

    if (m_showSide) {
        Mode sideMode = GetSideModeForCurrent();
        const auto* sideData = GetYDataForMode(sideMode);
        if (sideData && !sideData->empty()) {
            for (int idx : m_selectedIndices) {
                if (idx < 0 || idx >= (int)sideData->size()) continue;
                const auto& vec = (*sideData)[idx];
                if (vec.size() != alpha.size()) continue;
                auto minmax = std::minmax_element(vec.begin(), vec.end());
                minY = std::min(minY, *minmax.first);
                maxY = std::max(maxY, *minmax.second);
            }
        }
    }

    if (std::abs(maxY - minY) < 1e-12) {
        maxY += 1.0;
        minY -= 1.0;
    }
    double yRange = maxY - minY;
    maxY += yRange * 0.05;  // отступ сверху

    m_dataMinY = minY;
    m_dataMaxY = maxY;
    m_hasValidData = true;
}

void KinematicPlotPanel::ResetView()
{
    if (!m_hasValidData) return;
    m_viewMinX = m_dataMinX;
    m_viewMaxX = m_dataMaxX;
    m_viewMinY = m_dataMinY;
    m_viewMaxY = m_dataMaxY;
    Refresh();
}

const std::vector<std::vector<double>>* KinematicPlotPanel::GetYDataForMode(Mode mode) const
{
    if (!m_results) return nullptr;

    switch (mode)
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

wxRect KinematicPlotPanel::GetPlotRect() const
{
    wxSize sz = GetClientSize();
    const int marginLeft   = 70;
    const int marginRight  = 20;
    const int marginTop    = 20;
    const int marginBottom = 50;
    return wxRect(marginLeft, marginTop,
                  sz.GetWidth() - marginLeft - marginRight,
                  sz.GetHeight() - marginTop - marginBottom);
}

KinematicPlotPanel::Mode KinematicPlotPanel::GetSideModeForCurrent() const
{
    switch (m_mode)
    {
    case Mode::DisplacementMain:
        return Mode::DisplacementSide;
    case Mode::VelocityMain:
        return Mode::VelocitySide;
    case Mode::AccelerationMain:
        return Mode::AccelerationSide;
    default:
        return m_mode;
    }
}

double KinematicPlotPanel::MapX(double x, const wxRect& rect) const
{
    return rect.GetLeft() + (x - m_viewMinX) * rect.GetWidth() / (m_viewMaxX - m_viewMinX);
}

double KinematicPlotPanel::MapY(double y, const wxRect& rect) const
{
    return rect.GetBottom() - (y - m_viewMinY) * rect.GetHeight() / (m_viewMaxY - m_viewMinY);
}

wxPoint KinematicPlotPanel::MapPoint(double x, double y, const wxRect& rect) const
{
    return wxPoint(static_cast<int>(MapX(x, rect) + 0.5),
                   static_cast<int>(MapY(y, rect) + 0.5));
}

// Обработчики мыши
void KinematicPlotPanel::OnMouseWheel(wxMouseEvent& evt)
{
    if (!m_hasValidData) {
        evt.Skip();
        return;
    }

    wxRect plotRect = GetPlotRect();
    wxPoint mousePos = evt.GetPosition();
    if (!plotRect.Contains(mousePos)) {
        evt.Skip();
        return;
    }

    double dataX = m_viewMinX + (mousePos.x - plotRect.GetLeft()) * (m_viewMaxX - m_viewMinX) / plotRect.GetWidth();
    double dataY = m_viewMaxY - (mousePos.y - plotRect.GetTop()) * (m_viewMaxY - m_viewMinY) / plotRect.GetHeight();

    double zoomFactor = 1.1;
    if (evt.GetWheelRotation() < 0)
        zoomFactor = 1.0 / zoomFactor;

    double newWidth = (m_viewMaxX - m_viewMinX) * zoomFactor;
    double newHeight = (m_viewMaxY - m_viewMinY) * zoomFactor;

    double newMinX = dataX - (dataX - m_viewMinX) * zoomFactor;
    double newMaxX = newMinX + newWidth;
    double newMinY = dataY - (dataY - m_viewMinY) * zoomFactor;
    double newMaxY = newMinY + newHeight;

    // Ограничение полными данными с сохранением размера окна
    if (newMinX < m_dataMinX) { newMinX = m_dataMinX; newMaxX = newMinX + newWidth; }
    if (newMaxX > m_dataMaxX) { newMaxX = m_dataMaxX; newMinX = newMaxX - newWidth; }
    if (newMinY < m_dataMinY) { newMinY = m_dataMinY; newMaxY = newMinY + newHeight; }
    if (newMaxY > m_dataMaxY) { newMaxY = m_dataMaxY; newMinY = newMaxY - newHeight; }

    m_viewMinX = newMinX;
    m_viewMaxX = newMaxX;
    m_viewMinY = newMinY;
    m_viewMaxY = newMaxY;

    Refresh();
}

void KinematicPlotPanel::OnMouseLeftDown(wxMouseEvent& evt)
{
    if (!m_hasValidData) {
        evt.Skip();
        return;
    }

    if (evt.ControlDown()) {
        m_dragging = true;
        m_dragLastPos = evt.GetPosition();
        m_dragStartMinX = m_viewMinX;
        m_dragStartMaxX = m_viewMaxX;
        m_dragStartMinY = m_viewMinY;
        m_dragStartMaxY = m_viewMaxY;
        CaptureMouse();
    } else {
        evt.Skip();
    }
}

void KinematicPlotPanel::OnMouseLeftUp(wxMouseEvent& evt)
{
    if (m_dragging) {
        m_dragging = false;
        ReleaseMouse();
    } else {
        evt.Skip();
    }
}

void KinematicPlotPanel::OnMouseMove(wxMouseEvent& evt)
{
    if (m_dragging && evt.LeftIsDown() && evt.ControlDown()) {
        wxPoint currentPos = evt.GetPosition();
        wxPoint delta = currentPos - m_dragLastPos;

        wxRect plotRect = GetPlotRect();
        if (plotRect.width == 0) return;

        double dx = -delta.x * (m_viewMaxX - m_viewMinX) / plotRect.width;
        double dy = delta.y * (m_viewMaxY - m_viewMinY) / plotRect.height;

        double newMinX = m_dragStartMinX + dx;
        double newMaxX = m_dragStartMaxX + dx;
        double newMinY = m_dragStartMinY + dy;
        double newMaxY = m_dragStartMaxY + dy;

        double width = m_viewMaxX - m_viewMinX;
        double height = m_viewMaxY - m_viewMinY;

        if (newMinX < m_dataMinX) { newMinX = m_dataMinX; newMaxX = newMinX + width; }
        if (newMaxX > m_dataMaxX) { newMaxX = m_dataMaxX; newMinX = newMaxX - width; }
        if (newMinY < m_dataMinY) { newMinY = m_dataMinY; newMaxY = newMinY + height; }
        if (newMaxY > m_dataMaxY) { newMaxY = m_dataMaxY; newMinY = newMaxY - height; }

        m_viewMinX = newMinX;
        m_viewMaxX = newMaxX;
        m_viewMinY = newMinY;
        m_viewMaxY = newMaxY;

        Refresh();
    } else {
        evt.Skip();
    }
}

void KinematicPlotPanel::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
    m_dragging = false;
}