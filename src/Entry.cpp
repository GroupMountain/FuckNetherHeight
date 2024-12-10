#include "Global.h"

namespace FuckNetherHeight {

std::unique_ptr<Entry>& Entry::getInstance() {
    static std::unique_ptr<Entry> instance;
    return instance;
}

bool Entry::load() {
    if (ll::getServerStatus() != ll::ServerStatus::Starting) {
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