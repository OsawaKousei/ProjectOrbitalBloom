#pragma once

#include "orbital/Simulation.h"

namespace orbital
{
// Record fixed-tick input and ordered anchor commands, not thousands of render
// objects per frame. Re-simulate with the recorded configuration on this machine.
class Replay
{
public:
    explicit Replay(const Recording& recording) : recording_(recording), simulation_(playbackConfig(recording.config)) {}
    void advance(double realSeconds)
    {
        if (!std::isfinite(realSeconds) || realSeconds < 0 || finished()) return;
        watchedSeconds_ += realSeconds;
        accumulator_ += realSeconds * speed_;
        const double dt = 1.0 / recording_.config.tickRate;
        while (accumulator_ + 1e-12 >= dt && !finished())
        {
            const auto tick = simulation_.snapshot().tick;
            while (command_ < recording_.commands.size() && recording_.commands[command_].tick <= tick)
            {
                const auto& c = recording_.commands[command_++];
                simulation_.setMode(Mode::Tactical);
                if (c.anchor == Anchor::Converge) simulation_.placeConverge(c.destination);
                else simulation_.returnToFollow();
                simulation_.setMode(Mode::Action);
            }
            simulation_.advance(dt, recording_.inputs[input_++]);
            accumulator_ = std::max(0.0, accumulator_ - dt);
        }
    }
    void setSlow(bool slow) { speed_ = slow ? 0.5 : 1.0; }
    bool slow() const { return speed_ == 0.5; }
    bool finished() const { return input_ >= recording_.inputs.size(); }
    double watchedSeconds() const { return watchedSeconds_; }
    const RenderSnapshot& snapshot() const { return simulation_.snapshot(); }

private:
    static Config playbackConfig(Config c) { c.recordInputs = false; return c; }
    Recording recording_;
    Simulation simulation_;
    size_t input_ = 0;
    size_t command_ = 0;
    double accumulator_ = 0;
    double speed_ = 1;
    double watchedSeconds_ = 0;
};
}
