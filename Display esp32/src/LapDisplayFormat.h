#pragma once
#include <cstdint>
#include <cstdio>
#include <cstddef>

inline void formatBestLap(char *text, size_t size, uint32_t milliseconds)
{
    if (milliseconds == 0) { std::snprintf(text, size, "--:--.---"); return; }
    std::snprintf(text, size, "%lu:%02lu.%03lu",
        static_cast<unsigned long>(milliseconds / 60000),
        static_cast<unsigned long>((milliseconds / 1000) % 60),
        static_cast<unsigned long>(milliseconds % 1000));
}

inline void formatLapGap(char *text, size_t size, bool valid, int32_t milliseconds)
{
    if (!valid) { std::snprintf(text, size, "--"); return; }
    const uint32_t magnitude = milliseconds < 0
        ? static_cast<uint32_t>(-static_cast<int64_t>(milliseconds))
        : static_cast<uint32_t>(milliseconds);
    std::snprintf(text, size, "%c%lu.%03lu", milliseconds < 0 ? '-' : '+',
        static_cast<unsigned long>(magnitude / 1000),
        static_cast<unsigned long>(magnitude % 1000));
}
