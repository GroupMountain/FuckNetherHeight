#pragma once

#include "Entry.h"
#include "gmlib/include_lib.h" // IWYU pragma: keep
#include "gmlib/include_ll.h"  // IWYU pragma: keep

#define selfMod FuckNetherHeight::Entry::getInstance()
#define logger  selfMod.getSelf().getLogger()

extern void enableMod();
extern void disableMod();