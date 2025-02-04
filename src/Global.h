#pragma once

#include "Entry.h"
#include "include_all.h" // IWYU pragma: keep

#define selfMod FuckNetherHeight::Entry::getInstance()
#define logger  selfMod.getSelf().getLogger()

extern void enableMod();
extern void disableMod();