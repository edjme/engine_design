#include "KinematicPlotPanel.h"

#include <wx/dcbuffer.h>
#include <algorithm>
#include <limits>

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
        wxString::FromUTF8("угол поворота коленвала, градусы"),
        rect.GetLeft() + 40,
        rect.GetBottom() + 5
    );

    // Подпись оси Y зависит от типа графика
    wxString yLabel;
    switch (m_mode)
    {
    case Mode::Displacement:
        yLabel = wxString::FromUTF8("перемещение, мм");
        break;
    case Mode::Velocity:
        yLabel = wxString::FromUTF8("скорость, мм/с");
        break;
    case Mode::Acceleration:
        yLabel = wxString::FromUTF8("ускорение, мм/с²");
        break;
    }

    dc.DrawRotatedText(yLabel,
                       rect.GetLeft() - 45,
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

    const std::vector<double>* yvec = nullptr;

    switch (m_mode)
    {
    case Mode::Displacement:
        yvec = &m_results->stroke_full;
        break;
    case Mode::Velocity:
        yvec = &m_results->velocity_full;
        break;
    case Mode::Acceleration:
        yvec = &m_results->acceleration_full;
        break;
    }

    if (!yvec || yvec->size() != alpha.size() || yvec->empty())
    {
        dc.DrawText(wxString::FromUTF8("Нет данных для выбранного типа графика"),
                    rect.GetLeft() + 10, rect.GetTop() + 10);
        return;
    }

    // Диапазоны X и Y
    double minX = alpha.front();
    double maxX = alpha.back();

    auto [minYIt, maxYIt] = std::minmax_element(yvec->begin(), yvec->end());
    double minY = *minYIt;
    double maxY = *maxYIt;

    if (std::abs(maxY - minY) < 1e-12)
    {
        maxY += 1.0;
        minY -= 1.0;
    }

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

    // Линия графика
    dc.SetPen(wxPen(wxColour(0x3A, 0x7B, 0xD5), 2));

    wxPoint prev(mapX(alpha[0]), mapY((*yvec)[0]));
    for (size_t i = 1; i < alpha.size(); ++i)
    {
        wxPoint cur(mapX(alpha[i]), mapY((*yvec)[i]));
        dc.DrawLine(prev, cur);
        prev = cur;
    }

    // Поиск максимума и минимума (для подписей и маркеров)
    size_t idxMax = std::distance(yvec->begin(), maxYIt);
    size_t idxMin = std::distance(yvec->begin(), minYIt);

    double xAtMax = alpha[idxMax];
    double xAtMin = alpha[idxMin];

    // Маркеры max/min
    auto drawMarker = [&](double xVal, double yVal, const wxColour& color)
    {
        int px = mapX(xVal);
        int py = mapY(yVal);
        dc.SetBrush(wxBrush(color));
        dc.SetPen(wxPen(color));
        dc.DrawCircle(px, py, 4);
    };

    drawMarker(xAtMax, maxY, wxColour(0, 180, 255));   // max — голубой
    drawMarker(xAtMin, minY, wxColour(255, 120, 120)); // min — красный

    // Подписи max / min в левом верхнем углу области графика
    dc.SetTextForeground(wxColour(0xE0, 0xE0, 0xE0));

    wxString maxText = wxString::Format(
        wxString::FromUTF8("max = %.3f при %.1f°"), maxY, xAtMax);
    wxString minText = wxString::Format(
        wxString::FromUTF8("min = %.3f при %.1f°"), minY, xAtMin);

    dc.DrawText(maxText, rect.GetLeft() + 5, rect.GetTop() + 5);
    dc.DrawText(minText, rect.GetLeft() + 5, rect.GetTop() + 25);
}
