#pragma once

struct float2 {
    float x, y;

    // Constructors
    float2() : x(0.0f), y(0.0f) {}
    float2(float _x, float _y) : x(_x), y(_y) {}

    // Addition
    float2 operator+(const float2& other) const {
        return {x + other.x, y + other.y};
    }

    // Subtraction
    float2 operator-(const float2& other) const {
        return {x - other.x, y - other.y};
    }

    // Scalar multiplication
    float2 operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    // Optional: magnitude
    float length_squared() const {
        return x*x + y*y;
    }
};

struct double2 {
    double x, y;

    double2() : x(0.0), y(0.0) {}
    double2(double _x, double _y) : x(_x), y(_y) {}
};