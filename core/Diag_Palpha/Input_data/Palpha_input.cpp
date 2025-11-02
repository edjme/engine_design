#include "Palpha_input.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>

using namespace std;

UnwrapParams IndUnwrapInput::loadParamsWithMenu()
{
    UnwrapParams p;

    cout << "Выберите режим ввода данных:\n";
    cout << "1 — Использовать параметры по умолчанию\n";
    cout << "2 — Ввести параметры вручную\n";
    cout << "3 — Загрузить данные из файлов (кинематика + индикаторка)\n";
    cout << "Ваш выбор: ";

    int mode = 1;
    cin >> mode;

    if (mode == 2)
    {
        p.useDefaults = false;
        p.useManual = true;
        cout << "\nВведите радиус кривошипа (м): ";
        cin >> p.r;
        cout << "Введите длину шатуна (м): ";
        cin >> p.L;
        cout << "Введите λ (r/L): ";
        cin >> p.lyambda;
        cout << "Введите диаметр цилиндра D (м): ";
        cin >> p.D;
        cout << "Введите шаг угла α (град): ";
        cin >> p.step_alpha;
        cout << "Введите предел угла α (град): ";
        cin >> p.end_alpha;
        cout << "Введите показатель сжатия n₁: ";
        cin >> p.n1;
        cout << "Введите показатель расширения n₂: ";
        cin >> p.n2;
    }
    else if (mode == 3)
    {
        p.useDefaults = false;
        p.useFiles = true;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "\nУкажите путь к файлу результатов КШМ (.csv): ";
        getline(cin, p.kinPath);

        cout << "Укажите путь к файлу индикаторной диаграммы (.csv): ";
        getline(cin, p.indPath);
    }
    else
    {
        cout << "\nИспользуются параметры по умолчанию.\n";
        p.useDefaults = true;
    }

    return p;
}
