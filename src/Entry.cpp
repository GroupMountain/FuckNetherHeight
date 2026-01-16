#include "Entry.h"
#include "ll/api/service/GamingStatus.h" // IWYU pragma: keep

namespace FuckNetherHeight {

Entry& Entry::getInstance() {
    static Entry instance;
    return instance;
}

bool Entry::load() {
#ifdef LL_PLAT_S
    if (ll::getGamingStatus() != ll::GamingStatus::Starting) {
        getSelf().getLogger().error("FuckNetherHeight must be loaded at startup!");
        return false;
    }
#endif
    return true;
}

bool Entry::enable() {
    getSelf().getLogger().info("FuckNetherHeight Loaded!");
    getSelf().getLogger().info("Author: GroupMountain");
    getSelf().getLogger().info("Repository: https://github.com/GroupMountain/FuckNetherHeight");
    return true;
}

bool Entry::disable() { return true; }

} // namespace FuckNetherHeight

LL_REGISTER_MOD(FuckNetherHeight::Entry, FuckNetherHeight::Entry::getInstance());