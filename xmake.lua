-- BondsOfSkyrim_Solo.dll : le menu « seul avec G » de Bonds of Skyrim (addon).
set_xmakever("3.0.0")

set_config("skyrim_vr", false)
includes("lib/commonlibsse-ng")

set_project("BondsOfSkyrim_Solo")
set_version("0.1.0")
set_languages("c++23")
set_warnings("allextra")
add_defines("UNICODE", "_UNICODE")
add_rules("mode.release")

target("BondsOfSkyrim_Solo")
    add_deps("commonlibsse-ng")
    add_rules("commonlibsse-ng.plugin", {
        name = "BondsOfSkyrim_Solo",
        author = "nozarashi001",
        description = "Bonds of Skyrim - Alone with G: your own actions menu (PrismaUI)"
    })
    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")
