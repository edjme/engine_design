#include "CrankConfigPanel.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/grid.h>

#include <algorithm>
#include <cmath>

namespace {

// Нормализация угла в диапазон [0..cycle)
inline double NormalizeDeg(double a, double cycle)
{
    if (cycle <= 0.0) return a;
    a = std::fmod(a, cycle);
    if (a < 0.0) a += cycle;
    return a;
}

// Унифицированное чтение double из wxTextCtrl с поддержкой запятой
inline bool ReadDouble(wxTextCtrl* c, double& v)
{
    if (!c) return false;
    wxString s = c->GetValue();
    s.Replace(",", ".");
    return s.ToDouble(&v);
}

// Компактное числовое поле
inline wxTextCtrl* MakeNumCtrl(wxWindow* parent, const wxString& def, int w = 90)
{
    return new wxTextCtrl(parent, wxID_ANY, def, wxDefaultPosition, wxSize(w, -1));
}

} // namespace

CrankConfigPanel::CrankConfigPanel(wxWindow* parent)
    : wxPanel(parent)
{
    auto* root = new wxBoxSizer(wxVERTICAL);

    // =========================================================
    // 1) Верхняя строка: тактность / кривошипы / цилиндров на шейку
    // =========================================================
    {
        auto* top = new wxFlexGridSizer(0, 6, 8, 12);
        top->AddGrowableCol(1, 0);
        top->AddGrowableCol(3, 0);
        top->AddGrowableCol(5, 0);

        // Тактность
        top->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Тактность:")),
                 0, wxALIGN_CENTER_VERTICAL);

        wxArrayString taktOpts; taktOpts.Add("2"); taktOpts.Add("4");
        m_taktChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(80, -1), taktOpts);
        m_taktChoice->SetSelection(1); // 4 по умолчанию
        top->Add(m_taktChoice, 0, wxALIGN_CENTER_VERTICAL);

        // Число кривошипов
        top->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Число кривошипов:")),
                 0, wxALIGN_CENTER_VERTICAL);

        m_crankCount = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS);
        m_crankCount->SetRange(1, 32);
        m_crankCount->SetValue(4);
        top->Add(m_crankCount, 0, wxALIGN_CENTER_VERTICAL);

        // Цилиндров на шейку
        top->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Цилиндров на шатунную шейку:")),
                 0, wxALIGN_CENTER_VERTICAL);

        wxArrayString cylOpts; cylOpts.Add("1"); cylOpts.Add("2");
        m_cylPerPin = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(80, -1), cylOpts);
        m_cylPerPin->SetSelection(0);
        top->Add(m_cylPerPin, 0, wxALIGN_CENTER_VERTICAL);

        root->Add(top, 0, wxLEFT | wxRIGHT | wxTOP, 6);
    }

    // =========================================================
    // 2) Таблица фаз кривошипов (компактная, слева)
    // =========================================================
    {
        m_phaseLbl = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Компоновка коленчатого вала."));
        root->Add(m_phaseLbl, 0, wxLEFT | wxRIGHT | wxTOP, 10);

        m_phaseGrid = new wxGrid(this, wxID_ANY);
        m_phaseGrid->CreateGrid(1, 1);
        m_phaseGrid->SetLabelTextColour(*wxWHITE);                // <- "Фаза, град" и "Кривошип"
        m_phaseGrid->SetLabelBackgroundColour(wxColour(0x18, 0x1C, 0x22)); // фон заголовков (под тему)
        m_phaseGrid->SetRowLabelValue(0, wxString::FromUTF8("Фаза, град"));
        m_phaseGrid->SetRowLabelSize(90);
        m_phaseGrid->SetColLabelValue(0, wxString::FromUTF8("Кривошип 1"));
        m_phaseGrid->SetCellValue(0, 0, "0");
        m_phaseGrid->SetRowSize(0, 28);
        m_phaseGrid->SetColSize(0, 90);
        m_phaseGrid->SetMinSize(wxSize(-1, 70));

        // Обёртка: грид слева, справа пустое пространство
        auto* phaseRow = new wxBoxSizer(wxHORIZONTAL);
        phaseRow->Add(m_phaseGrid, 0, wxALIGN_LEFT | wxTOP, 6);
        phaseRow->AddStretchSpacer(1);
        root->Add(phaseRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);

        m_hint = new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Фазы задаются относительно 1-го кривошипа."));
        root->Add(m_hint, 0, wxLEFT | wxRIGHT | wxTOP, 4);
    }

    // =========================================================
    // 3) Выбор сочленения шатунов (компактный блок)
    // =========================================================
    {
        m_rodBox = new wxStaticBoxSizer(wxVERTICAL, this, wxString::FromUTF8("Сочленение шатунов"));

    m_rodSideBySide = new wxRadioButton(this, wxID_ANY, wxString::FromUTF8("Рядом сидящие шатуны"),
                                        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_rodArticulated = new wxRadioButton(this, wxID_ANY, wxString::FromUTF8("Прицепной шатун"));

    m_rodBox->Add(m_rodSideBySide, 0, wxBOTTOM, 4);
    m_rodBox->Add(m_rodArticulated, 0);

    m_rodBox->GetStaticBox()->SetMinSize(wxSize(330, -1));

    // --- справа: параметры прицепного
    m_attachedBox = new wxStaticBoxSizer(wxVERTICAL, this, wxString::FromUTF8("Параметры прицепного шатуна"));

    auto* ag = new wxFlexGridSizer(0, 4, 8, 12);
    ag->AddGrowableCol(1, 1);
    ag->AddGrowableCol(3, 1);

    ag->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Угол γp, град:")),
            0, wxALIGN_CENTER_VERTICAL);
    m_gammaPric = MakeNumCtrl(this, "90.0", 90);
    ag->Add(m_gammaPric, 1, wxEXPAND);

    ag->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Радиус r1, м:")),
            0, wxALIGN_CENTER_VERTICAL);
    m_r1 = MakeNumCtrl(this, "0.05", 90);
    ag->Add(m_r1, 1, wxEXPAND);

    ag->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Длина L1, м:")),
            0, wxALIGN_CENTER_VERTICAL);
    m_L1 = MakeNumCtrl(this, "0.008", 90);
    ag->Add(m_L1, 1, wxEXPAND);

    m_attachedBox->Add(ag, 0, wxEXPAND | wxALL, 8);
    m_attachedBox->GetStaticBox()->SetMinSize(wxSize(520, -1));

    // --- одна строка: слева rodBox, справа attachedBox
    auto* rodAndAttachedRow = new wxBoxSizer(wxHORIZONTAL);
    rodAndAttachedRow->Add(m_rodBox, 0, wxTOP, 10);
    rodAndAttachedRow->AddSpacer(12);
    rodAndAttachedRow->Add(m_attachedBox, 0, wxTOP, 10);
    rodAndAttachedRow->AddStretchSpacer(1);

    // ВАЖНО: attached будем скрывать/показывать через sizer item
    m_attachedBoxItem = rodAndAttachedRow->GetItem(m_attachedBox);

    root->Add(rodAndAttachedRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
}

    // =========================================================
    // 4) Опоры (компактный блок)
    // =========================================================
    {
        auto* bear = new wxStaticBoxSizer(wxHORIZONTAL, this, wxString::FromUTF8("Опоры"));

        m_fullSupport = new wxRadioButton(this, wxID_ANY, wxString::FromUTF8("Полноопорный"),
                                          wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        m_semiSupport = new wxRadioButton(this, wxID_ANY, wxString::FromUTF8("Неполноопорный"));
        m_fullSupport->SetValue(true);

        bear->Add(m_fullSupport, 0, wxRIGHT, 16);
        bear->Add(m_semiSupport, 0);

        bear->GetStaticBox()->SetMinSize(wxSize(330, -1));

        auto* bearRow = new wxBoxSizer(wxHORIZONTAL);
        bearRow->Add(bear, 0, wxTOP, 10);
        bearRow->AddStretchSpacer(1);
        root->Add(bearRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 0);
    }

    // =========================================================
    // 5) Геометрия + базовые параметры кинематики (r/λ/n/α)
    // =========================================================
    {
    // 4 колонки: label+field | label+field | label+field | label+field
    auto* geo = new wxFlexGridSizer(0, 8, 8, 12);
    geo->AddGrowableCol(1, 1);
    geo->AddGrowableCol(3, 1);
    geo->AddGrowableCol(5, 1);
    geo->AddGrowableCol(7, 1);

    // -------- строка 1: γ | e | n | (пусто)
    geo->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Угол развала, град:")),
             0, wxALIGN_CENTER_VERTICAL);
    m_gamma = MakeNumCtrl(this, "90.0", 90);
    geo->Add(m_gamma, 1, wxEXPAND);

    geo->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Дезаксиал e, м:")),
             0, wxALIGN_CENTER_VERTICAL);
    m_dezaxial = MakeNumCtrl(this, "0.0", 90);
    geo->Add(m_dezaxial, 1, wxEXPAND);

    geo->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Частота вращения, об/мин:")),
             0, wxALIGN_CENTER_VERTICAL);
    m_n = MakeNumCtrl(this, "4800", 90);
    geo->Add(m_n, 1, wxEXPAND);

    // справа в первой строке оставим место пустым, чтобы второй строкой туда встал шаг α
    geo->AddSpacer(0);
    geo->AddSpacer(0);

    // -------- строка 2: r | λ(на месте шага) | (пусто) | шаг α (под n)
    geo->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Радиус кривошипа r, м:")),
             0, wxALIGN_CENTER_VERTICAL);
    m_r = MakeNumCtrl(this, "0.020", 90);
    geo->Add(m_r, 1, wxEXPAND);

    // λ теперь там, где раньше был шаг α
    geo->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("λ:")),
             0, wxALIGN_CENTER_VERTICAL);
    m_lambda = MakeNumCtrl(this, "0.3", 70);
    geo->Add(m_lambda, 1, wxEXPAND);

    // шаг α под частоту вращения (в правом блоке второй строки)
    geo->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8("Шаг α, град:")),
             0, wxALIGN_CENTER_VERTICAL);
    m_stepAlpha = MakeNumCtrl(this, "1.0", 70);
    geo->Add(m_stepAlpha, 1, wxEXPAND);

    // две ячейки “пусто” под третий блок
    geo->AddSpacer(0);
    geo->AddSpacer(0);

    root->Add(geo, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
}

    

    SetSizer(root);

    // =========================================================
    // Bind
    // =========================================================
    if (m_crankCount) {
        m_crankCount->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) {
            RebuildPhaseGrid(m_crankCount->GetValue());
        });
    }

    if (m_cylPerPin) {
        m_cylPerPin->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
            UpdateVisibility();
        });
    }

    if (m_rodSideBySide) {
        m_rodSideBySide->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) {
            UpdateVisibility();
        });
    }

    if (m_rodArticulated) {
        m_rodArticulated->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) {
            UpdateVisibility();
        });
    }

    // первичная инициализация
    RebuildPhaseGrid(m_crankCount ? m_crankCount->GetValue() : 1);
    UpdateVisibility();
}

