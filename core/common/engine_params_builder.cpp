#include "engine_params_builder.h"
#include <cmath>
#include <algorithm>

static double NormalizeDeg(double v, double cycle)
{
    v = std::fmod(v, cycle);
    if (v < 0) v += cycle;
    return v;
}

bool BuildEngineParamsFromCrank(const EngineBuildInput& in, EngineParams& out, std::string& err)
{
    out = EngineParams{};

    if (in.taktnost != 2 && in.taktnost != 4) { err = "taktnost must be 2 or 4"; return false; }
    const double cycle = (in.taktnost == 2) ? 360.0 : 720.0;

    if (in.crank_count <= 0) { err = "crank_count must be > 0"; return false; }
    if (in.cyl_per_crankpin != 1 && in.cyl_per_crankpin != 2) { err = "cyl_per_crankpin must be 1 or 2"; return false; }

    if ((int)in.crank_phase_deg.size() != in.crank_count) { err = "crank_phase_deg size mismatch"; return false; }

    // --- базовые ---
    out.taktnost = in.taktnost;
    out.end_alpha = (in.taktnost == 2) ? 360.0 : 720.0;
    out.cyl_per_crankpin = in.cyl_per_crankpin;
    out.articulated_rod = in.articulated_rod;
    out.full_support_bearings = in.full_support_bearings;

    // countCyl теперь вычисляется
    out.countCyl = in.crank_count * in.cyl_per_crankpin;

    // геометрия
    // γ может быть задан и при 1 цилиндре на шейку (оппозит/особые схемы)
out.gamma = in.gamma_deg;
out.dezaxial = in.dezaxial_m;

// параметры прицепного шатуна актуальны только для пары цилиндров на шейке
if (in.cyl_per_crankpin == 2 && in.articulated_rod)
{
    out.gammaPric  = in.gamma_pric_deg;
    out.radcrank1  = in.radcrank1_m;
    out.lengthRod1 = in.lengthRod1_m;
}
else
{
    out.gammaPric  = 0.0;
    out.radcrank1  = 0.0;
    out.lengthRod1 = 0.0;
}

    // --- фазы цилиндров ---
// Геометрия: цилиндры на одной шейке имеют одинаковую геометрическую фазу
out.cyl_geom_phase_deg.assign(out.countCyl, 0.0);
out.cyl_phase_deg.assign(out.countCyl, 0.0); // legacy = геометрия

for (int i = 0; i < in.crank_count; ++i)
{
    // геометрия КВ по хорошему живёт в 0..360,
    // но нормализуем по cycle (360/720), чтобы не падать от ввода пользователя
    const double ph = NormalizeDeg(in.crank_phase_deg[i], cycle);

    for (int j = 0; j < in.cyl_per_crankpin; ++j)
    {
        const int cyl = i * in.cyl_per_crankpin + j; // 0..countCyl-1
        out.cyl_geom_phase_deg[cyl] = ph;
        out.cyl_phase_deg[cyl] = ph; // legacy
    }
}

// Цикловая фаза (для динамики/давления):
// Если пользователь задал углы вспышки по цилиндрам — используем их.
// Иначе дефолт: 2Т = геометрия, 4Т = равномерно по 720.
out.cyl_cycle_phase_deg.assign(out.countCyl, 0.0);

const double cycleCycle = (out.taktnost == 2) ? 360.0 : 720.0;

if (in.cyl_cycle_phase_deg.size() == static_cast<size_t>(out.countCyl))
{
    out.has_cycle_phase = true;
    for (int c = 0; c < out.countCyl; ++c)
        out.cyl_cycle_phase_deg[c] = NormalizeDeg(in.cyl_cycle_phase_deg[c], cycleCycle);
}
else
{
    out.has_cycle_phase = false;

    if (out.taktnost == 2)
    {
        out.cyl_cycle_phase_deg = out.cyl_geom_phase_deg;
    }
    else
    {
        const double interval = 720.0 / static_cast<double>(out.countCyl);
        for (int c = 0; c < out.countCyl; ++c)
            out.cyl_cycle_phase_deg[c] = interval * static_cast<double>(c);
    }
}

    // старые layout_* оставим как совместимость (если где-то используется)
    out.layout_sections = in.crank_count;
    out.layout_rows = in.cyl_per_crankpin;

    return true;
}