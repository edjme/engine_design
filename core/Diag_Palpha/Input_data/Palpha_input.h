#ifndef PALPHA_INPUT_H
#define PALPHA_INPUT_H

#include <string>
#include <vector>

struct UnwrapParams
{
    // Геометрические и расчётные параметры
    double r = 0.022;
    double L = 0.095;
    double lyambda = 0.2316;
    double D = 0.08;
    double step_alpha = 1.0;
    double end_alpha = 720.0;
    double n1 = 1.3;
    double n2 = 1.25;

    // Пути к файлам (если пользователь хочет загрузить)
    std::string kinPath;
    std::string indPath;

    // Данные, считанные из файлов (если есть)
    std::vector<double> alpha;
    std::vector<double> stroke;
    std::vector<double> pressures;

    bool useDefaults = true;
    bool useManual = false;
    bool useFiles = false;
};

class IndUnwrapInput
{
public:
    static UnwrapParams loadParamsWithMenu();
};

#endif