void CrankConfigPanel::NormalizePhases(std::vector<double>& a, double cycle)
{
    for (double& x : a)
        x = NormalizeDeg(x, cycle);
}

void CrankConfigPanel::RebuildPhaseGrid(int crankCount)
{
    if (!m_phaseGrid) return;

    crankCount = std::max(1, crankCount);

    // rows всегда 1
    if (m_phaseGrid->GetNumberRows() != 1) {
        if (m_phaseGrid->GetNumberRows() > 0)
            m_phaseGrid->DeleteRows(0, m_phaseGrid->GetNumberRows());
        m_phaseGrid->AppendRows(1);
    }

    // подгоняем колонки под crankCount
    const int curCols = m_phaseGrid->GetNumberCols();
    if (curCols < crankCount)
        m_phaseGrid->AppendCols(crankCount - curCols);
    else if (curCols > crankCount)
        m_phaseGrid->DeleteCols(crankCount, curCols - crankCount);

    m_phaseGrid->SetRowLabelValue(0, wxString::FromUTF8("Фаза, град"));
    m_phaseGrid->SetRowLabelSize(90);

    for (int c = 0; c < crankCount; ++c) {
        m_phaseGrid->SetColLabelValue(c, wxString::Format(wxString::FromUTF8("Кривошип %d"), c + 1));
        m_phaseGrid->SetColSize(c, 90);
    }

    m_phaseGrid->SetLabelTextColour(*wxWHITE);
m_phaseGrid->SetLabelBackgroundColour(wxColour(0x18, 0x1C, 0x22));
    m_phaseGrid->SetRowSize(0, 28);

    // дефолтные фазы
    FillDefaultPhases();

    // IMPORTANT: сначала AutoSize -> потом фиксируем минимальную ширину по BestSize
    m_phaseGrid->AutoSize();

    const wxSize best = m_phaseGrid->GetBestSize();
    m_phaseGrid->SetMinSize(best);
    m_phaseGrid->SetSize(best);

    Layout();
    if (GetParent()) GetParent()->Layout();
}

