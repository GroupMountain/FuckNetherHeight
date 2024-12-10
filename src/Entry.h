#include <span> // temporarily fix the workflows build

#include <ll/api/mod/NativeMod.h>
#include <ll/api/mod/RegisterHelper.h>

namespace FuckNetherHeight {

class Entry {

public:
    static std::unique_ptr<Entry>& getInstance();

    Entry(ll::mod::NativeMod& self) : mSelf(self) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();

    bool enable();

    bool disable();

private:
    ll::mod::NativeMod& mSelf;
};

} // namespace FuckNetherHeight