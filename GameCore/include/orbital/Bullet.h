#pragma once

#include "orbital/Math.h"
#include <cstdint>
#include <vector>

namespace orbital
{
enum class Pattern { Shell, Helix, Lattice };
struct Bullet
{
    std::uint64_t id = 0;
    Vec3 position;
    Vec3 velocity;
    double age = 0.0;
    double radius = 0.65;
    bool dangerous = false;
    Pattern pattern = Pattern::Shell;
};

inline double closestDistance(Vec3 relativePosition, Vec3 relativeVelocity, double horizon)
{
    const double speedSquared = dot(relativeVelocity, relativeVelocity);
    const double time = speedSquared > 1e-12
        ? std::clamp(-dot(relativePosition, relativeVelocity) / speedSquared, 0.0, horizon) : 0.0;
    return length(relativePosition + relativeVelocity * time);
}

// Relative swept spheres prevent fast bullets from tunnelling between fixed ticks.
inline bool sweptHit(Vec3 bulletStart, Vec3 bulletEnd, Vec3 playerStart, Vec3 playerEnd, double combinedRadius)
{
    const Vec3 start = bulletStart - playerStart;
    const Vec3 displacement = (bulletEnd - playerEnd) - start;
    return closestDistance(start, displacement, 1.0) <= combinedRadius;
}

inline bool shellOpening(Vec3 direction)
{
    // Two oblique passages: discoverable by observing from the side.
    return dot(direction, normalized({-0.86, 0.42, 0.26})) > std::cos(0.30)
        || dot(direction, normalized({-0.30, -0.80, 0.50})) > std::cos(0.24);
}

inline void emitShell(std::vector<Bullet>& bullets, Vec3 boss, std::uint64_t& nextId, unsigned density = 1)
{
    const unsigned Samples = 576 * density;
    constexpr double GoldenAngle = 2.399963229728653;
    for (unsigned i = 0; i < Samples; ++i)
    {
        const double z = 1.0 - 2.0 * (i + 0.5) / Samples;
        const double radial = std::sqrt(1.0 - z * z);
        const double angle = i * GoldenAngle;
        const Vec3 direction{radial * std::cos(angle), radial * std::sin(angle), z};
        if (shellOpening(direction)) continue;
        bullets.push_back({nextId++, boss + direction * 6.0, direction * 10.0});
    }
}

inline void emitHelix(std::vector<Bullet>& bullets, Vec3 boss, std::uint64_t& nextId, double phase, unsigned density = 1)
{
    for (unsigned i = 0; i < 96 * density; ++i)
    {
        const double angle = i * (6.283185307179586 / (96.0 * density));
        // An offset ring creates a moving tunnel, with a distinct rotating slit.
        if (i < 6 * density) continue;
        Vec3 position = rotate({-6, 26 + 26 * std::cos(angle), 26 * std::sin(angle)}, {1, 0, 0}, phase);
        bullets.push_back({nextId++, boss + position, {-12, -position.z * 0.35, position.y * 0.35}, 0, 0.65, false, Pattern::Helix});
    }
}

inline void emitLattice(std::vector<Bullet>& bullets, Vec3 boss, std::uint64_t& nextId, unsigned wave, unsigned density = 1)
{
    const Frame frame = turnTowards(Frame{}, {-1, 0.35, 0.55}, 3.141592653589793);
    const double gapRight = wave % 2 ? -18.0 : 18.0;
    const int extent = static_cast<int>(15 * std::sqrt(density));
    const double spacing = 60.0 / extent;
    for (int u = -extent; u <= extent; ++u) for (int v = -extent; v <= extent; ++v)
    {
        const double right = u * spacing, up = v * spacing;
        if (std::hypot(right - gapRight, up + 12) < 12) continue;
        const Vec3 position = boss + frame.forward * 6 + frame.right * right + frame.up * up;
        bullets.push_back({nextId++, position, frame.forward * 12, 0, 0.65, false, Pattern::Lattice});
    }
}
}
