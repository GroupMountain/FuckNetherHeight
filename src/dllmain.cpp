#include <memory>
#include "Global.h"
#include "Plugin.h"

namespace plugin {

// The global plugin instance.
std::unique_ptr<Plugin> plugin = nullptr;

extern "C" {
_declspec(dllexport) bool ll_plugin_load(ll::mod::NativeMod& self) {
    plugin = std::make_unique<plugin::Plugin>(self);

    return true;
}

/// @warning Unloading the plugin may cause a crash if the plugin has not released all of its
/// resources. If you are unsure, keep this function commented out.
// _declspec(dllexport) bool ll_plugin_unload(ll::plugin::Plugin&) {
//     plugin.reset();
//
//     return true;
// }

_declspec(dllexport) bool ll_plugin_enable(ll::mod::NativeMod&) { return plugin->enable(); }

_declspec(dllexport) bool ll_plugin_disable(ll::mod::NativeMod&) { return plugin->disable(); }
}

} // namespace plugin
