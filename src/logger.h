#pragma once

#include <FS.h>

extern const char* const LOG_TIMEZONE;

void logSetup();
void logPrintf(const char* format, ...) __attribute__((format(printf, 1, 2)));
File logOpen(bool rotated);