void CrankConfigPanel::FillDefaultPhases()
{
    if (!m_phaseGrid || !m_crankCount) return;

    const int n = std::max(1, m_crankCount->GetValue());
    const double cycle = 360.0;
    const double step = cycle / n;

    for (int c = 0; c < n; ++c)
        m_phaseGrid->SetCellValue(0, c, wxString::Format("%.0f", c * step));
}

bool CrankConfigPanel::ReadPhaseGrid(std::vector<double>& out, wxString& err) const
{
    out.clear();

    if (!m_phaseGrid || !m_crankCount) {
        err = wxString::FromUTF8("Таблица фаз не создана.");
        return false;
    }

    const int n = std::max(1, m_crankCount->GetValue());
    out.resize(n, 0.0);

    for (int c = 0; c < n; ++c) {
        wxString cell = m_phaseGrid->GetCellValue(0, c);
        cell.Replace(",", ".");
        double v = 0.0;
        if (!cell.ToDouble(&v)) {
            err = wxString::Format(wxString::FromUTF8("Некорректная фаза кривошипа %d."), c + 1);
            return false;
        }
        out[c] = v;
    }

    NormalizePhases(out, 360.0);
    return true;
}

void CrankConfigPanel::UpdateVisibility()
{
    const bool twoCyl = (m_cylPerPin && m_cylPerPin->GetSelection() == 1);
    const bool articulated = (m_rodArticulated && m_rodArticulated->GetValue());

    // Тип шатуна доступен только при 2 цилиндрах на шейку
    if (m_rodSideBySide) {
        m_rodSideBySide->Enable(twoCyl);
        if (!twoCyl) m_rodSideBySide->SetValue(true);
    }
    if (m_rodArticulated) {
        m_rodArticulated->Enable(twoCyl);
        if (!twoCyl) m_rodArticulated->SetValue(false);
    }

    // ✅ Угол развала γ всегда доступен (нужен и для оппозитного)
    if (m_gamma) m_gamma->Enable(true);

    // Параметры прицепного — только если (2 цилиндра) и выбран прицепной
    const bool showAttached = (twoCyl && articulated);

    if (m_attachedBoxItem) {
        m_attachedBoxItem->Show(showAttached);
    } else if (m_attachedBox) {
        m_attachedBox->Show(showAttached);
    }

    Layout();
}

