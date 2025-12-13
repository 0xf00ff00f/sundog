#include "lambert.h"

#include <glm/gtc/constants.hpp>

#include <array>
#include <cmath>

//
// Ported from MATLAB code by David Eagle (2025). Thanks!
//
// Source: MATLAB functions for solving Lambert's problem
// https://www.mathworks.com/matlabcentral/fileexchange/158221-matlab-functions-for-solving-lambert-s-problem)
//
// MATLAB Central File Exchange.
// Retrieved August 19, 2025.
//

namespace
{

constexpr const auto Pi = glm::pi<double>();

// this function computes the lagrange coefficient values to compute
// the final 2 velocity vectors
// input
//  mu = gravitational constant (kilometers^3/seconds^2)
//  a  =
//  s  =  semiparameter
//  c  =  chord
//  nu =  true anomaly angle (radians),
//  t  =  scaled time-of-flight parameter
//  r1 =  initial radius magnitude (kilometers)
//  r2 =  final radius magnitude (kilometers)
// output
//  fg(1) = f lagrange coefficient
//  fg(2) = g lagrange coefficient
//  fg(3) = gdot lagrange coefficient
std::tuple<double, double, double> fg_battin(double mu, double a, double s, double c, double nu, double t, double r1,
                                             double r2)
{
    double small_number = 1.0e-3;
    double f, g, gdot;
    if (a > small_number)
    {
        double be = 2.0 * std::asin((std::sqrt((s - c) / (2.0 * a))));
        if (nu > Pi)
        {
            be = -be;
        }
        double a_min = s * 5.0e-1;
        double t_min = std::sqrt(std::pow(a_min, 3.0) / mu) * (Pi - be + std::sin(be));
        double dum = (std::sqrt(s / (2.0 * a)));
        double ae = 2.0 * std::asin(dum);
        if (t > t_min)
        {
            ae = 2.0 * Pi - ae;
        }
        double de = ae - be;
        f = 1.0 - a / r1 * (1.0 - std::cos(de));
        g = t - std::sqrt(a * a * a / mu) * (de - std::sin(de));
        gdot = 1.0 - a / r2 * (1.0 - cos(de));
    }
    else if (a < -small_number)
    {
        double ah = 2.0 * std::asinh(std::sqrt(s / (-2.0 * a)));
        double bh = 2.0 * std::asinh(std::sqrt((s - c) / (-2.0 * a)));
        double dh = ah - bh;
        f = 1.0 - a / r1 * (1.0 - std::cosh(dh));
        g = t - std::sqrt(std::pow(-a, 3.0) / mu) * (std::sinh(dh) - dh);
        gdot = 1.0 - a / r2 * (1.0 - std::cosh(dh));
    }
    else
    {
        f = 0.0;
        g = 0.0;
        gdot = 0.0;
    }
    return {f, g, gdot};
}

// this function computed the k continued fraction for the battin
// lambert solution algorithm
// input
//  u = same as the u variable in the battin description
// output
//  k = continued fraction value
double k_battin(double u)
{
    static constexpr std::array<double, 21> d = {
        0.33333333333333331, 0.14814814814814814, 0.29629629629629628, 0.22222222222222221, 0.27160493827160492,
        0.23344556677890010, 0.26418026418026419, 0.23817663817663817, 0.26056644880174290, 0.24079807361541108,
        0.25842383737120578, 0.24246606855302508, 0.25700483091787441, 0.24362139917695474, 0.25599545906059318,
        0.24446916326782844, 0.25524057782122300, 0.24511784511784512, 0.25465465465465464, 0.24563024563024563,
        0.25418664443054689};
    // clang-format off
    return d[0] / (1.0 + d[1] * u / (1.0 + d[2] * u / (1.0 + d[3] * u /
           (1.0 + d[4] * u / (1.0 + d[5] * u/ (1.0 + d[6] * u /
           (1.0 + d[7] * u / (1.0 + d[8] * u / (1.0 + d[9] * u /
           (1.0 + d[10] * u / (1.0 + d[11] * u / (1.0 + d[12] * u /
           (1.0 + d[13] * u / (1.0 + d[14] * u / (1.0 + d[15] * u /
           (1.0 + d[16] * u / (1.0 + d[17] * u / (1.0 + d[18] * u /
           (1.0 + d[19] * u / (1.0 + d[20] * u))))))))))))))))))));
    // clang-format on
}

// this function computes the first continued fraction for the battin
// algorithm
// input
//  x = current x estimate
// output
//  xi = value for the continued fraction
// Orbital Mechanics with MATLAB
double xi_battin(double x)
{
    static constexpr std::array<double, 20> c = {
        0.25396825396825395, 0.25252525252525254, 0.25174825174825177, 0.25128205128205128, 0.25098039215686274,
        0.25077399380804954, 0.25062656641604009, 0.25051759834368531, 0.25043478260869567, 0.25037037037037035,
        0.25031928480204341, 0.25027808676307006, 0.25024437927663734, 0.25021645021645023, 0.25019305019305021,
        0.25017325017325015, 0.25015634771732331, 0.25014180374361883, 0.25012919896640828, 0.25011820330969264};
    double eta = x / std::pow(std::sqrt(1.0 + x) + 1.0, 2.0);
    // clang-format off
    return 8.0 * (std::sqrt(1.0 + x) + 1.0) / (3.0 + 1.0 /
           (5.0 + eta + 9.0 / 7.0 * eta / (1.0 + c[0] * eta /
           (1.0 + c[1] * eta / (1.0 + c[2] * eta / (1.0 + c[3] * eta /
           (1.0 + c[4] * eta / (1.0 + c[5] * eta / (1.0 + c[6] * eta /
           (1.0 + c[7] * eta /(1.0 + c[8] * eta / (1.0 + c[9] * eta /
           (1.0 + c[10] * eta / (1.0 + c[11] * eta / (1.0 + c[12] * eta /
           (1.0 + c[13] * eta / (1.0 + c[14] * eta / (1.0 + c[15] * eta /
           (1.0 + c[16] * eta / (1.0 + c[17] * eta / (1.0 + c[18] * eta /
           (1.0 + c[19] * eta))))))))))))))))))))));
    // clang-format on
}

} // namespace

