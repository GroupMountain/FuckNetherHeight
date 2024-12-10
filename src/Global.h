#pragma once

#include <span> // temporarily fix the workflows build

#include "Entry.h"
#include <include_all.h>

#define selfMod FuckNetherHeight::Entry::getInstance()
#define logger  selfMod->getSelf().getLogger()

extern void enableMod();
extern void disableMod();