bool CrankConfigPanel::GetConfig(CrankConfig& out, wxString& err) const
{
    err.clear();
    out = CrankConfig{};

    // ---- Тактность
    out.taktnost = (m_taktChoice && m_taktChoice->GetSelection() == 0) ? 2 : 4;

    // ---- Кривошипы
    out.crank_count = m_crankCount ? m_crankCount->GetValue() : 1;
    out.crank_count = std::max(1, out.crank_count);

    // ---- Цилиндров на шейку
    out.cyl_per_crankpin = (m_cylPerPin && m_cylPerPin->GetSelection() == 1) ? 2 : 1;

    // ---- Тип шатуна
    if (out.cyl_per_crankpin == 2) {
        out.rod_pair = (m_rodArticulated && m_rodArticulated->GetValue())
                         ? RodPairType::Articulated
                         : RodPairType::SideBySide;
    } else {
        out.rod_pair = RodPairType::SideBySide;
    }

    // ---- Фазы кривошипов
    if (!ReadPhaseGrid(out.crank_phase_deg, err))
        return false;

    if ((int)out.crank_phase_deg.size() != out.crank_count) {
        err = wxString::Format(wxString::FromUTF8("Фаз должно быть %d."), out.crank_count);
        return false;
    }

    // ---- Базовые кинематические параметры
    double stepA = 0.0, r = 0.0, lam = 0.0, n = 0.0;

    if (!ReadDouble(m_stepAlpha, stepA) || stepA <= 0.0 || stepA > 360.0) {
        err = wxString::FromUTF8("Некорректный шаг α.");
        return false;
    }
    if (!ReadDouble(m_r, r) || r <= 0.0 || r > 1.0) {
        err = wxString::FromUTF8("Некорректный радиус r.");
        return false;
    }
    if (!ReadDouble(m_lambda, lam) || lam <= 0.0 || lam >= 1.0) {
        err = wxString::FromUTF8("Некорректная λ (должно быть 0 < λ < 1).");
        return false;
    }
    if (!ReadDouble(m_n, n) || n <= 0.0 || n > 20000.0) {
        err = wxString::FromUTF8("Некорректная частота n.");
        return false;
    }

    out.step_alpha_deg = stepA;
    out.r_m = r;
    out.lambda = lam;
    out.n_rpm = n;

    // ---- Геометрия (γ, e)
double gamma = 0.0, dez = 0.0;

if (!ReadDouble(m_dezaxial, dez)) {
    err = wxString::FromUTF8("Некорректный дезаксиал e.");
    return false;
}

// ✅ γ читаем всегда (нужно для оппозитного даже при 1 цилиндре на шейку)
if (!ReadDouble(m_gamma, gamma)) {
    err = wxString::FromUTF8("Некорректный угол развала γ.");
    return false;
}

// простая проверка диапазона
if (gamma < 0.0 || gamma > 180.0) {
    err = wxString::FromUTF8("Угол развала γ должен быть в диапазоне 0..180.");
    return false;
}

out.gamma_deg = gamma;
out.dezaxial_m = dez;

    // ---- Прицепной шатун
    double gp = 0.0, r1 = 0.0, L1 = 0.0;
    if (out.cyl_per_crankpin == 2 && out.rod_pair == RodPairType::Articulated) {
        if (!ReadDouble(m_gammaPric, gp) || !ReadDouble(m_r1, r1) || !ReadDouble(m_L1, L1)) {
            err = wxString::FromUTF8("Некорректные параметры прицепного шатуна.");
            return false;
        }
        if (r1 <= 0.0 || L1 <= 0.0) {
            err = wxString::FromUTF8("r1 и L1 должны быть > 0.");
            return false;
        }
    }

    out.gamma_pric_deg = gp;
    out.radcrank1_m = r1;
    out.lengthRod1_m = L1;

    // ---- Опоры
    out.full_support_bearings = (m_fullSupport && m_fullSupport->GetValue());

    return true;
}

