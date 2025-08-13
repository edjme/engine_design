#pragma once
#include <cmath>

inline double deg2rad(double deg) { return deg * M_PI / 180.0; }
inline double rad2deg(double rad) { return rad * 180.0 / M_PI; }

inline double pistonArea(double D) { return M_PI * D * D / 4.0; }

// перевод удельной массы (кг/м^2) в массу на цилиндр (кг) по площади поршня
inline double arealToMass(double m_areal, double D) {
    return m_areal * pistonArea(D);
}
