#pragma once

#include <algorithm>
#include <cmath>

namespace orbital
{
// Metres, seconds, radians. X forward, Y right, Z up; conversion lives in the host.
struct Vec3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    Vec3 operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
    Vec3 operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(double s) const { return *this * (1.0 / s); }
};

inline double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline double length(Vec3 v) { return std::sqrt(dot(v, v)); }
inline bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
inline Vec3 normalized(Vec3 v, Vec3 fallback = {1.0, 0.0, 0.0})
{
    const double n = length(v);
    return n > 1e-10 && std::isfinite(n) ? v / n : fallback;
}
inline Vec3 rotate(Vec3 v, Vec3 axis, double angle)
{
    return v * std::cos(angle) + cross(axis, v) * std::sin(angle)
        + axis * (dot(axis, v) * (1.0 - std::cos(angle)));
}

struct Frame
{
    Vec3 forward{1.0, 0.0, 0.0};
    Vec3 right{0.0, 1.0, 0.0};
    Vec3 up{0.0, 0.0, 1.0};
};

// Parallel transport avoids a world-up singularity when orbiting over a pole.
inline Frame turnTowards(Frame frame, Vec3 direction, double maxAngle)
{
    const Vec3 target = normalized(direction, frame.forward);
    const double angle = std::acos(std::clamp(dot(frame.forward, target), -1.0, 1.0));
    if (angle < 1e-10) return frame;
    const Vec3 axis = normalized(cross(frame.forward, target), frame.up);
    const double step = std::min(angle, maxAngle);
    frame.forward = normalized(rotate(frame.forward, axis, step));
    frame.right = normalized(rotate(frame.right, axis, step));
    frame.up = normalized(cross(frame.forward, frame.right));
    frame.right = normalized(cross(frame.up, frame.forward));
    return frame;
}
}
