#include "orbital/Simulation.h"

namespace orbital
{
Simulation::Simulation(Config config) : config_(config)
{
    if (config_.tickRate != 60 && config_.tickRate != 120) config_.tickRate = 120;
    if (!std::isfinite(config_.movementSpeed) || config_.movementSpeed < 0.0) config_.movementSpeed = 12.0;
    if (!std::isfinite(config_.convergeSpeed) || config_.convergeSpeed <= 0.0) config_.convergeSpeed = 20.0;
    if (!std::isfinite(config_.frameTurnSpeed) || config_.frameTurnSpeed <= 0.0) config_.frameTurnSpeed = 8.0;
}

void Simulation::record(EventKind kind)
{
    events_.push_back({kind, state_.tick, metrics_.realTime, state_.player.position, state_.anchor});
}

void Simulation::setMode(Mode mode)
{
    if (state_.mode == mode) return;
    state_.mode = mode;
    // Retain the sub-tick fraction, but never add tactical thinking time to it.
    if (mode == Mode::Tactical)
    {
        tacticalStart_ = metrics_.realTime;
        ++metrics_.tacticalEntries;
        record(EventKind::TacticalEntered);
    }
    else
    {
        metrics_.tacticalIntervals.push_back(metrics_.realTime - tacticalStart_);
        record(EventKind::TacticalExited);
    }
}

bool Simulation::canPlaceConverge(Vec3 destination) const
{
    if (state_.mode != Mode::Tactical || !finite(destination)) return false;
    const double distance = length(destination - state_.player.position);
    // A bounded working volume keeps accidental ray depths useful in this prototype.
    if (distance < 0.5 || distance > 250.0 || length(destination - state_.boss.position) < 3.0
        || length(destination - state_.boss.position) > 300.0) return false;
    return true;
}

bool Simulation::placeConverge(Vec3 destination)
{
    if (!canPlaceConverge(destination)) return false;
    state_.anchor.kind = Anchor::Converge;
    state_.anchor.destination = destination;
    state_.anchor.arrivalFrame = turnTowards(state_.player.frame, destination - state_.player.position, 3.141592653589793);
    ++metrics_.convergePlacements;
    record(EventKind::ConvergePlaced);
    return true;
}

bool Simulation::returnToFollow()
{
    if (state_.mode != Mode::Tactical || state_.anchor.kind == Anchor::Follow) return false;
    state_.anchor.kind = Anchor::Follow;
    state_.anchor.followDistance = length(state_.player.position - state_.boss.position);
    ++metrics_.followReturns;
    record(EventKind::FollowReturned);
    return true;
}

void Simulation::advance(double realSeconds, Input input)
{
    if (!std::isfinite(realSeconds) || realSeconds < 0.0) return;
    metrics_.realTime += realSeconds;
    if (state_.mode == Mode::Tactical)
    {
        metrics_.tacticalTime += realSeconds;
        return;
    }
    if (!std::isfinite(input.right)) input.right = 0.0;
    if (!std::isfinite(input.up)) input.up = 0.0;
    const double magnitude = std::hypot(input.right, input.up);
    if (magnitude > 1.0) { input.right /= magnitude; input.up /= magnitude; }
    accumulator_ += realSeconds;
    const double dt = 1.0 / config_.tickRate;
    while (accumulator_ + 1e-12 >= dt)
    {
        step(input);
        accumulator_ = std::max(0.0, accumulator_ - dt);
    }
}

void Simulation::step(Input input)
{
    const double dt = 1.0 / config_.tickRate;
    ++state_.tick;
    state_.gameTime = static_cast<double>(state_.tick) / config_.tickRate;
    auto& p = state_.player;
    auto& a = state_.anchor;
    const Vec3 oldPosition = p.position;
    if (a.kind == Anchor::Follow)
    {
        const Vec3 inward = normalized(state_.boss.position - p.position);
        p.frame = turnTowards(p.frame, inward, config_.frameTurnSpeed * dt);
        Vec3 tangent = p.frame.right * input.right + p.frame.up * input.up;
        tangent = tangent - inward * dot(tangent, inward);
        const double speed = length(tangent) * config_.movementSpeed;
        if (speed > 1e-10)
        {
            const Vec3 radial = inward * -1.0;
            const double angle = speed * dt / a.followDistance;
            p.position = state_.boss.position
                + (radial * std::cos(angle) + normalized(tangent) * std::sin(angle)) * a.followDistance;
            p.frame = turnTowards(p.frame, state_.boss.position - p.position, config_.frameTurnSpeed * dt);
        }
    }
    else if (a.kind == Anchor::Converge)
    {
        p.frame = turnTowards(p.frame, a.destination - p.position, config_.frameTurnSpeed * dt);
        const Vec3 direction = normalized(a.destination - p.position, p.frame.forward);
        Vec3 lateral = p.frame.right * input.right + p.frame.up * input.up;
        lateral = lateral - direction * dot(lateral, direction);
        p.position = p.position + lateral * (config_.movementSpeed * dt);
        const Vec3 remaining = a.destination - p.position;
        if (length(remaining) <= config_.convergeSpeed * dt)
        {
            p.position = a.destination;
            a.kind = Anchor::Fixed;
            // Preserve the actual arrival plane; never normalize a zero direction.
            a.arrivalFrame = p.frame;
            record(EventKind::Arrived);
        }
        else p.position = p.position + normalized(remaining) * (config_.convergeSpeed * dt);
    }
    else
    {
        p.frame = a.arrivalFrame;
        p.position = p.position + (p.frame.right * input.right + p.frame.up * input.up) * (config_.movementSpeed * dt);
    }
    p.velocity = (p.position - oldPosition) / dt;
    metrics_.travelDistance += length(p.position - oldPosition);
}
}
