#ifndef KINEMATIC_INPUT_H
#define KINEMATIC_INPUT_H

#include <string>
#include <filesystem>
#include <vector>

namespace kinio
{

    // Стандартные значения для автогенерации входа кинематики
    struct Defaults
    {
        // Скалярные параметры (SI)
        double diam_cyl = 0.086;   // м
        double stroke = 0.086;     // м
        double conrod_len = 0.170; // м
        double pin_offset = 0.0;   // м (эксцентриситет поршневого пальца)
        int rpm = 3000;            // об/мин
        int cycle_deg = 720;       // град (4Т по умолчанию)
        double phase_tdc = 0.0;    // град
        int direction = 1;         // +1 или -1
        double mass_piston = 0.5;  // кг (на будущее)
        double mass_conrod = 0.5;  // кг (на будущее)

        // Сетка углов alpha
        double alpha_start = 0.0;  // град
        double alpha_stop = 720.0; // град (по умолчанию = cycle_deg)
        double alpha_step = 1.0;   // град (1° шаг, чтобы не плодить гигантские файлы)

        // Имя файла по умолчанию
        std::string filename = "kinematic_input.csv";
    };

    // Создаёт CSV с дефолтами, если файла нет.
    // path: папка для файла или полный путь до файла. Если path — папка, используется Defaults::filename.
    // Возвращает true, если файл создан; false, если уже существовал и оставлен без изменений.
    bool EnsureDefaultInputCSV(const std::filesystem::path &path = {});

    // Принудительно (пере)создаёт CSV с дефолтами (перезаписывает существующий).
    // Возвращает полный путь созданного файла.
    std::filesystem::path WriteDefaultInputCSV(const std::filesystem::path &path = {},
                                               const Defaults &def = Defaults());

    // Утилита: формирует вектор углов по настройкам.
    std::vector<double> MakeAlphaGrid(double start_deg, double stop_deg, double step_deg);

} // namespace kinio

#endif // KINEMATIC_INPUT_H
