#pragma once

#include "Entry.h" // IWYU pragma: keep

#define selfMod FuckNetherHeight::Entry::getInstance()
#define logger  selfMod.getSelf().getLogger()

extern void enableMod();
extern void disableMod();