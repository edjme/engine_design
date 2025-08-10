#ifndef INPUT_H
#define INPUT_H

struct Params {
    double diam_cyl;
    double stroke;
    double epsilent;
    double p_a;
    double p_r;
    double n_1;
    double n_2;
    double lymbda_z;
    double ro;
    double lyambda;
    double n;
    double m_pd;
    double r;
    double leng_rod;
    double m_rod;
    double m_2;
    double w;
    double tau;
    double count_cyl;
    double gamma;
    double diam_root_neck;
    double diam_rod_neck;
    double length_rod_neck;
    double length_root_neck;
    double depth_web;
    double fillet_rad;
    double width_web;
    double dist_axes;
    double dist_web;
    double rho_material;
};

Params input();

#endif