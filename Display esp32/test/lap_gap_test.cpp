#include "../src/LapGap.h"
#include "../src/LapDisplayFormat.h"
#include <cassert>
#include <cstring>
#include <cstdio>

static LapGap gap;
static void point(int index, int count, float &x, float &y)
{
    const double angle = index * 6.283185307179586 / count;
    x = 100 * std::cos(angle);
    y = 100 * std::sin(angle);
}
static void lap(uint32_t stepMs, bool best, int count = 60)
{
    gap.startLap(100, 0);
    for (int i = 1; i < count; ++i) {
        float x, y; point(i, count, x, y);
        gap.sample(x, y, i * stepMs);
    }
    gap.finishLap(100, 0, count * stepMs, best);
}
static int32_t halfLap(uint32_t stepMs)
{
    gap.startLap(100, 0);
    for (int i = 1; i <= 30; ++i) {
        float x, y; point(i, 60, x, y);
        gap.sample(x, y, i * stepMs);
    }
    assert(gap.gapValid());
    return gap.gapMs();
}
int main()
{
    gap.reset();
    gap.startLap(100, 0);
    gap.sample(99, 10, 1000);
    assert(!gap.gapValid()); // no completed reference
    lap(1000, true);
    assert(gap.referenceCount() >= 60);
    assert(std::abs(halfLap(1000)) <= 1);
    assert(std::abs(halfLap(800) + 6000) <= 1);
    assert(std::abs(halfLap(1200) - 6000) <= 1);
    lap(1200, false); // slower lap cannot replace reference
    assert(std::abs(halfLap(1000)) <= 1);
    lap(800, true);
    assert(std::abs(halfLap(1000) - 6000) <= 1);

    // At the start, the geometrically nearby end of a lap must not be selected.
    gap.startLap(100, 0);
    gap.sample(100, 0, 1000);
    assert(gap.gapValid() && gap.gapMs() == 1000);
    // Leaving the reference corridor must hide the gap, not invent progress.
    gap.startLap(100, 0);
    gap.sample(140, 0, 1000);
    assert(!gap.gapValid());
    halfLap(1000);

    gap.invalidateLap();
    assert(!gap.gapValid() && !gap.lapValid());
    gap.sample(100, 0, 31000);
    assert(!gap.gapValid());
    assert(std::abs(halfLap(1000) - 6000) <= 1); // next lap recovers
    gap.sample(-100, 0, 34000); // stale fix / missing positions
    assert(!gap.lapValid() && !gap.gapValid());
    gap.startLap(100, 0);
    gap.sample(1000, 1000, 1000); // GPS jump
    assert(!gap.lapValid());

    gap.reset();
    lap(200, true, 2400); // bounded memory: long trace is thinned
    assert(gap.lapValid());
    assert(gap.referenceCount() <= LapGap::CAPACITY);
    assert(gap.referenceCount() >= 3);
    gap.startLap(100, 0);
    for (int i = 1; i <= 1200; ++i) {
        float x, y; point(i, 2400, x, y);
        gap.sample(x, y, i * 200);
    }
    assert(gap.gapValid() && std::abs(gap.gapMs()) < 5);
    gap.reset();
    assert(gap.referenceCount() == 0 && !gap.gapValid());

    char text[24];
    formatBestLap(text, sizeof(text), 90035);
    assert(std::strcmp(text, "1:30.035") == 0);
    formatBestLap(text, sizeof(text), 600000);
    assert(std::strcmp(text, "10:00.000") == 0);
    formatBestLap(text, sizeof(text), 0);
    assert(std::strcmp(text, "--:--.---") == 0);
    formatLapGap(text, sizeof(text), true, -385);
    assert(std::strcmp(text, "-0.385") == 0);
    formatLapGap(text, sizeof(text), true, 12345);
    assert(std::strcmp(text, "+12.345") == 0);
    formatLapGap(text, sizeof(text), false, 12345);
    assert(std::strcmp(text, "--") == 0);
    puts("PASS: reference, signs, best replacement, GPS loss/jump, bounded memory, reset and display formatting.");
}
