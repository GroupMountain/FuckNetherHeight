#include "Global.h"

namespace plugin {

Plugin::Plugin(ll::plugin::NativePlugin& self) : mSelf(self) {
    // Code for loading the plugin goes here.
    PatchNetherHeight();
}

bool Plugin::enable() {
    auto& logger = mSelf.getLogger();
    logger.info("FuckNetherHeight Loaded!");
    logger.info("Author: Tsubasa6848");
    logger.info("Repository: https://github.com/GroupMountain/FuckNetherHeight");
    return true;
}

bool Plugin::disable() {
    auto& logger = mSelf.getLogger();
    logger.info("FuckNetherHeight Disabled!");
    return true;
}

} // namespace plugin