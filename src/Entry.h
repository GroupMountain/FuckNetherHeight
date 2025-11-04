#pragma once

#include <span> // temporarily fix the workflows build

#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/RegisterHelper.h>

    namespace FuckNetherHeight {

class Entry {

public:
    static Entry& getInstance();

    Entry() : mSelf(ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return *mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    std::shared_ptr<ll::mod::NativeMod> mSelf;
};

} // namespace FuckNetherHeight