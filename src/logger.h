#pragma once

#include <FS.h>
#include <time.h>

constexpr time_t MIN_VALID_EPOCH = 1704067200; // 2024-01-01, anything earlier means NTP not synced yet

extern const char* const LOG_TIMEZONE;

void logSetup();
void logPrintf(const char* format, ...) __attribute__((format(printf, 1, 2)));
File logOpen(bool rotated);
