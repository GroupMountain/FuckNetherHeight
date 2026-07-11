add_rules("mode.debug", "mode.release")

add_repositories("groupmountain-repo https://github.com/GroupMountain/xmake-repo.git")
add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

local is_server = is_config("target_type", "server")

if is_server then
    add_requires("levilamina 26.20.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 26.20.0", {configs = {target_type = "client"}})
end

add_requires("levibuildscript 0.6.0")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end
target("FuckNetherHeight")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags("/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_cxflags(
        "-Wno-microsoft-cast",
        "-Wno-invalid-offsetof",
        "-Wno-c++2b-extensions",
        "-Wno-microsoft-include",
        "-Wno-overloaded-virtual",
        "-Wno-ignored-qualifiers",
        "-Wno-missing-field-initializers",
        "-Wno-potentially-evaluated-expression",
        "-Wno-pragma-system-header-outside-header",
        {tools = {"clang_cl"}}
    )
    set_toolchains("clang-cl")
    add_defines("NOMINMAX", "UNICODE")
    add_packages("levilamina")
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++23")
    set_symbols("debug")
    add_files("src/**.cpp")
    add_includedirs("src")
    if is_server then
        add_defines("LL_PLAT_S")
    else
        add_defines("LL_PLAT_C")
    end
