add_rules("mode.debug", "mode.release")

add_repositories("groupmountain-repo https://github.com/GroupMountain/xmake-repo.git")
add_repositories("liteldev-repo https://github.com/LiteLDev/xmake-repo.git")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

add_requires("levilaminalibrary", "gmlib")

option("pdb")
    set_default(true)
    set_description("Generate PDB files")

option("version")
    set_default("")
    set_description("Version of the mod")

target("FuckNetherHeight") -- Change this to your mod name.
    add_cxflags(
        "/EHa",
        "/utf-8"
    )
    add_defines(
        "NOMINMAX", 
        "UNICODE", 
        "_HAS_CXX17",
        "_HAS_CXX20"
    )
    add_files(
        "src/**.cpp"
    )
    add_includedirs(
        "src"
    )
    add_packages(
        "levilaminalibrary",
        "gmlib"
    )
    add_shflags(
        "/DELAYLOAD:bedrock_server.dll"
    )
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++23")
    if get_config("pdb") then
        set_symbols("debug")
    end

    after_build(function (target)
        import("lib.detect.find_file")

        local version = get_config("version")
        if version == "" then
            version = os.iorun("git describe --tags --abbrev=0 --always"):match("(%d+.%d+.%d+)")
            if not version then
                print("Failed to parse version tag, using 0.0.0")
                version = "0.0.0"
            end
        end
        local mod_define = {
            modName = target:name(),
            modFile = path.filename(target:targetfile()),
            modVersion = version
        }

        local output_dir = path.join(os.projectdir(), "bin", target:name())
        local manifest_path = path.join(output_dir, "manifest.json")
        os.cp(path.join(os.projectdir(), "manifest.json"), manifest_path)
        io.gsub(manifest_path, "%${(.-)}", function(var)
            return mod_define[var] or "${" .. var .. "}"
        end)
        os.cp(target:targetfile(), path.join(output_dir, target:name() .. ".dll"))
        if get_config("pdb") and os.isfile(target:symbolfile()) then
            os.cp(target:symbolfile(), path.join(output_dir, target:name() .. ".pdb"))
        else
            os.rm(path.join(output_dir, target:name() .. ".pdb"))
        end
    end)