// this function contains the battin lambert solution method
// inputs
//  mu = gravitational constant (kilometers^3/seconds^2)
//  r1 = initial position vector (kilometers)
//  r2 = final position vector (kilometers)
//  dt = transfer time (seconds)
//  ot = orbit type (1 = prograde, 2 = retrograde)
// outputs
// vi = initial velocity vector of transfer orbit (kilometers/second)
// vf = final velocity vector of transfer orbit (kilometers/second)
// Battin, R. An Introduction to the Mathematics and Methods of Astrodynamics,
// Chapter 7: Solving Lambert's Problem, AIAA Education Series,
// 1801 Alexander Bell Drive, Reston, VA, Revised Edition edition, 1999
std::optional<TransferVelocities> lambert_battin(double mu, const glm::dvec3 &r1, const glm::dvec3 &r2, double dt,
                                                 OrbitType ot)
{
    // convergence tolerance
    double tol = 1.0e-8;
    double r1mag = glm::length(r1);
    double r2mag = glm::length(r2);
    // determine true anomaly angle here (radians)
    glm::dvec3 c12 = glm::cross(r1, r2);
    double nu = std::acos(glm::dot(r1, r2) / (r1mag * r2mag));
    // determine the true anomaly angle using the orbit type
    // 1 is prograde, 2 is retrograde
    if (ot == OrbitType::Prograde)
    {
        if (c12.z <= 0.0)
        {
            nu = 2.0 * Pi - nu;
        }
    }
    if (ot == OrbitType::Retrograde)
    {
        if (c12.z >= 0.0)
        {
            nu = 2.0 * Pi - nu;
        }
    }
    double c = std::sqrt(r1mag * r1mag + r2mag * r2mag - 2.0 * r1mag * r2mag * std::cos(nu));
    double s = (r1mag + r2mag + c) / 2.0;
    double eps = (r2mag - r1mag) / r1mag;
    double lam = std::sqrt(r1mag * r2mag) * std::cos(nu * 0.5) / s;
    double t = sqrt(8.0 * mu / std::pow(s, 3.0)) * dt;
    double t_p = 4.0 / 3.0 * (1.0 - std::pow(lam, 3.0));
    double m = std::pow(t, 2.0) / std::pow(1.0 + lam, 6.0);
    double tansq2w = (eps * eps * 0.25) / (std::sqrt(r2mag / r1mag) + r2mag / r1mag * (2.0 + sqrt(r2mag / r1mag)));
    double rop = std::sqrt(r2mag * r1mag) * (std::cos(nu * 0.25) * std::cos(nu * 0.25) + tansq2w);
    double ltop, l;
    if (nu < Pi)
    {
        ltop = (std::sin(nu * 25.0e-2) * std::sin(nu * 25.0e-2) + tansq2w);
        l = ltop / (ltop + std::cos(nu * 5.0e-1));
    }
    else
    {
        ltop = std::cos(nu * 25.0e-2) * std::cos(nu * 25.0e-2) + tansq2w;
        l = (ltop - std::cos(nu * 5.0e-1)) / ltop;
    }
    // initial guess is set here
    double x, y;
    if (t <= t_p)
    {
        x = 0.0;
    }
    else
    {
        x = l;
    }
    double dx = 1.0;
    int iter = 0;
    const int itermax = 20;
    // this loop does the successive substitution
    while (dx >= tol && iter <= itermax)
    {
        double xi = xi_battin(x);
        double denom = (1.0 + 2.0 * x + l) * (4.0 * x + xi * (3.0 + x));
        double h1 = std::pow(l + x, 2.0) * (1.0 + 3.0 * x + xi) / denom;
        double h2 = (m * (x - l + xi)) / denom;
        double b = 27.0 * h2 * 25.0e-2 / std::pow(1.0 + h1, 3.0);
        double u = b / (2.0 * (std::sqrt(1.0 + b) + 1.0));
        double k = k_battin(u);
        y = (1.0 + h1) / 3.0 * (2.0 + std::sqrt(1.0 + b) / (1.0 + 2.0 * u * k * k));
        double xnew = std::sqrt(std::pow((1.0 - l) / 2.0, 2.0) + m / (y * y)) - (1.0 + l) / 2.0;
        dx = std::abs(x - xnew);
        x = xnew;
        iter = iter + 1;
    }
    if (iter > itermax)
    {
        // solution wasn't found
        return {};
    }

    double a = mu * dt * dt / (16.0 * rop * rop * x * y * y);
    auto [f, g, gdot] = fg_battin(mu, a, s, c, nu, dt, r1mag, r2mag);
    auto v1 = (r2 - f * r1) / g;
    auto v2 = (gdot * r2 - r1) / g;
    return TransferVelocities{v1, v2};
}
