#include "KinematicTablePanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <sstream>
#include <iomanip>

wxBEGIN_EVENT_TABLE(KinematicTablePanel, wxPanel)
    EVT_CHOICE(wxID_ANY, KinematicTablePanel::OnCylinderChanged)
    EVT_CHECKBOX(wxID_ANY, KinematicTablePanel::OnShowSideChanged)
    EVT_GRID_CELL_LEFT_CLICK(KinematicTablePanel::OnGridCellClick)
    EVT_GRID_LABEL_LEFT_CLICK(KinematicTablePanel::OnGridLabelClick)
wxEND_EVENT_TABLE()

KinematicTablePanel::KinematicTablePanel(wxWindow* parent) : wxPanel(parent) {
    SetBackgroundColour(wxColour(0x18, 0x1C, 0x22));
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Панель управления
    auto* controlPanel = new wxPanel(this);
    controlPanel->SetBackgroundColour(wxColour(0x18, 0x1C, 0x22));
    auto* controlSizer = new wxBoxSizer(wxHORIZONTAL);

    // Метка выбора (будет обновляться позже)
    m_cylinderLabel = new wxStaticText(controlPanel, wxID_ANY, wxString::FromUTF8("Цилиндр:"));
    m_cylinderLabel->SetForegroundColour(*wxWHITE);
    controlSizer->Add(m_cylinderLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    // Выпадающий список (изначально пустой, заполним в SetData)
    m_cylinderChoice = new wxChoice(controlPanel, wxID_ANY, wxDefaultPosition, wxSize(120, -1));
    m_cylinderChoice->SetSelection(0);
    m_cylinderChoice->Enable(false);
    controlSizer->Add(m_cylinderChoice, 0, wxRIGHT, 15);

    // Показать боковой цилиндр
    m_showSideCheck = new wxCheckBox(controlPanel, wxID_ANY, wxString::FromUTF8("Показать боковой цилиндр"));
    m_showSideCheck->SetForegroundColour(*wxWHITE);
    m_showSideCheck->Enable(false);
    controlSizer->Add(m_showSideCheck, 0, wxRIGHT, 15);

    controlSizer->AddStretchSpacer();

    // Счётчик строк
    m_rowCountText = new wxStaticText(controlPanel, wxID_ANY, wxString::FromUTF8("Строк: 0"));
    m_rowCountText->SetForegroundColour(*wxWHITE);
    controlSizer->Add(m_rowCountText, 0, wxALIGN_CENTER_VERTICAL);

    controlPanel->SetSizer(controlSizer);
    mainSizer->Add(controlPanel, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

    // Таблица
    m_grid = new wxGrid(this, wxID_ANY);
    m_grid->CreateGrid(0, 0);
    m_grid->SetBackgroundColour(wxColour(0x25, 0x2A, 0x30));
    m_grid->SetLabelBackgroundColour(wxColour(0x18, 0x1C, 0x22));
    m_grid->SetLabelTextColour(wxColour(0xE0, 0xE0, 0xE0));
    m_grid->SetGridLineColour(wxColour(0x40, 0x45, 0x4D));
    m_grid->SetDefaultCellBackgroundColour(wxColour(0x25, 0x2A, 0x30));
    m_grid->SetDefaultCellTextColour(wxColour(0xF0, 0xF0, 0xF0));
    m_grid->SetDefaultRenderer(new wxGridCellFloatRenderer(6, 6));
    m_grid->EnableEditing(false);
    m_grid->EnableDragGridSize(false);
    m_grid->EnableDragColSize(true);
    m_grid->EnableDragRowSize(false);

    mainSizer->Add(m_grid, 1, wxEXPAND | wxALL, 5);
    SetSizer(mainSizer);
}

void KinematicTablePanel::SetData(const CalculationResults* results, const EngineParams* params) {
    m_results = results;
    m_params = params;

    if (!m_results || !m_params) {
        Clear();
        return;
    }

    // Определяем, V-образный ли двигатель
    bool isVType = (m_params->gamma != 0.0 || m_params->gammaPric != 0.0);
    int numItems = isVType ? (static_cast<int>(m_params->countCyl) / 2) : static_cast<int>(m_params->countCyl);
    wxString itemLabel = isVType ? wxString::FromUTF8("Ряд:") : wxString::FromUTF8("Цилиндр:");
    wxString itemName = isVType ? wxString::FromUTF8("Ряд %d") : wxString::FromUTF8("Цилиндр %d");

    // Обновляем метку
    m_cylinderLabel->SetLabel(itemLabel);

    // Заполняем выпадающий список
    m_cylinderChoice->Clear();
    for (int i = 0; i < numItems; ++i) {
        m_cylinderChoice->Append(wxString::Format(itemName, i + 1));
    }
    m_cylinderChoice->SetSelection(0);
    m_cylinderChoice->Enable(true);

    // Проверяем наличие бокового цилиндра
    bool hasSide = isVType && !m_results->cylinder_stroke_full_side.empty();
    m_showSideCheck->Enable(hasSide);
    // Если боковой цилиндр есть, но двигатель не V-образный (маловероятно), всё равно блокируем
    if (!isVType) m_showSideCheck->Enable(false);

    UpdateTable();
}

void KinematicTablePanel::Clear() {
    if (m_grid->GetNumberRows() > 0) m_grid->DeleteRows(0, m_grid->GetNumberRows());
    if (m_grid->GetNumberCols() > 0) m_grid->DeleteCols(0, m_grid->GetNumberCols());
    m_cylinderChoice->Enable(false);
    m_showSideCheck->Enable(false);
    m_rowCountText->SetLabel(wxString::FromUTF8("Строк: 0"));
}

wxString KinematicTablePanel::FormatDouble(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return wxString(ss.str());
}

void KinematicTablePanel::UpdateTable() {
    if (!m_results || !m_params || m_results->alpha.empty()) return;

    int selectedIndex = m_cylinderChoice->GetSelection();
    if (selectedIndex == wxNOT_FOUND) return;

    bool isVType = (m_params->gamma != 0.0 || m_params->gammaPric != 0.0);
    bool showSide = m_showSideCheck->IsChecked() && m_showSideCheck->IsEnabled();

    // Заголовки колонок
    std::vector<wxString> columns = {
        wxString::FromUTF8("α, град"),
        wxString::FromUTF8("S полн, м"),
        wxString::FromUTF8("S₁, м"),
        wxString::FromUTF8("S₂, м"),
        wxString::FromUTF8("V полн, м/с"),
        wxString::FromUTF8("V₁, м/с"),
        wxString::FromUTF8("V₂, м/с"),
        wxString::FromUTF8("A полн, м/с²"),
        wxString::FromUTF8("A₁, м/с²"),
        wxString::FromUTF8("A₂, м/с²"),
        wxString::FromUTF8("β, рад"),
        wxString::FromUTF8("ω, рад/с"),
        wxString::FromUTF8("ε, рад/с²")
    };

    if (showSide) {
        columns.push_back(wxString::FromUTF8("S бок, м"));
        columns.push_back(wxString::FromUTF8("V бок, м/с"));
        columns.push_back(wxString::FromUTF8("A бок, м/с²"));
        columns.push_back(wxString::FromUTF8("β бок, рад"));
        columns.push_back(wxString::FromUTF8("ω бок, рад/с"));
        columns.push_back(wxString::FromUTF8("ε бок, рад/с²"));
    }

    if (m_grid->GetNumberRows() > 0) m_grid->DeleteRows(0, m_grid->GetNumberRows());
    if (m_grid->GetNumberCols() > 0) m_grid->DeleteCols(0, m_grid->GetNumberCols());

    m_grid->AppendCols(columns.size());
    for (size_t i = 0; i < columns.size(); ++i) {
        m_grid->SetColLabelValue(i, columns[i]);
        m_grid->SetColSize(i, 100);
    }

    // Данные для выбранного ряда (индекс = selectedIndex)
    const auto& alpha = m_results->alpha;
    const auto& sf = m_results->cylinder_stroke_full[selectedIndex];
    const auto& s1 = m_results->cylinder_stroke1[selectedIndex];
    const auto& s2 = m_results->cylinder_stroke2[selectedIndex];
    const auto& vf = m_results->cylinder_velocity_full[selectedIndex];
    const auto& v1 = m_results->cylinder_velocity1[selectedIndex];
    const auto& v2 = m_results->cylinder_velocity2[selectedIndex];
    const auto& af = m_results->cylinder_acceleration_full[selectedIndex];
    const auto& a1 = m_results->cylinder_acceleration1[selectedIndex];
    const auto& a2 = m_results->cylinder_acceleration2[selectedIndex];
    const auto& bet = m_results->cylinder_betta_rod[selectedIndex];
    const auto& om = m_results->cylinder_omega_rod[selectedIndex];
    const auto& ep = m_results->cylinder_eps_rod[selectedIndex];

    const auto* sfs = showSide ? &m_results->cylinder_stroke_full_side[selectedIndex] : nullptr;
    const auto* vfs = showSide ? &m_results->cylinder_velocity_full_side[selectedIndex] : nullptr;
    const auto* afs = showSide ? &m_results->cylinder_acceleration_full_side[selectedIndex] : nullptr;
    const auto* bts = showSide ? &m_results->cylinder_betta_rod_side[selectedIndex] : nullptr;
    const auto* oms = showSide ? &m_results->cylinder_omega_rod_side[selectedIndex] : nullptr;
    const auto* eps = showSide ? &m_results->cylinder_eps_rod_side[selectedIndex] : nullptr;

    size_t numRows = alpha.size();
    m_grid->AppendRows(numRows);

    for (size_t i = 0; i < numRows; ++i) {
        int col = 0;
        m_grid->SetCellValue(i, col++, FormatDouble(alpha[i], 3));
        m_grid->SetCellValue(i, col++, FormatDouble(sf[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(s1[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(s2[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(vf[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(v1[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(v2[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(af[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(a1[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(a2[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(bet[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(om[i], 6));
        m_grid->SetCellValue(i, col++, FormatDouble(ep[i], 6));

        if (showSide && sfs) {
            m_grid->SetCellValue(i, col++, FormatDouble((*sfs)[i], 6));
            m_grid->SetCellValue(i, col++, FormatDouble((*vfs)[i], 6));
            m_grid->SetCellValue(i, col++, FormatDouble((*afs)[i], 6));
            m_grid->SetCellValue(i, col++, FormatDouble((*bts)[i], 6));
            m_grid->SetCellValue(i, col++, FormatDouble((*oms)[i], 6));
            m_grid->SetCellValue(i, col++, FormatDouble((*eps)[i], 6));
        }
    }

    m_rowCountText->SetLabel(wxString::Format(wxString::FromUTF8("Строк: %zu"), numRows));
    m_grid->AutoSizeColumns();
}

void KinematicTablePanel::OnCylinderChanged(wxCommandEvent&) { UpdateTable(); }
void KinematicTablePanel::OnShowSideChanged(wxCommandEvent&) { UpdateTable(); }
void KinematicTablePanel::OnGridCellClick(wxGridEvent& evt) { evt.Skip(); }
void KinematicTablePanel::OnGridLabelClick(wxGridEvent& evt) { evt.Skip(); }