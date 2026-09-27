#pragma once

#include <FS.h>

void historySetup();
void historyLoop();
File historyOpenDay(const char* day);  // day = "YYYYMMDD" (local date)
