
#include <iostream>
#include <cmath>
#include <vector>
#include "Kinematic/input_data/kinematic_input.h"
#include "Kinematic/output_data/kinematic_output.h"
#include "common/common_types.h"
#include <locale>
#include <codecvt>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    system("chcp 65001 > nul");

    bool programIsOn = true;

    while (programIsOn)
    {
        // Загрузка параметров
        EngineParams params = KinematicInput::loadParamsWithMenu();
        KinematicInput::printParams(params);

        // Создаем структуру для результатов
        CalculationResults results;

        const double pi = 3.14159265358979323846;
        const double DEG_TO_RAD = pi / 180.0;
        const double r = params.radcrank;
        const double r1 = params.radcrank1;
        const double L1 = params.lengthRod1;
        const double k = params.lyambda;               // r/L
        const double L = r / k;                        // длина шатуна
        const double omega = 2 * pi * params.n / 60.0; // угловая скорость
        const double e = params.dezaxial;              // эксцентриситет
        const double z = (r != 0.0) ? e / r : 0.0;     // без деления на 0

        // Создание массива для альфа
        for (double i = 0; i <= params.end_alpha + 1e-9; i += params.step_alpha)
        {

            results.alpha.push_back(i);
        }

        cout << "\n Угловая скорость (рад./с.): " << omega << "\n \n";

        if (e == 0 && params.gamma == 0 && params.gammaPric == 0) // Аксиальный КШМ
        {
            for (size_t i = 0; i < results.alpha.size(); ++i)
            {
                double alpha_rad = results.alpha[i] * DEG_TO_RAD;
                double D = sqrt(1 - pow((r / L) * sin(alpha_rad), 2));

                // Нахождение перемещения
                double stroke_1 = r * (1 - cos(alpha_rad));                                                  // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2 = r * 0.25 * k * (1 - cos(2 * alpha_rad));                                   // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f = r + L - sqrt(pow(L, 2) - pow(r * sin(alpha_rad), 2)) - r * cos(alpha_rad); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full.push_back(stroke_f);
                results.stroke1.push_back(stroke_1);
                results.stroke2.push_back(stroke_2);

                // Нахождение скорости
                double velocity_1 = omega * r * sin(alpha_rad);                                                                                            // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2 = omega * r * 0.5 * k * sin(2 * alpha_rad);                                                                              // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f = omega * r * (sin(alpha_rad) + (r / L) * sin(alpha_rad) * cos(alpha_rad) / sqrt(1 - pow((r / L) * sin(alpha_rad), 2))); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full.push_back(velocity_f);
                results.velocity1.push_back(velocity_1);
                results.velocity2.push_back(velocity_2);

                // Нахождение ускорения

                double acceleration_1 = r * omega * omega * cos(alpha_rad);                                                                                                                                // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2 = omega * omega * r * k * cos(2 * alpha_rad);                                                                                                                        // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f = omega * omega * r * (cos(alpha_rad) + k * cos(2 * alpha_rad) / D + k * k * k * (sin(alpha_rad) * sin(alpha_rad) * cos(alpha_rad) * cos(alpha_rad)) / (D * D * D)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full.push_back(acceleration_f);
                results.acceleration1.push_back(acceleration_1);
                results.acceleration2.push_back(acceleration_2);
            }
        }
        //  ДЕЗАКСИАЛЬНЫЙ КШМ
        else if (e != 0 && params.gamma == 0 && params.gammaPric == 0)
        {
            for (size_t i = 0; i < results.alpha.size(); ++i)
            {
                double alpha_rad = results.alpha[i] * DEG_TO_RAD;

                double q;
                q = r * sin(alpha_rad) - e;
                double S;
                S = sqrt(L * L - q * q);

                // Нахождение перемещения
                double stroke_1 = r * ((1 - cos(alpha_rad)) - k * z * sin(alpha_rad));                                           // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2 = r * 0.25 * k * (1 - cos(2 * alpha_rad));                                                       // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f = r * (1 - cos(alpha_rad)) + sqrt(L * L - e * e) - sqrt(L * L - pow(r * sin(alpha_rad) - e, 2)); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full.push_back(stroke_f);
                results.stroke1.push_back(stroke_1);
                results.stroke2.push_back(stroke_2);

                // Нахождение скорости
                double velocity_1 = omega * r * (sin(alpha_rad) - k * z * cos(alpha_rad));         // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2 = omega * r * 0.5 * k * sin(2 * alpha_rad);                      // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f = omega * (r * sin(alpha_rad) + (r * cos(alpha_rad)) * (q) / S); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full.push_back(velocity_f);
                results.velocity1.push_back(velocity_1);
                results.velocity2.push_back(velocity_2);

                // Нахождение ускорения

                double acceleration_1 = r * omega * omega * (cos(alpha_rad) + k * z * sin(alpha_rad));                                                                         // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2 = omega * omega * r * k * cos(2 * alpha_rad);                                                                                            // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f = omega * omega * (r * cos(alpha_rad) - (r * q * sin(alpha_rad)) / S + (r * r * L * L * cos(alpha_rad) * cos(alpha_rad)) / (S * S * S)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full.push_back(acceleration_f);
                results.acceleration1.push_back(acceleration_1);
                results.acceleration2.push_back(acceleration_2);
            }
        }
        // V - ОБРАЗНЫЙ КШМ С РЯДОМ СИДЯЩИМИ ШАТУНАМИ
        else if (e == 0 && params.gamma != 0 && params.gammaPric == 0)
        {
            for (size_t i = 0; i < results.alpha.size(); ++i)
            {
                double alpha_rad = results.alpha[i] * DEG_TO_RAD;
                double gamma_rad = params.gamma * DEG_TO_RAD;
                double D = sqrt(1 - pow((r / L) * sin(alpha_rad), 2));

                // ГЛАВНЫЙ ЦИЛИНДР
                //  Нахождение перемещения
                double stroke_1 = r * (1 - cos(alpha_rad));                                                  // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2 = r * 0.25 * k * (1 - cos(2 * (alpha_rad)));                                 // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f = r + L - sqrt(pow(L, 2) - pow(r * sin(alpha_rad), 2)) - r * cos(alpha_rad); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full.push_back(stroke_f);
                results.stroke1.push_back(stroke_1);
                results.stroke2.push_back(stroke_2);

                // Нахождение скорости
                double velocity_1 = omega * r * sin(alpha_rad);                                                                                // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2 = omega * r * 0.5 * k * sin(2 * (alpha_rad));                                                                // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f = omega * r * (sin(alpha_rad) + (k)*sin(alpha_rad) * cos(alpha_rad) / sqrt(1 - pow((k)*sin(alpha_rad), 2))); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full.push_back(velocity_f);
                results.velocity1.push_back(velocity_1);
                results.velocity2.push_back(velocity_2);

                // Нахождение ускорения

                double acceleration_1 = r * omega * omega * cos(alpha_rad);                                                                                                                                // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2 = omega * omega * r * k * cos(2 * (alpha_rad));                                                                                                                      // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f = omega * omega * r * (cos(alpha_rad) + k * cos(2 * alpha_rad) / D + k * k * k * (sin(alpha_rad) * sin(alpha_rad) * cos(alpha_rad) * cos(alpha_rad)) / (D * D * D)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full.push_back(acceleration_f);
                results.acceleration1.push_back(acceleration_1);
                results.acceleration2.push_back(acceleration_2);

                // БОКОВОЙ ЦИЛИНДР
                double Dside = sqrt(1 - pow((r / L) * sin(alpha_rad - gamma_rad), 2));

                //  Нахождение перемещения
                double stroke_1_side = r * (1 - cos(alpha_rad - gamma_rad));                                                              // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2_side = r * 0.25 * k * (1 - cos(2 * (alpha_rad - gamma_rad)));                                             // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f_side = r + L - sqrt(pow(L, 2) - pow(r * sin(alpha_rad - gamma_rad), 2)) - r * cos(alpha_rad - gamma_rad); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full_side.push_back(stroke_f_side);
                results.stroke1_side.push_back(stroke_1_side);
                results.stroke2_side.push_back(stroke_2_side);

                // Нахождение скорости
                double velocity_1_side = omega * r * sin(alpha_rad - gamma_rad);                                                                                                                    // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2_side = omega * r * 0.5 * k * sin(2 * (alpha_rad - gamma_rad));                                                                                                    // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f_side = omega * r * (sin(alpha_rad - gamma_rad) + (k)*sin(alpha_rad - gamma_rad) * cos(alpha_rad - gamma_rad) / sqrt(1 - pow((k)*sin(alpha_rad - gamma_rad), 2))); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full_side.push_back(velocity_f_side);
                results.velocity1_side.push_back(velocity_1_side);
                results.velocity2_side.push_back(velocity_2_side);

                // Нахождение ускорения

                double acceleration_1_side = r * omega * omega * cos(alpha_rad - gamma_rad);                                                                                                                                                                                                              // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2_side = omega * omega * r * k * cos(2 * (alpha_rad - gamma_rad));                                                                                                                                                                                                    // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f_side = omega * omega * r * (cos(alpha_rad - gamma_rad) + k * cos(2 * (alpha_rad - gamma_rad)) / Dside + k * k * k * (sin(alpha_rad - gamma_rad) * sin(alpha_rad - gamma_rad) * cos(alpha_rad - gamma_rad) * cos(alpha_rad - gamma_rad)) / (Dside * Dside * Dside)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full_side.push_back(acceleration_f_side);
                results.acceleration1_side.push_back(acceleration_1_side);
                results.acceleration2_side.push_back(acceleration_2_side);
            }
        }

        // V - ОБРАЗНЫЙ КШМ С РЯДОМ СИДЯЩИМИ ШАТУНАМИ ДЕЗАКСИАЛЬНЫЙ
        else if (e != 0 && params.gamma != 0 && params.gammaPric == 0)
        {
            for (size_t i = 0; i < results.alpha.size(); ++i)
            {
                double alpha_rad = results.alpha[i] * DEG_TO_RAD;
                double gamma_rad = params.gamma * DEG_TO_RAD;

                double qM = r * sin(alpha_rad) - e;
                double SM = sqrt(L * L - qM * qM);

                // ГЛАВНЫЙ ЦИЛИНДР
                //  Нахождение перемещения
                double stroke_1 = r * ((1 - cos(alpha_rad)) - k * z * sin(alpha_rad));                                           // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2 = r * 0.25 * k * (1 - cos(2 * alpha_rad));                                                       // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f = r * (1 - cos(alpha_rad)) + sqrt(L * L - e * e) - sqrt(L * L - pow(r * sin(alpha_rad) - e, 2)); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full.push_back(stroke_f);
                results.stroke1.push_back(stroke_1);
                results.stroke2.push_back(stroke_2);

                // Нахождение скорости
                double velocity_1 = omega * r * (sin(alpha_rad) - k * z * cos(alpha_rad));           // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2 = omega * r * 0.5 * k * sin(2 * alpha_rad);                        // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f = omega * (r * sin(alpha_rad) + (r * cos(alpha_rad)) * (qM) / SM); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full.push_back(velocity_f);
                results.velocity1.push_back(velocity_1);
                results.velocity2.push_back(velocity_2);

                // Нахождение ускорения

                double acceleration_1 = r * pow(omega, 2) * (cos(alpha_rad) + k * z * sin(alpha_rad));                                                                              // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2 = r * pow(omega, 2) * k * cos(2 * alpha_rad);                                                                                                 // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f = omega * omega * (r * cos(alpha_rad) - (r * qM * sin(alpha_rad)) / SM + (r * r * L * L * cos(alpha_rad) * cos(alpha_rad)) / (SM * SM * SM)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full.push_back(acceleration_f);
                results.acceleration1.push_back(acceleration_1);
                results.acceleration2.push_back(acceleration_2);

                // БОКОВОЙ ЦИЛИНДР
                double qS = r * sin(alpha_rad - gamma_rad) - e;
                double SS = sqrt(L * L - qS * qS);
                //  Нахождение перемещения
                double stroke_1_side = r * ((1 - cos(alpha_rad - gamma_rad)) - k * z * sin(alpha_rad - gamma_rad));                                           // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2_side = r * 0.25 * k * (1 - cos(2 * (alpha_rad - gamma_rad)));                                                                 // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f_side = r * (1 - cos(alpha_rad - gamma_rad)) + sqrt(L * L - e * e) - sqrt(L * L - pow(r * sin(alpha_rad - gamma_rad) - e, 2)); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full_side.push_back(stroke_f_side);
                results.stroke1_side.push_back(stroke_1_side);
                results.stroke2_side.push_back(stroke_2_side);

                // Нахождение скорости
                double velocity_1_side = omega * r * (sin(alpha_rad - gamma_rad) - k * z * cos(alpha_rad - gamma_rad));           // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2_side = omega * r * 0.5 * k * sin(2 * (alpha_rad - gamma_rad));                                  // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f_side = omega * (r * sin(alpha_rad - gamma_rad) + (r * cos(alpha_rad - gamma_rad)) * (qS) / SS); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full_side.push_back(velocity_f_side);
                results.velocity1_side.push_back(velocity_1_side);
                results.velocity2_side.push_back(velocity_2_side);

                // Нахождение ускорения

                double acceleration_1_side = r * pow(omega, 2) * (cos(alpha_rad - gamma_rad) + k * z * sin(alpha_rad - gamma_rad));                                                                                                      // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2_side = r * pow(omega, 2) * k * cos(2 * (alpha_rad - gamma_rad));                                                                                                                                   // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f_side = omega * omega * (r * cos(alpha_rad - gamma_rad) - (r * qS * sin(alpha_rad - gamma_rad)) / SS + (r * r * L * L * cos(alpha_rad - gamma_rad) * cos(alpha_rad - gamma_rad)) / (SS * SS * SS)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full_side.push_back(acceleration_f_side);
                results.acceleration1_side.push_back(acceleration_1_side);
                results.acceleration2_side.push_back(acceleration_2_side);
            }
        }

        // V - ОБРАЗНЫЙ КШМ С ПРИЦЕПНЫМ ШАТУНОМ
        else if (e == 0 && params.gamma != 0 && params.gammaPric != 0)
        {
            for (size_t i = 0; i < results.alpha.size(); ++i)
            {
                double alpha_rad = results.alpha[i] * DEG_TO_RAD;
                double gamma_rad = params.gamma * DEG_TO_RAD;
                double gammaPric_rad = params.gammaPric * DEG_TO_RAD;

                double lyambda1 = r1 / L1;

                // ГЛАВНЫЙ ЦИЛИНДР
                double D = sqrt(1 - pow((r / L) * sin(alpha_rad), 2));

                //  Нахождение перемещения
                double stroke_1 = r * (1 - cos(alpha_rad));                                                  // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2 = r * 0.25 * k * (1 - cos(2 * alpha_rad));                                   // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f = r + L - sqrt(pow(L, 2) - pow(r * sin(alpha_rad), 2)) - r * cos(alpha_rad); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full.push_back(stroke_f);
                results.stroke1.push_back(stroke_1);
                results.stroke2.push_back(stroke_2);

                // Нахождение скорости
                double velocity_1 = omega * r * sin(alpha_rad);                                                                                // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2 = omega * r * 0.5 * k * sin(2 * alpha_rad);                                                                  // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f = omega * r * (sin(alpha_rad) + (k)*sin(alpha_rad) * cos(alpha_rad) / sqrt(1 - pow((k)*sin(alpha_rad), 2))); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full.push_back(velocity_f);
                results.velocity1.push_back(velocity_1);
                results.velocity2.push_back(velocity_2);

                // Нахождение ускорения

                double acceleration_1 = r * pow(omega, 2) * cos(alpha_rad);                                                                                                                                // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2 = r * pow(omega, 2) * k * cos(2 * alpha_rad);                                                                                                                        // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f = omega * omega * r * (cos(alpha_rad) + k * cos(2 * alpha_rad) / D + k * k * k * (sin(alpha_rad) * sin(alpha_rad) * cos(alpha_rad) * cos(alpha_rad)) / (D * D * D)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full.push_back(acceleration_f);
                results.acceleration1.push_back(acceleration_1);
                results.acceleration2.push_back(acceleration_2);

                // БОКОВОЙ ЦИЛИНДР
                double argS = alpha_rad - gamma_rad - gammaPric_rad;
                double qS = r1 * sin(argS); // e = 0 в этой ветке
                double SS = sqrt(L1 * L1 - qS * qS);

                double kPric = r / L1;
                double delta = r1 / L1;
                double tetta_rad = gamma_rad - gammaPric_rad;

                double sinBetta = k * sin(alpha_rad);
                double cosBetta = sqrt(std::max(0.0, 1.0 - sinBetta * sinBetta));

                double sinBettaPric = kPric * sin(alpha_rad - gamma_rad) - delta * (sin(tetta_rad) * cosBetta + cos(tetta_rad) * sinBetta);
                double cosBettaPric = sqrt(std::max(0.0, 1.0 - sinBettaPric * sinBettaPric));

                // Производные β' и β'' по α
                const double eps = 1e-12;
                double betap = ((r / L) * std::cos(alpha_rad)) / std::max(eps, cosBetta);
                double betapp = (-(r / L) * std::sin(alpha_rad)) / std::max(eps, cosBetta) + ((r / L) * (r / L) * std::cos(alpha_rad) * std::cos(alpha_rad) * sinBetta) / std::max(eps, cosBetta * cosBetta * cosBetta);

                // 2) Вспомогательные для (β + tetta): sin(β+tetta), cos(β+tetta)
                double sD = std::sin(tetta_rad), cD = std::cos(tetta_rad);
                double sinBpD = sinBetta * cD + cosBetta * sD;
                double cosBpD = cosBetta * cD - sinBetta * sD;

                // Производные β1' и β1'' по α
                double betap1 = (r * std::cos(alpha_rad - gamma_rad) - r1 * cosBpD * betap) / std::max(eps, (L1 * cosBettaPric));

                double betapp1 = (-r * std::sin(alpha_rad - gamma_rad) + L1 * sinBettaPric * betap1 * betap1 + r1 * sinBpD * betap * betap - r1 * cosBpD * betapp) / std::max(eps, (L1 * cosBettaPric));

                // dx'/dα
                double dxp_dalpha = -r * std::sin(alpha_rad - gamma_rad) - r1 * sinBpD * betap - L1 * sinBettaPric * betap1;

                // d²x'/dα²
                double d2xp_dalpha2 = -r * std::cos(alpha_rad - gamma_rad) - r1 * (cosBpD * betap * betap + sinBpD * betapp) - L1 * (cosBettaPric * betap1 * betap1 + sinBettaPric * betapp1);

                //  Нахождение перемещения
                double stroke_1_side = r1 * (1 - cos(alpha_rad - gamma_rad - gammaPric_rad));                                                                               // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2_side = r1 * 0.25 * lyambda1 * (1 - cos(2 * (alpha_rad - gamma_rad - gammaPric_rad)));                                                       // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f_side = r * (1 - cos(alpha_rad - gamma_rad)) + r1 * (1 - (cos(tetta_rad) * cosBetta - sin(tetta_rad) * sinBetta)) + L1 * (1 - cosBettaPric); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full_side.push_back(stroke_f_side);
                results.stroke1_side.push_back(stroke_1_side);
                results.stroke2_side.push_back(stroke_2_side);

                // Нахождение скорости
                double velocity_1_side = omega * r1 * sin(alpha_rad - gamma_rad - gammaPric_rad);                        // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2_side = omega * r1 * 0.5 * lyambda1 * sin(2 * (alpha_rad - gamma_rad - gammaPric_rad)); // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f_side = -omega * dxp_dalpha;                                                            // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full_side.push_back(velocity_f_side);
                results.velocity1_side.push_back(velocity_1_side);
                results.velocity2_side.push_back(velocity_2_side);

                // Нахождение ускорения

                double acceleration_1_side = r1 * pow(omega, 2) * cos(alpha_rad - gamma_rad - gammaPric_rad);                  // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2_side = r1 * pow(omega, 2) * lyambda1 * cos(2 * (alpha_rad - gamma_rad - gammaPric_rad)); // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f_side = -omega * omega * d2xp_dalpha2;                                                    // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full_side.push_back(acceleration_f_side);
                results.acceleration1_side.push_back(acceleration_1_side);
                results.acceleration2_side.push_back(acceleration_2_side);
            }
        }

        // V - ОБРАЗНЫЙ КШМ С ПРИЦЕПНЫМ ШАТУНОМ ДЕЗАКСИАЛЬНЫЙ
        else if (e != 0 && params.gamma != 0 && params.gammaPric != 0)
        {
            for (size_t i = 0; i < results.alpha.size(); ++i)
            {
                double alpha_rad = results.alpha[i] * DEG_TO_RAD;
                double gamma_rad = params.gamma * DEG_TO_RAD;
                double gammaPric_rad = params.gammaPric * DEG_TO_RAD;

                double argS = alpha_rad - gamma_rad - gammaPric_rad;
                double qS = r1 * sin(argS) - e; // <— e учтён
                double SS = sqrt(L1 * L1 - qS * qS);

                double lyambda1 = r1 / L1;

                double qM = r * sin(alpha_rad) - e;
                double SM = sqrt(L * L - qM * qM);

                // ГЛАВНЫЙ ЦИЛИНДР
                //  Нахождение перемещения
                double stroke_1 = r * ((1 - cos(alpha_rad)) - k * z * sin(alpha_rad));                                           // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2 = r * 0.25 * k * (1 - cos(2 * alpha_rad));                                                       // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f = r * (1 - cos(alpha_rad)) + sqrt(L * L - e * e) - sqrt(L * L - pow(r * sin(alpha_rad) - e, 2)); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full.push_back(stroke_f);
                results.stroke1.push_back(stroke_1);
                results.stroke2.push_back(stroke_2);

                // Нахождение скорости
                double velocity_1 = omega * r * (sin(alpha_rad) - k * z * cos(alpha_rad));           // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2 = omega * r * 0.5 * k * sin(2 * alpha_rad);                        // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f = omega * (r * sin(alpha_rad) + (r * cos(alpha_rad)) * (qM) / SM); // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full.push_back(velocity_f);
                results.velocity1.push_back(velocity_1);
                results.velocity2.push_back(velocity_2);

                // Нахождение ускорения

                double acceleration_1 = r * pow(omega, 2) * (cos(alpha_rad) + k * z * sin(alpha_rad));                                                                              // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2 = r * pow(omega, 2) * k * cos(2 * alpha_rad);                                                                                                 // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f = omega * omega * (r * cos(alpha_rad) - (r * qM * sin(alpha_rad)) / SM + (r * r * L * L * cos(alpha_rad) * cos(alpha_rad)) / (SM * SM * SM)); // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full.push_back(acceleration_f);
                results.acceleration1.push_back(acceleration_1);
                results.acceleration2.push_back(acceleration_2);

                // БОКОВОЙ ЦИЛИНДР

                double kPric = r / L1;
                double delta = r1 / L1;
                double tetta_rad = gamma_rad - gammaPric_rad;

                double sinBetta = k * sin(alpha_rad);
                double cosBetta = sqrt(std::max(0.0, 1.0 - sinBetta * sinBetta));

                double sinBettaPric = kPric * sin(alpha_rad - gamma_rad) - delta * (sin(tetta_rad) * cosBetta + cos(tetta_rad) * sinBetta) - e / L1;
                double cosBettaPric = sqrt(std::max(0.0, 1.0 - sinBettaPric * sinBettaPric));

                // Производные β' и β'' по α
                const double eps = 1e-12;
                double betap = ((r / L) * std::cos(alpha_rad)) / std::max(eps, cosBetta);
                double betapp = (-(r / L) * std::sin(alpha_rad)) / std::max(eps, cosBetta) + ((r / L) * (r / L) * std::cos(alpha_rad) * std::cos(alpha_rad) * sinBetta) / std::max(eps, cosBetta * cosBetta * cosBetta);

                // 2) Вспомогательные для (β + tetta): sin(β+tetta), cos(β+tetta)
                double sD = std::sin(tetta_rad), cD = std::cos(tetta_rad);
                double sinBpD = sinBetta * cD + cosBetta * sD;
                double cosBpD = cosBetta * cD - sinBetta * sD;

                // Производные β1' и β1'' по α
                double betap1 = (r * std::cos(alpha_rad - gamma_rad) - r1 * cosBpD * betap) / std::max(eps, (L1 * cosBettaPric));

                double betapp1 = (-r * std::sin(alpha_rad - gamma_rad) + L1 * sinBettaPric * betap1 * betap1 + r1 * sinBpD * betap * betap - r1 * cosBpD * betapp) / std::max(eps, (L1 * cosBettaPric));

                // dx'/dα
                double dxp_dalpha = -r * std::sin(alpha_rad - gamma_rad) - r1 * sinBpD * betap - L1 * sinBettaPric * betap1;

                // d²x'/dα²
                double d2xp_dalpha2 = -r * std::cos(alpha_rad - gamma_rad) - r1 * (cosBpD * betap * betap + sinBpD * betapp) - L1 * (cosBettaPric * betap1 * betap1 + sinBettaPric * betapp1);

                double z1 = e / r1; // для бокового
                //  Нахождение перемещения
                double stroke_1_side = r1 * ((1 - cos(alpha_rad - gamma_rad - gammaPric_rad)) - lyambda1 * z1 * sin(alpha_rad - gamma_rad - gammaPric_rad));                // нахождение перемещения поршня 1го порядка (м.)
                double stroke_2_side = r1 * 0.25 * lyambda1 * (1 - cos(2 * (alpha_rad - gamma_rad - gammaPric_rad)));                                                       // нахождение перемещения поршня 2го порядка (м.)
                double stroke_f_side = r * (1 - cos(alpha_rad - gamma_rad)) + r1 * (1 - (cos(tetta_rad) * cosBetta - sin(tetta_rad) * sinBetta)) + L1 * (1 - cosBettaPric); // нахождение полного перемещения поршня от ВМТ в зависимости от альфа (м.)

                results.stroke_full_side.push_back(stroke_f_side);
                results.stroke1_side.push_back(stroke_1_side);
                results.stroke2_side.push_back(stroke_2_side);

                // Нахождение скорости
                double velocity_1_side = omega * r1 * (sin(alpha_rad - gamma_rad - gammaPric_rad) - lyambda1 * z1 * cos(alpha_rad - gamma_rad - gammaPric_rad)); // нахождение скорости поршня 1го порядка (м./с.)
                double velocity_2_side = omega * r1 * 0.5 * lyambda1 * sin(2 * (alpha_rad - gamma_rad - gammaPric_rad));                                         // нахождение скорости поршня 2го порядка (м./с.)
                double velocity_f_side = -omega * dxp_dalpha;                                                                                                    // нахождение полной скорости поршня от ВМТ в зависимости от альфа (м./с.)

                results.velocity_full_side.push_back(velocity_f_side);
                results.velocity1_side.push_back(velocity_1_side);
                results.velocity2_side.push_back(velocity_2_side);

                // Нахождение ускорения

                double acceleration_1_side = r1 * pow(omega, 2) * (cos(alpha_rad - gamma_rad - gammaPric_rad) + lyambda1 * z1 * sin(alpha_rad - gamma_rad - gammaPric_rad)); // нахождение ускорения поршня 1го порядка (м/с^2)
                double acceleration_2_side = r1 * pow(omega, 2) * lyambda1 * cos(2 * (alpha_rad - gamma_rad - gammaPric_rad));                                               // нахождение ускорения поршня 2го порядка (м/с^2)
                double acceleration_f_side = -omega * omega * d2xp_dalpha2;                                                                                                  // нахождение полного ускорения поршня от ВМТ в зависимости от альфа (м/с^2)

                results.acceleration_full_side.push_back(acceleration_f_side);
                results.acceleration1_side.push_back(acceleration_1_side);
                results.acceleration2_side.push_back(acceleration_2_side);
            }
        }

        // Вывод первых значений для проверки
        cout << "\nПервые 3 значения для проверки:\n";
        if (params.gamma != 0)
        {
            cout << "alpha_rad[град]\tS_main[м]\tS_side[м]\tV_main[м/с]\tV_side[м/с]\tA_main[м/с²]\tA_side[м/с²]\n";
            for (int i = 0; i < min(3, (int)results.alpha.size()); ++i)
            {
                cout << results.alpha[i] << "\t"
                     << results.stroke_full[i] << "\t"
                     << results.stroke_full_side[i] << "\t"
                     << results.velocity_full[i] << "\t"
                     << results.velocity_full_side[i] << "\t"
                     << results.acceleration_full[i] << "\t"
                     << results.acceleration_full_side[i] << "\n";
            }
        }
        else
        {
            cout << "alpha_rad[град]\tS[м]\t\tV[м/с]\t\tA[м/с²]\n";
            for (int i = 0; i < min(3, (int)results.alpha.size()); ++i)
            {
                cout << results.alpha[i] << "\t"
                     << results.stroke_full[i] << "\t"
                     << results.velocity_full[i] << "\t"
                     << results.acceleration_full[i] << "\n";
            }
        }

        // Экспорт результатов
        KinematicOutput::exportWithMenu(results, params);

        int otvet;
        cout << "\n Сделать еще один расчет? \n";
        cout << " --------------- \n";
        cout << " 1 - Да \n";
        cout << " 2 - Нет \n";
        cout << " --------------- \n";
        cout << " Ваш ответ: ";
        cin >> otvet;
        switch (otvet)
        {
        case 1:
            cout << "\n Перехожу к следующему расчету... \n\n";
            programIsOn = true;
            break;

        case 2:
            cout << "\n До свидания! \n";
            programIsOn = false;
            break;
        }
    }

    return 0;
}