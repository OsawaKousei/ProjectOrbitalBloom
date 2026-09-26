#pragma once

#include "orbital/Math.h"
#include "orbital/Bullet.h"
#include <cstdint>
#include <vector>

namespace orbital
{
enum class Mode { Action, Tactical };
enum class Anchor { Follow, Converge, Fixed };
enum class EventKind { TacticalEntered, TacticalExited, ConvergePlaced, Arrived, FollowReturned, Hit };

struct Config
{
    unsigned tickRate = 120;
    double movementSpeed = 12.0;
    double convergeSpeed = 20.0;
    double frameTurnSpeed = 8.0;
    bool enableEncounter = false;
    bool recordInputs = true;
    unsigned densityScale = 1; // Development stress comparison; not a difficulty mode.
};
struct Input { double right = 0.0; double up = 0.0; };
struct PlayerState
{
    Vec3 position{-80.0, 0.0, 0.0};
    Vec3 velocity;
    Frame frame;
};
struct BossState { Vec3 position; };
struct AnchorState
{
    Anchor kind = Anchor::Follow;
    Vec3 destination;
    Frame arrivalFrame;
    double followDistance = 80.0;
};
struct RenderSnapshot
{
    std::uint64_t tick = 0;
    double gameTime = 0.0;
    Mode mode = Mode::Action;
    PlayerState player;
    BossState boss;
    AnchorState anchor;
    std::vector<Bullet> bullets;
    unsigned hits = 0;
    double invulnerableUntil = 0.0;
    Pattern pattern = Pattern::Shell;
    bool encounterComplete = false;
};
struct Event
{
    EventKind kind;
    std::uint64_t tick;
    double realTime;
    Vec3 position;
    AnchorState anchor;
};
struct Metrics
{
    double realTime = 0.0;
    double tacticalTime = 0.0;
    double travelDistance = 0.0;
    unsigned tacticalEntries = 0;
    unsigned convergePlacements = 0;
    unsigned followReturns = 0;
    std::vector<double> tacticalIntervals;
    double patternTime[3] = {};
};

struct RecordedCommand
{
    std::uint64_t tick;
    Anchor anchor;
    Vec3 destination;
};
struct Recording
{
    Config config;
    std::vector<Input> inputs;
    std::vector<RecordedCommand> commands;
    std::vector<Vec3> trajectory;
};

class Simulation
{
public:
    explicit Simulation(Config config = {});
    void advance(double realSeconds, Input input = {});
    void setMode(Mode mode);
    bool canPlaceConverge(Vec3 destination) const;
    bool placeConverge(Vec3 destination);
    bool returnToFollow();
    const RenderSnapshot& snapshot() const { return state_; }
    const Metrics& metrics() const { return metrics_; }
    const std::vector<Event>& events() const { return events_; }
    const Config& config() const { return config_; }
    const Recording& recording() const { return recording_; }

private:
    void step(Input input);
    void record(EventKind kind);
    void updateBullets(Vec3 oldPlayerPosition, double dt);
    Config config_;
    RenderSnapshot state_;
    Metrics metrics_;
    std::vector<Event> events_;
    double accumulator_ = 0.0;
    double tacticalStart_ = 0.0;
    std::uint64_t nextBulletId_ = 1;
    Recording recording_;
};
}
