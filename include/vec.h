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

    float2& operator+=(const float2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    // Subtraction
    float2 operator-(const float2& other) const {
        return {x - other.x, y - other.y};
    }

    float2& operator-=(const float2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
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

struct float3 {
    float x, y, z;

    // Constructors
    float3() : x(0.0f), y(0.0f), z(0.0f) {}
    float3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // Addition
    float3 operator+(const float3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    float3& operator+=(const float3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    // Subtraction
    float3 operator-(const float3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    float3& operator-=(const float3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // Scalar multiplication
    float3 operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }

    // Optional: magnitude
    float length_squared() const {
        return x*x + y*y + z*z;
    }
};