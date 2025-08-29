#ifndef BALANCE_INERTIA_H
#define BALANCE_INERTIA_H

#include <string>
#include "input_data/input.h"
#include "Calculations/calc_mass_crankshaft.h"

// Результаты уравновешивания инерционных воздействий.
// Все S — статические моменты на ОДИН дополнительный вал (кг·м).
struct BalanceResults {
    bool ok = false;

    // База для отчёта
    double Fp = 0.0;              // площадь поршня, м²
    double M_pd = 0.0;            // масса поступ. частей: m_pd * Fp, кг
    double R = 0.0;               // длина кривошипа r, м
    double lambda = 0.0;          // геом. характеристика КШМ
    double omega = 0.0;           // рад/с
    long   tau = 0;               // тактность
    long   n_cyl = 0;             // число цилиндров
    double gamma_deg = 0.0;       // угол развала, град

    // Информационные (необязательные в формулах, но выводим для полноты)
    double m_root_reduce = 0.0;   // кг/м²
    double m_rotating    = 0.0;   // кг/м²

    // Плечи/расстояния из модуля масс (всегда выводим)
    double axis_p = 0.0, web_p = 0.0;  // полноопорный
    double axis_n = 0.0, web_n = 0.0;  // неполноопорный
    double c_lanch = 0.0;              // межосевое валов Ланчестера
    std::string c_lanch_note;          // текст: по какой формуле получен c_lanch

    // Итоговые статические моменты (на один вал)
    double S_prot1 = 0.0;              // 1-го порядка
    double S_prot2 = 0.0;              // 2-го порядка
    std::string S1_label;              // "от силы 1-го порядка" / "от момента 1-го порядка"
    std::string S2_label;              // "от силы 2-го порядка" / "от момента 2-го порядка"

    // Описание выбранной ветки/схемы и сообщения
    std::string mode;                  // человекочитаемое описание выбранной схемы
    std::string message;               // предупреждения/ошибки
    std::string summary;               // краткий отчёт
};

// Расчёт уравновешивания инерционных сил/моментов 1-го и 2-го порядков
// по алгоритму из твоего файла.
BalanceResults build_balance_inertia(const Params& p,
                                     const CrankshaftMassResults& cm,
                                     const std::string& out_csv = {},
                                     const std::string& out_html = {},
                                     bool auto_open_html = true);

#endif // BALANCE_INERTIA_H
