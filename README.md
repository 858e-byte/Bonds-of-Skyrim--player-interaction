# Bonds of Skyrim — Player Interaction (Alone with G) — SKSE plugin source

Source code of `BondsOfSkyrim_Solo.dll`, the SKSE plugin of the addon "Alone with G" of the Skyrim Special Edition mod
[Bonds of Skyrim](https://www.nexusmods.com/skyrimspecialedition/mods/198013): press G with nobody in sight and a menu
of your own actions opens (eat, map, train, meditate...).

Copyright (C) 2026 nozarashi001.

## License
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
version. It is distributed WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License in [LICENSE](LICENSE).

The DLL is built with [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR) (GPL-3.0-or-later WITH Modding
Exception AND GPL-3.0 Linking Exception).

`src/PrismaUI_API.h` is the public API header provided by the PrismaUI author for mods to include, unchanged.

This repository covers the DLL only. The rest of the addon (plugin, scripts, menu page, animations) is distributed on
Nexus Mods under the permissions shown on the mod page.

## What the plugin does
- shows the addon's menu page with PrismaUI (view `AMLS_Solo/index.html`) and sends the choice back to Papyrus as the
  mod event `AMLS_UIAction`;
- the Papyrus functions of the script `AMLS_Native` (`IsLoaded`, `UIReady`, `OpenUI`, `CloseUI`, `GetFoods`,
  `GetSchoolSpell`, `GetBestWeapon`, `GetBestShield`).

## How to build (Windows)
1. Install Visual Studio 2022 (or its Build Tools) with "Desktop development with C++".
2. Install [xmake](https://xmake.io) 3.x.
3. Download CommonLibSSE-NG at commit `c5424463bba9af0d75cde8640ba7ddd4cacb9e39` (release 7.5.4) from
   https://github.com/alandtse/CommonLibVR and put it, unchanged, in `lib/commonlibsse-ng` next to `xmake.lua`.
4. In this folder:
   ```
   xmake f -m release -y
   xmake build -y
   ```
   The DLL is written to `build/windows/x64/release/BondsOfSkyrim_Solo.dll`.
