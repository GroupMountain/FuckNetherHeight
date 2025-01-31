#include "Global.h"

namespace FuckNetherHeight {

Entry& Entry::getInstance() {
    static Entry instance;
    return instance;
}

bool Entry::load() {
    if (ll::getGamingStatus() != ll::GamingStatus::Starting) {
        logger.error("FuckNetherHeight must be loaded at startup!");
        return false;
    }
    enableMod();
    return true;
}

bool Entry::enable() {
    logger.info("FuckNetherHeight Loaded!");
    logger.info("Author: GroupMountain");
    logger.info("Repository: https://github.com/GroupMountain/FuckNetherHeight");
    return true;
}

bool Entry::disable() {
    disableMod();
    return true;
}

} // namespace FuckNetherHeight

LL_REGISTER_MOD(FuckNetherHeight::Entry, FuckNetherHeight::Entry::getInstance());