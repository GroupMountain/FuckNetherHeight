#pragma once

#include "Entry.h"
#include "gmlib/include_ll.h"  // IWYU pragma: keep

#define selfMod FuckNetherHeight::Entry::getInstance()
#define logger  selfMod.getSelf().getLogger()

extern void enableMod();
extern void disableMod();