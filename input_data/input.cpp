#include <iostream>
using namespace std;

int input() {
    // Исходные данные для программы
    const double pi = 3.14159265358979323846; // число ПИ
    const double deg = pi/180;  // градусы
    double diam_cyl = 0.11; //  диаметр цилиндра в м.
    double stroke = 0.12; //    ход поршня в м.
    double epsilent = 12; //	Степень сжатия 
    double p_a = 0.09e6; // 	Давление впуска в Па
    double p_r = 0.11e6;    // 	Давление на выпуске в Па
    double n_1 = 1.38;  //  	Показатель политропы сжатия 
    double n_2 = 1.22;  //      Показатель политропы расширения 
    double lymbda_z = 2;    //  Степень повышения давления 
    double ro = 1.35;   //      Степень предварительного расширения
    double lyambda = 1/3;   //  Геометрическая характеристика КШМ
    double n = 3900;    //      Частота вращения коленчатого вала в об/мин
    double m_pd = 110;      //  Масса поступ движ частей КШМ, отнесенная к площади поршня в кг/м^2
    double r = 0.06;        //  Длина кривошипа в м
    double leng_rod = 0.18;   //Длина шатуна в м
    double m_rod = m_pd;        //Масса шатуна, отнесенная в площади поршня в кг/м^2
    double m_2 = (2/3) * m_rod;     //Масса шатуна, приведенная к оси шатунной шейки и отнесенная к площади поршня в кг/м^2
    double w = 2 * pi * n / 60;     //Скорость вращения коленчатого вала в рад/с
    double tau = 4;     //          Тактность
    double count_cyl = 2;           //Количество цилиндров 
    double gamma = 180 * deg;       //Угол развала в град
    double diam_root_neck = 0.727 * diam_cyl;       //Диаметр корен шейки в м
    double diam_rod_neck = 0.65 * diam_cyl;         //Диаметр шатунной шейки в м
    double length_rod_neck = 0.51 * diam_rod_neck;      //Длина шатунной шейки в м
    double length_root_neck = 0.66 * diam_root_neck;        //Длина коренной шейки в м
    double depth_web = 0.4 * diam_cyl;      //Толщина щеки в м
    double fillet_rad = 0.08 * depth_web;               //Радиус скругления галтели в м
    double width_web = diam_root_neck + 2 * fillet_rad;                     //Ширина щеки в м
    double dist_axes = length_rod_neck + 2 * depth_web + length_root_neck;      //Расстояние между осями цилиндров в м 
    double dist_web = 2 * length_rod_neck + 3 * depth_web + length_root_neck;       //Расстояние между щеками в м 



    cout<<"Depth web = "<<depth_web;
    
    return 0;
}
