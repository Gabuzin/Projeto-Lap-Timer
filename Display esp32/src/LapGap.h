#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

// Bounded, session-only reference. Coordinates are local meters, shared by all
// laps. No filesystem access or heap allocation in the live comparison.
class LapGap
{
public:
    static constexpr size_t CAPACITY = 512;

    void reset()
    {
        _referenceCount = 0;
        startLap(0, 0);
        _currentCount = 0;
    }

    void startLap(float x, float y)
    {
        _currentCount = 1;
        _current[0] = {x, y, 0, 0};
        _intervalMs = 200;
        _validLap = true;
        _gapValid = false;
        _progress = 0;
        _previousX = x;
        _previousY = y;
        _previousMs = 0;
        _distance = 0;
    }

    void invalidateLap() { _validLap = false; _gapValid = false; }
    bool lapValid() const { return _validLap; }
    bool gapValid() const { return _gapValid; }
    int32_t gapMs() const { return _gapMs; }
    size_t referenceCount() const { return _referenceCount; }

    void sample(float x, float y, uint32_t elapsedMs, bool force = false)
    {
        if (!_validLap || _currentCount == 0) return;
        const float dx = x - _previousX, dy = y - _previousY;
        const float moved = std::sqrt(dx * dx + dy * dy);
        if (!std::isfinite(moved) || elapsedMs < _previousMs ||
            elapsedMs - _previousMs > 2500 ||
            moved > 20.0f + (elapsedMs - _previousMs) * 0.1f)
        {
            invalidateLap();
            return;
        }
        _distance += moved;
        compare(x, y, elapsedMs, dx, dy, moved);
        _previousX = x;
        _previousY = y;
        _previousMs = elapsedMs;
        const Point &last = _current[_currentCount - 1];
        if (!force && (elapsedMs - last.ms < _intervalMs || _distance - last.distance < 1.0f)) return;
        if (_currentCount == CAPACITY)
        {
            // Retain start and every other point; always append the finish.
            size_t count = 0;
            for (size_t i = 0; i < _currentCount; i += 2)
                _current[count++] = _current[i];
            _currentCount = count;
            _intervalMs *= 2;
        }
        _current[_currentCount++] = {x, y, _distance, elapsedMs};
    }

    void finishLap(float x, float y, uint32_t elapsedMs, bool newBest)
    {
        sample(x, y, elapsedMs, true);
        if (!newBest) return;
        // Never compare against a slower lap when the best has no good trace.
        _referenceCount = _validLap && _currentCount >= 3 ? _currentCount : 0;
        for (size_t i = 0; i < _referenceCount; ++i) _reference[i] = _current[i];
    }

private:
    struct Point { float x, y, distance; uint32_t ms; };
    Point _current[CAPACITY]{};
    Point _reference[CAPACITY]{};
    size_t _currentCount = 0, _referenceCount = 0;
    uint32_t _intervalMs = 200, _previousMs = 0;
    float _previousX = 0, _previousY = 0, _distance = 0, _progress = 0;
    bool _validLap = true, _gapValid = false;
    int32_t _gapMs = 0;

    void compare(float x, float y, uint32_t elapsedMs, float dx, float dy, float moved)
    {
        _gapValid = false;
        if (_referenceCount < 3) return;
        float bestScore = 1e30f, bestProgress = 0, referenceMs = 0;
        const float predicted = _progress + moved;
        for (size_t i = 1; i < _referenceCount; ++i)
        {
            const Point &a = _reference[i - 1], &b = _reference[i];
            const float vx = b.x - a.x, vy = b.y - a.y;
            const float length2 = vx * vx + vy * vy;
            if (length2 < 0.01f) continue;
            // Reject opposite-direction sections, e.g. the other side of a hairpin.
            if (moved > 2.0f && dx * vx + dy * vy <= 0) continue;
            float t = ((x - a.x) * vx + (y - a.y) * vy) / length2;
            if (t < 0) t = 0;
            if (t > 1) t = 1;
            const float progress = a.distance + t * (b.distance - a.distance);
            // Local, monotonic search prevents jumping across nearby branches.
            if (progress < _progress - 5.0f || progress > predicted + 30.0f) continue;
            const float ex = x - (a.x + t * vx), ey = y - (a.y + t * vy);
            const float distance2 = ex * ex + ey * ey;
            if (distance2 > 25.0f * 25.0f) continue;
            const float drift = progress - predicted;
            const float score = distance2 + 0.05f * drift * drift;
            if (score < bestScore)
            {
                bestScore = score;
                bestProgress = progress;
                referenceMs = a.ms + t * (b.ms - a.ms);
                _gapValid = true;
            }
        }
        if (_gapValid)
        {
            if (bestProgress > _progress) _progress = bestProgress;
            _gapMs = static_cast<int32_t>(static_cast<int64_t>(elapsedMs) -
                                         static_cast<int64_t>(referenceMs + 0.5f));
        }
    }
};
