#include "orbital/Simulation.h"
#include "orbital/Replay.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <chrono>

using namespace orbital;
namespace
{
int checks = 0;
void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
bool near(double a, double b, double tolerance = 1e-8) { return std::abs(a - b) < tolerance; }
void checkFrame(Frame f)
{
    require(near(length(f.forward), 1) && near(length(f.right), 1) && near(length(f.up), 1), "unit frame axes");
    require(near(dot(f.forward, f.right), 0) && near(dot(f.forward, f.up), 0) && near(dot(f.right, f.up), 0), "orthogonal frame axes");
    require(length(cross(f.forward, f.right) - f.up) < 1e-8, "consistent frame handedness");
}
void cadence()
{
    Simulation a, b, c;
    for (int i = 0; i < 300; ++i) a.advance(1.0 / 30, {0.6, 0.8});
    for (int i = 0; i < 1440; ++i) b.advance(1.0 / 144, {0.6, 0.8});
    for (int i = 0; i < 100; ++i) { c.advance(0.007, {0.6, 0.8}); c.advance(0.093, {0.6, 0.8}); }
    require(a.snapshot().tick == 1200 && b.snapshot().tick == 1200 && c.snapshot().tick == 1200, "cadence-independent tick count");
    require(length(a.snapshot().player.position - b.snapshot().player.position) < 1e-8, "30 vs 144 fps");
    require(length(a.snapshot().player.position - c.snapshot().player.position) < 1e-8, "irregular frame cadence");
    Simulation sixty(Config{60});
    sixty.advance(10, {0.6, 0.8});
    require(sixty.snapshot().tick == 600, "60 Hz selectable");
    require(length(sixty.snapshot().player.position - a.snapshot().player.position) < 0.02, "60/120 Hz trajectory agreement");
}
void orbit()
{
    Simulation sim;
    for (int i = 0; i < 600; ++i)
    {
        sim.advance(0.1, {0, 1});
        require(near(length(sim.snapshot().player.position), 80), "follow maintains radius over poles");
        checkFrame(sim.snapshot().player.frame);
    }
    Simulation diagonal, straight;
    diagonal.advance(1, {1, 1}); straight.advance(1, {1, 0});
    require(near(diagonal.metrics().travelDistance, straight.metrics().travelDistance), "diagonal input speed capped");
    checkFrame(turnTowards(Frame{}, {-1, 0, 0}, 3.141592653589793));
    checkFrame(turnTowards(Frame{}, {}, 1));
}
void pause()
{
    Simulation sim;
    sim.advance(0.004);
    const auto before = sim.snapshot();
    sim.setMode(Mode::Tactical);
    sim.setMode(Mode::Tactical);
    sim.advance(300, {1, 1});
    require(sim.snapshot().tick == before.tick && length(sim.snapshot().player.position - before.player.position) == 0, "tactical fully freezes simulation");
    sim.setMode(Mode::Action);
    sim.advance(1.0 / 120 - 0.004);
    require(sim.snapshot().tick == 1, "resume retains fractional step without tactical catch-up");
    require(near(sim.metrics().tacticalTime, 300) && sim.metrics().tacticalEntries == 1, "pause metrics");
    require(sim.metrics().tacticalIntervals.size() == 1 && near(sim.metrics().tacticalIntervals[0], 300), "individual tactical interval");
    for (int i = 0; i < 10; ++i) { sim.setMode(Mode::Tactical); sim.setMode(Mode::Action); }
    require(sim.snapshot().tick == 1 && sim.metrics().tacticalEntries == 11, "repeated pausing allowed");
}
void anchors()
{
    Simulation sim;
    require(!sim.placeConverge({-40, 20, 10}), "placement action guard");
    sim.setMode(Mode::Tactical);
    require(!sim.placeConverge(sim.snapshot().player.position), "reject zero-distance anchor");
    require(!sim.placeConverge({0, 0, 0}), "reject boss centre");
    require(!sim.placeConverge({std::numeric_limits<double>::quiet_NaN(), 0, 0}), "reject invalid destination");
    const Vec3 destination{-40, 20, 10};
    const size_t eventCount = sim.events().size();
    require(sim.canPlaceConverge(destination), "placement preview accepts useful destination");
    require(sim.events().size() == eventCount && sim.snapshot().anchor.kind == Anchor::Follow, "placement preview is read-only");
    require(sim.placeConverge(destination), "accept tactical placement");
    require(length(sim.events().back().anchor.destination - destination) == 0, "placement event preserves target for replay");
    const auto tick = sim.snapshot().tick;
    sim.advance(5);
    require(sim.snapshot().tick == tick, "placing anchor does not advance time");
    sim.setMode(Mode::Action);
    sim.advance(1, {0.5, 0.2});
    require(sim.snapshot().anchor.kind == Anchor::Converge, "converge remains active en route");
    require(length(sim.snapshot().player.position - destination) < length(Vec3{-80, 0, 0} - destination), "converge progresses during evasion");
    sim.advance(4);
    require(sim.snapshot().anchor.kind == Anchor::Fixed, "automatic fixed arrival");
    require(length(sim.snapshot().player.position - destination) < 1e-8, "arrives without overshoot");
    const Frame arrival = sim.snapshot().anchor.arrivalFrame;
    sim.advance(2, {1, 0.5});
    require(near(dot(sim.snapshot().player.position - destination, arrival.forward), 0), "fixed has no depth displacement");
    checkFrame(sim.snapshot().player.frame);
    require(!sim.returnToFollow(), "follow return is tactical operation");
    sim.setMode(Mode::Tactical);
    const double radius = length(sim.snapshot().player.position);
    require(sim.returnToFollow(), "fixed to follow");
    sim.setMode(Mode::Action);
    sim.advance(5, {1, 0});
    require(near(length(sim.snapshot().player.position), radius), "follow captures current distance");
    unsigned arrivals = 0;
    for (const auto& event : sim.events()) if (event.kind == EventKind::Arrived) { ++arrivals; require(event.tick > 0, "arrival event uses completed tick"); }
    require(arrivals == 1, "arrival emits exactly once");

    Simulation evasion;
    evasion.setMode(Mode::Tactical);
    evasion.placeConverge({-50, 20, 10});
    evasion.setMode(Mode::Action);
    for (int i = 0; i < 1200 && evasion.snapshot().anchor.kind != Anchor::Fixed; ++i)
        evasion.advance(1.0 / 120, {1, 1});
    require(evasion.snapshot().anchor.kind == Anchor::Fixed, "continuous evasion can still reach anchor");
    checkFrame(evasion.snapshot().player.frame);
}
void invalidInput()
{
    Simulation sim;
    sim.advance(-1); sim.advance(std::numeric_limits<double>::infinity());
    require(sim.snapshot().tick == 0, "invalid elapsed time ignored");
    sim.advance(1, {std::numeric_limits<double>::quiet_NaN(), 0});
    require(finite(sim.snapshot().player.position), "invalid input sanitized");
    Simulation defaults(Config{0, -1, 0, -1});
    require(defaults.config().tickRate == 120 && defaults.config().convergeSpeed > 0, "invalid configuration defaults");
}

void bullets()
{
    require(sweptHit({-10, 0, 0}, {10, 0, 0}, {}, {}, 1), "swept hit catches tunnelling");
    require(!sweptHit({-10, 2, 0}, {10, 2, 0}, {}, {}, 1), "swept near miss");
    require(sweptHit({}, {}, {-10, 0, 0}, {10, 0, 0}, 1), "moving player swept hit");
    require(near(closestDistance({10, 0, 0}, {-10, 0, 0}, 1.5), 0), "approaching danger");
    require(near(closestDistance({10, 0, 0}, {10, 0, 0}, 1.5), 10), "receding danger clamps to now");
    require(near(closestDistance({10, 0, 0}, {}, 1.5), 10), "zero relative velocity");
    std::vector<Bullet> shell;
    std::uint64_t nextId = 1;
    emitShell(shell, {}, nextId);
    require(shell.size() > 500 && shell.size() < 576, "intentional holes in dense shell");
    for (const auto& b : shell)
    {
        require(near(length(b.position), 6) && near(length(b.velocity), 10), "shell initial radius and speed");
        require(!shellOpening(normalized(b.velocity)), "shell leaves both passages open");
    }
    Config config; config.enableEncounter = true;
    Simulation sim(config), other(config);
    for (int i = 0; i < 360; ++i) sim.advance(1.0 / 30);
    for (int i = 0; i < 1728; ++i) other.advance(1.0 / 144);
    require(sim.snapshot().bullets.size() == other.snapshot().bullets.size(), "bullet count cadence independence");
    require(sim.snapshot().bullets.size() > 1000 && sim.snapshot().bullets.size() < 2000, "typical shell population");
    for (size_t i = 0; i < sim.snapshot().bullets.size(); ++i)
    {
        require(sim.snapshot().bullets[i].id == other.snapshot().bullets[i].id, "stable bullet ids");
        require(length(sim.snapshot().bullets[i].position - other.snapshot().bullets[i].position) < 1e-8, "bullet trajectory cadence independence");
    }
    const auto before = sim.snapshot();
    sim.setMode(Mode::Tactical); sim.advance(100);
    require(sim.snapshot().bullets.size() == before.bullets.size(), "pause does not emit or expire bullets");
    require(length(sim.snapshot().bullets.front().position - before.bullets.front().position) == 0, "pause freezes bullet position");
    sim.setMode(Mode::Action); sim.advance(1.0 / 120);
    require(sim.snapshot().tick == before.tick + 1, "bullet resume excludes thinking time");

    // Place directly in one known shell ray before the shell arrives.
    Simulation contact(config);
    contact.setMode(Mode::Tactical);
    Vec3 target;
    for (const auto& b : shell) if (b.velocity.x < -9.5) { target = normalized(b.velocity) * 30; break; }
    require(length(target) > 0 && contact.placeConverge(target), "setup known collision trajectory");
    contact.setMode(Mode::Action); contact.advance(15);
    require(contact.snapshot().hits > 0 && near(contact.snapshot().gameTime, 15), "hits are recorded and non-terminal");
    double lastHit = -1;
    for (const auto& e : contact.events()) if (e.kind == EventKind::Hit)
    {
        const double time = static_cast<double>(e.tick) / config.tickRate;
        require(time - lastHit >= 0.6 - 1e-8, "invulnerability suppresses repeated contacts");
        lastHit = time;
    }
}

void replayAndEncounter()
{
    Config config; config.enableEncounter = true;
    Simulation sim(config);
    sim.advance(2, {0.5, 0});
    sim.setMode(Mode::Tactical); sim.advance(45);
    sim.placeConverge({-40, 12, 8}); sim.setMode(Mode::Action);
    sim.advance(5, {0, 0.3});
    sim.setMode(Mode::Tactical); sim.advance(20); sim.returnToFollow(); sim.setMode(Mode::Action);
    sim.advance(18, {0.2, 0});
    require(sim.snapshot().pattern == Pattern::Shell, "shell lasts 25 simulation seconds");
    sim.advance(5);
    require(sim.snapshot().pattern == Pattern::Helix, "helix follows shell");
    require(!sim.snapshot().bullets.empty() && sim.snapshot().bullets.front().pattern == Pattern::Helix, "helix bullets generated");
    sim.advance(25);
    require(sim.snapshot().pattern == Pattern::Lattice, "lattice follows helix");
    require(!sim.snapshot().bullets.empty() && sim.snapshot().bullets.front().pattern == Pattern::Lattice, "lattice bullets generated");
    sim.advance(30);
    require(sim.snapshot().encounterComplete && sim.snapshot().tick == 9000, "encounter ends at 75 seconds without overshoot");
    for (double time : sim.metrics().patternTime) require(near(time, 25), "each pattern receives 25 seconds");
    Replay replay(sim.recording());
    for (int i = 0; i < 900; ++i) replay.advance(1.0 / 12);
    require(replay.finished() && near(replay.snapshot().gameTime, 75), "replay removes 65 seconds of tactical thinking");
    require(length(replay.snapshot().player.position - sim.snapshot().player.position) < 1e-8, "replay reproduces anchor trajectory");
    require(replay.snapshot().hits == sim.snapshot().hits, "replay reproduces collision events");
    require(replay.snapshot().bullets.size() == sim.snapshot().bullets.size(), "replay reproduces bullet population");
    require(length(replay.snapshot().bullets.front().position - sim.snapshot().bullets.front().position) < 1e-8, "replay reproduces pattern motion");
    Replay slow(sim.recording()); slow.setSlow(true); slow.advance(2);
    require(near(slow.snapshot().gameTime, 1), "half-speed replay");
    require(near(sim.snapshot().gameTime, 75), "replay does not modify recorded run");
}

void stress()
{
    Config config; config.enableEncounter = true; config.densityScale = 3; config.recordInputs = false;
    Simulation sim(config);
    sim.advance(7.1);
    require(sim.snapshot().bullets.size() > 4500 && sim.snapshot().bullets.size() < 5500, "approximately 5000 bullets in stress configuration");
    const auto start = std::chrono::steady_clock::now();
    sim.advance(2);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "Stress: " << sim.snapshot().bullets.size() << " bullets, " << ms / 240 << " ms/core tick at 120 Hz (headless)\n";
    require(sim.recording().inputs.empty() && sim.recording().trajectory.empty(), "replay simulation can disable recursive recording");
}
}
int main()
{
    cadence(); orbit(); pause(); anchors(); invalidInput(); bullets(); replayAndEncounter(); stress();
    std::cout << "PASS: " << checks << " checks (cadence, orbit, frames, pause, anchors, bullets, collision, danger)\n";
}
