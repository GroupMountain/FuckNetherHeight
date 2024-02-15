#include "Global.h"

ll::Logger logger("FuckNetherHeight");

namespace plugin {

Plugin::Plugin(ll::plugin::NativePlugin& self) : mSelf(self) {
    // Code for loading the plugin goes here.
    PatchNetherHeight();
}

bool Plugin::enable() {
    logger.info("FuckNetherHeight Loaded!");
    logger.info("Author: GroupMountain");
    logger.info("Repository: https://github.com/GroupMountain/FuckNetherHeight");
    return true;
}

bool Plugin::disable() {
    logger.info("FuckNetherHeight Disabled!");
    return true;
}

} // namespace plugin