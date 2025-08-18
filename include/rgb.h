#pragma once

struct color {
    int r, g, b;
    color() : r(0), g(0), b(0) {}
    color(int _r, int _g, int _b) : r(_r), g(_g), b(_b) {}
};

inline int ColorDifferenceSquared(color a, color b){
    int dr = a.r - b.r;
    int dg = a.g - b.g;
    int db = a.b - b.b;
    int rmean = (a.r + b.r) >> 1;
    return ((512 + rmean) * dr * dr) + (1024 * dg * dg) + ((512 + (255 - rmean)) * db * db);
}