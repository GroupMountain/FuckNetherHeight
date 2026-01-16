#pragma once

#include "Hooks.h"
#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/RegisterHelper.h>

namespace FuckNetherHeight {

class Entry {

public:
    static Entry& getInstance();

    Entry() : mSelf(*ll::mod::NativeMod::current()), mHooks(FuckNetherHeightHooks::getInstance()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    ll::mod::NativeMod&    mSelf;
    FuckNetherHeightHooks& mHooks;
};

} // namespace FuckNetherHeight