void CrankConfigPanel::SetConfig(const CrankConfig& cfg)
{
    if (m_taktChoice) m_taktChoice->SetSelection(cfg.taktnost == 2 ? 0 : 1);
    if (m_crankCount) m_crankCount->SetValue(std::max(1, cfg.crank_count));
    if (m_cylPerPin)  m_cylPerPin->SetSelection(cfg.cyl_per_crankpin == 2 ? 1 : 0);

    // шатун
    if (m_rodArticulated && m_rodSideBySide) {
        m_rodArticulated->SetValue(cfg.rod_pair == RodPairType::Articulated);
        m_rodSideBySide->SetValue(cfg.rod_pair != RodPairType::Articulated);
    }

    // таблица фаз
    RebuildPhaseGrid(std::max(1, cfg.crank_count));
    if (m_phaseGrid) {
        const int n = std::min((int)cfg.crank_phase_deg.size(), std::max(1, cfg.crank_count));
        for (int i = 0; i < n; ++i)
            m_phaseGrid->SetCellValue(0, i, wxString::Format("%.6g", cfg.crank_phase_deg[i]));
    }

    // геометрия
    if (m_gamma)     m_gamma->SetValue(wxString::Format("%.6g", cfg.gamma_deg));
    if (m_dezaxial)  m_dezaxial->SetValue(wxString::Format("%.6g", cfg.dezaxial_m));

    // прицепной
    if (m_gammaPric) m_gammaPric->SetValue(wxString::Format("%.6g", cfg.gamma_pric_deg));
    if (m_r1)        m_r1->SetValue(wxString::Format("%.6g", cfg.radcrank1_m));
    if (m_L1)        m_L1->SetValue(wxString::Format("%.6g", cfg.lengthRod1_m));

    // базовые параметры
    if (m_stepAlpha) m_stepAlpha->SetValue(wxString::Format("%.6g", cfg.step_alpha_deg));
    if (m_r)         m_r->SetValue(wxString::Format("%.6g", cfg.r_m));
    if (m_lambda)    m_lambda->SetValue(wxString::Format("%.6g", cfg.lambda));
    if (m_n)         m_n->SetValue(wxString::Format("%.6g", cfg.n_rpm));

    // опоры
    if (m_fullSupport) m_fullSupport->SetValue(cfg.full_support_bearings);
    if (m_semiSupport) m_semiSupport->SetValue(!cfg.full_support_bearings);

    UpdateVisibility();
}