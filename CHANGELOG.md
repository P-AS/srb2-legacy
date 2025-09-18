# 2.1.29
### Major new additions

- Renderer Switching has (finally) been added, you can now swap between renderers ingame #124
  - To go along with that, setting the renderer via `renderer.txt` has been removed due to it being redundant with renderer switching https://github.com/P-AS/srb2-legacy/commit/fab29e1261b8e6b06c53381e9c7e8603c022d641
- Palette Rendering #126
- A new information screen has been added before joining a server #97
- Custom skincolor support #114, Try the Kart skincolors lua [here](https://file.garden/Z8jIwHTgmgo8vTto/kartcolors.lua)!
  - `netcompat` has been removed to issues with custom skincolors and general instability
  - All custom skincolor related Lua functions are exposed
- `perfstats` from 2.2 has been backported #91
- Improved color settings #94
  - Notably, this removes OpenGL gamma correction
- Changing the FOV is now supported in Software https://github.com/P-AS/srb2-legacy/commit/e7d37547433b30b55c950106990c50871a71dfa5
  - `fovchange` is supported too
- Orbital camera has been backported, can be enabled with `cam_orbit on`
- Downhill slope adjustment has been backported, enabled by default and can be toggled with `cam_adjust`
- Connect IP Menu has been replaced with a textbox https://github.com/P-AS/srb2-legacy/commit/db1f37ea68b6d8824bec70c7f43e48dee1ca8707  <!-- This is actually *not* the 2.2 version -->
- Wireframe mode in OpenGL https://github.com/P-AS/srb2-legacy/commit/592067a43e7db3af640f4aac53293ca24ef058c8
- Model blend textures have been updated to support translation colormaps https://github.com/P-AS/srb2-legacy/commit/82aa4f33149778498e58b187a91b97c32f76fc9b
  <img src="https://github.com/user-attachments/assets/1bbff964-aca6-4564-aaf3-68da5793b576" width=30%/>
- New Lua string alignment options: https://github.com/P-AS/srb2-legacy/commit/5b75724dbfb4ce047362e14b8799c468fb8f79a4
  ```
  "fixed-center",
  "fixed-right",
  "small-fixed",
  "small-fixed-center",
  "small-fixed-right",
  "small-center",
  "small-thin",
  "small-thin-center",
  "small-thin-right",
  "small-thin-fixed",
  "small-thin-fixed-center",
  "small-thin-fixed-right",
  "thin-fixed",
  "thin-fixed-center",
  "thin-fixed-right",
  "thin-center"
  ```
- Papersprites have been added, and so have `FF_PAPERSPRITE` and `MF_PAPERCOLLISION` #123
  <img src="https://github.com/user-attachments/assets/6556a837-efd3-4664-9cbc-832d7c1ad972" width=30%> </img>
- New drawer library functions
  ```
  v.getSpritePatch(string/int skin, string/int sprite, [int frame, [int rotation]])
  v.drawStretched(fixed_t x, fixed_t y, fixed_t hscale, fixed_t vscale, [patch_t]patch, [int flags, [colormap c]])
  v.fadeScreen(int color, int strength)
  ```
- Lua I/O support https://github.com/P-AS/srb2-legacy/commit/9224e8f56dd736a6f0a397ce14c2a1573b815630
  - Including the I/O library from 2.2, backwards compatible with 2.1 mods, like SUBARASHII #134
- Paired down OS Lua library #108
- Text colormaps from SRB2Kart https://github.com/P-AS/srb2-legacy/commit/53fde3604c03182fb1fb85525154c6d325e19f36
- Newer sprite rotation system which allows sprites to be displayed for the entirety of the left side or the right side #123
- New Lua command registration flags
  ```
    COM_ADMIN, 
    COM_SPLITSCREEN,
    COM_LOCAL
  ```
- `IntermissionThinker`, `MobjLineCollide`, `PlayerThink`, `GameQuit PreThinkFrame` and `PostThinkFrame` hooks
- All Lua music functions now work in HUD hooks
- `CV_Set`, `CV_StealthSet`,  `CV_AddValue`, `CV_FindVar` exposed to Lua
- New music related functions exposed to Lua,`S_GetMusicLength`, `S_GetMusicPosition `and `S_SetMusicPosition`
- `isserver`, `isdedicatedserver`, `consoleplayer`, `displayplayer` and `secondarydisplayplayer` exposed to Lua
- `V_DrawFill` now supports translucency flags https://github.com/P-AS/srb2-legacy/commit/5e36e902e21f390f29144d808f0406ed7ad2116c
- Lua HUD Drawlists github.com/P-AS/srb2-legacy/commit/d10d32d4115907cdbebe4a885fab1190635b4a56
- Resend gamestate #109
- The size of the FPS and TPS counter can be changed #111
- Software Skydome https://github.com/P-AS/srb2-legacy/commit/a07be0264632296cb7045e1074c1fff70986f056
- Add a "-noaudio" parm to cover "-nomusic" and "-nosound" https://github.com/P-AS/srb2-legacy/commit/4e4aea687d36fc8695ccee9f8e0ec6f819173f53
- Menu tooltips https://github.com/P-AS/srb2-legacy/commit/eeded20462058c82a349d367cb8319a072acf413
- The pause screen has been updated to the one in 2.2 https://github.com/P-AS/srb2-legacy/commit/bb559c31ef213c8f89706ccdb76ab89d08472186
  - The original pause screen can be reenabled https://github.com/P-AS/srb2-legacy/commit/33c5ebf555719b945ff14026a55e5b0fcfb67e10
- Different resolutions can be set for full screen and windowed mode https://github.com/P-AS/srb2-legacy/commit/00e86819606216c8fabc0767aa44b53aba855329
- Models now support interpolation flags, which fixes interpolation if using the softpoly model pack or models from 2.2 https://github.com/P-AS/srb2-legacy/commit/785c384dc9f5a27b1c11fd6a201b2d99f5bccc17
- `ffloorclip` has been added to Software, improving performance in maps with a large amount of FOFs
- Various netcode improvements #117
- Fixed issues with joining netgames during intermission
- `ALAM_LIGHTING` has been totally obliterated https://github.com/P-AS/srb2-legacy/commit/9d96c7cfba49386a629cf70019e4a5278000c46e
- Horizon lines have been added and they work in both renderers https://github.com/P-AS/srb2-legacy/commit/0dcb55e9b20fbc99e8b16ceda6c8266a11ac4f63 https://github.com/P-AS/srb2-legacy/commit/027e8a7fb60d1ebb9836e2ad46aff16325167055 https://github.com/P-AS/srb2-legacy/commit/3abf2355885162424b23304150652648a9b194c4
- Added a menu option to join last server https://github.com/P-AS/srb2-legacy/commit/51cb6e21238e4fb0c8d4ab3a760d53b683ec062a
- Statistics Menu has been overhauled to be a single page https://github.com/P-AS/srb2-legacy/commit/7204769ed85f9ebeb89f4393aca39382e3804029

### Improvements

- `P_DivlineSide` has been optimized, allowing for faster line of sight checks #90
- OpenGL transparency sorting has been significantly optimized #92
- OpenGL sprite sorting has also been optimized https://github.com/P-AS/srb2-legacy/commit/9e2994b55e9bb02bbd23f76f3cd8d50ee4f02e0a
- View rolling #98
- Software now supports ripples on sloped planes https://github.com/P-AS/srb2-legacy/commit/e017baf9dece7bed0e75197cc410ed4984afc996
- PK3 loading fixed backported from Kart #99
  - You can now load non game modifying add-ons, like music files or shaders from PK3's without marking the game as modified, and create PK3's loadable by the game using an external tool, rather than having to use SLADE
- Port Kart's dedicated server idle #102
- Make the server information screen gamepad friendly https://github.com/P-AS/srb2-legacy/commit/4b26215b5febc1408b55467f34b869aad47af31d
- SDL2: Set application name hint if SDL >= 2.0.22 https://github.com/P-AS/srb2-legacy/commit/49c266fec599bd699373b25ec7a25553bf850339
- Allow FPS Cap Values https://github.com/P-AS/srb2-legacy/commit/68a1b4aa704e998164a80e27eac0e4fe252aeee0
- Removed a lot of useless files and code from the repo #121 #129
- Make execinfo.h optional (fixes musl libc build) https://github.com/P-AS/srb2-legacy/commit/a12b5000ed9e4ecaed1ed3fcd7cb04e58a2510b4
  - On Linux systems with musl libc, use the `NOEXECINFO=1` Makefile option or `-DSRB2_CONFIG_EXECINFO=OFF` CMake option when compiling.
- OpenBSD support, for both Makefile and CMake https://github.com/P-AS/srb2-legacy/commit/000c13cf4af36ab93a447817e85aae1cbcd03ce9 https://github.com/P-AS/srb2-legacy/commit/16ea10d290b71f5e1f162b323e2ca9968ce47a6c
- NiGHTs render distance now only applies to hoops and not *all* objects https://github.com/P-AS/srb2-legacy/commit/0610afc0af76d5ae42178a8f56e748820cb10c73
- When loading a mod that changes the title screen in any way, the title screen will no longer be a mishmashed amalgamation https://github.com/P-AS/srb2-legacy/commit/ae8bafde1d082023faaeaa20fd54029c254b692f
- Fixed OpenGL debug logging, `ogllog.txt` works now https://github.com/P-AS/srb2-legacy/commit/cb144b066c3be68d9c290010a6fd89ae79b3dde4
- Objectplace now uses Weapon Next/Prev to cycle between objects https://github.com/P-AS/srb2-legacy/commit/9ea0060b67935ba5eceebeadb96b7f5b90c90817
- Added `DEVMODE` only 2d mode toggle, `toggletwod` https://github.com/P-AS/srb2-legacy/commit/9f6019ab9009bad820e07b58caa0d2ef6d876cb5
- Added the `add` command (lol) to increment a variables value https://github.com/P-AS/srb2-legacy/commit/5d174738288b27184571ba6769f35010dbf83eab

### Bug/Regression fixes

- Important: "Fix loss of momentum when travelling up multiple steps in a tic" (#5) has been [reverted](https://github.com/P-AS/srb2-legacy/commit/9018b0d1060b2c16a90cd55544cbe85e6f871331) as it [negativley affected gameplay](https://file.garden/ZuUiCPHqjV5ssvQM/dump/brokendsz2waterfall.gif) and made SRB2 Legacy no longer have true vanilla 2.1 gameplay. 
  - Regrettably, this means demos recorded in 2.1.27 and 2.1.28 may no longer sync correctly in 2.1.29 and later.
    Note for any contributors or future contributors, changes that affect gameplay or physics will not be accepted!!!
- Shadows in OpenGL combined with shaders have been fixed https://github.com/P-AS/srb2-legacy/commit/3cd73a5da27d805462b68c6c89b8c7c3800cdce8
- Multithreading on macOS has been properly enabled https://github.com/P-AS/srb2-legacy/commit/46b5689f9a277de031c7b2551574b8e520cc6ed6
- Multithreading on Haiku has been fixed https://github.com/P-AS/srb2-legacy/commit/da06c002ffa33379664306b39ac9fe868807b865
- The character select menu scrolling too fast with uncapped has been fixed #95
- Fix issues with uninitialized variables on polyobject thinkers #100
- In OpenGL mode, the default size of the depth buffer has been increased to (hopefully) fix Z-fighting on some GPUs #101
- Various fixes for CMake, especially benefiting macOS users
  - CMake modules for finding libopenmpt and libgme have been fixed https://github.com/P-AS/srb2-legacy/commit/b4afb3b9395e6bf90b0a45bcaeefdc88f8bdc176 https://github.com/P-AS/srb2-legacy/commit/82b7bc28c3ef00a4e1710a438b4c3c85528c067f
  - Fullscreen mode is no longer broken on CMake builds https://github.com/P-AS/srb2-legacy/commit/68e5c3e462f4d45e42e289629183b6920c495e4c
  - `ld: library 'SDL2' not found` when building on macOS has been fixed https://github.com/P-AS/srb2-legacy/commit/7ab608a25b02ca6aff4d698751ed4ccd3ed169b8
  - rpath issues have been fixed https://github.com/P-AS/srb2-legacy/commit/3fb1630f8f5a40cd3c1acf483196a5b1aad77530
  - `Info.plist` properties in the macOS bundle are now being set https://github.com/P-AS/srb2-legacy/commit/a98236414bab42a2ebb05380280eda2fe50fee07
  - An option to configure CMake without the presence of assets in `assets/installer` has been added, since it complicates the process and often isn't necessary just to get a working build. The old behavior can be re-enabled with `-DSRB2_CONFIG_EXTERNAL_ASSETS=ON`. https://github.com/P-AS/srb2-legacy/commit/d0991281caa03fd8f86f98ac29e409ecc30a2882
  - CMake: don't configure before CPACK_PACKAGE_DESCRIPTION_SUMMARY is set https://github.com/P-AS/srb2-legacy/commit/9548e8e6274f33097a9a8f37412e89f2a55b890d
- Toggling VSync at runtime is fixed if SDL >= 2.0.18 https://github.com/P-AS/srb2-legacy/commit/3ab9b8bb5e3f028eeb97a70a49107879bf330f0e https://github.com/P-AS/srb2-legacy/commit/4ee85286edd423566095aeb9fb3df3e4128cdec8
- Fix dedicated build #104
- Fix compilation with C23/gnu23 #106
- Fix text input mode not being reset after closing the menu https://github.com/P-AS/srb2-legacy/commit/0da349ab68dfe21063c578930ff3ffc21eacb1a7
- Fix InterScreen level header parameter https://github.com/P-AS/srb2-legacy/commit/3bb4b6fac14e4c6248df5807345d9eb47b46202e
- Clamp OpenGL lightlevel stuff https://github.com/P-AS/srb2-legacy/commit/a9587f55cd7d50134ffc74f6db02e1ef54c304eb
- Fix custom addon directory being ignored when downloading addons https://github.com/P-AS/srb2-legacy/commit/ade636ef4de555fbaea7c32a4e6d092802dfaa44
- Register the variables in more sane places https://github.com/P-AS/srb2-legacy/commit/bf961b4819c03865a72ee634d3d7db51076f0036
- Store last ticcmd in a buffer rather than in netcmd https://github.com/P-AS/srb2-legacy/commit/63e0fcc67d67c5fa5ff6cd47da5cadf61202cf6c
  - This fixes a consistent one frame input delay caused by #88
- Move HWR_ClearClipper() call after SetTransform()
  - Fixes bugged culling in OpenGL when fov is set to >90
- Fix issues with fog blocks when shaders are enabled https://github.com/P-AS/srb2-legacy/commit/3317bb98b0d70dd8b343f97738e06ba7d1706dfa
- Fix issues with extremely large rooms, in both OpenGL and Software https://github.com/P-AS/srb2-legacy/commit/f92e4be1a23e305f1f42e21363bf927c5ca3a3c0 5a767c521b93fe125a2113cb458d76e3c6ef678b 
- Reduced amplitude on underwater wave in OpenGL https://github.com/P-AS/srb2-legacy/commit/911fe530ae9f1c07c64cd39f5f59e372d0532c59
- Fixed OpenGL sprite scale not interpolating https://github.com/P-AS/srb2-legacy/commit/2221a5b5252f3cb873b3eda5ea246445b265be8c
- Fixed level interpolators jittering during objectplace https://github.com/P-AS/srb2-legacy/commit/8d2b499681f044a5bbb2a4e2ab17f7dcdc816141
- Fixed explosion ring interpolation https://github.com/P-AS/srb2-legacy/commit/cc21d16a16817a97f580f1ba8a9d3f55323b0b65
- Improved relative teleport interpolation https://github.com/P-AS/srb2-legacy/commit/82a00d230930ad3f456e641f881c5ba974237318
- Fixed Mario Block interpolation https://github.com/P-AS/srb2-legacy/commit/5f147418b360df9f3fb4fefd903e3f0355fab05c
- Fixed menu highlights with scrolling menus https://github.com/P-AS/srb2-legacy/commit/7dcf0333a52393ed9e70735d20f88215aafbe62f
- Fixed dependencies on Makefile not working properly https://github.com/P-AS/srb2-legacy/commit/f468400a7ee9afe9de802a35c54d8673e99cb274
- Fix using `exec .` crashing on Linux https://github.com/P-AS/srb2-legacy/commit/d82d88724b08b7f0c5b47abe8577cc7db36e4c2f
- A bunch of unsafe R_PointinSubsector calls causing issues have been fixed
- Fix TERMIOS buffer overflow https://github.com/P-AS/srb2-legacy/commit/6fcd97967722c96299dc9bf62eaf4a55d9bba91f
- Fix segfault when removing mobjs while iterating thinglist (Backport of MR 2293) https://github.com/P-AS/srb2-legacy/commit/282e9702554264f4ac0d3635139e71ed10acea2a
- check if LUA field exists before accessing it (Backport of MR 2261)  https://github.com/P-AS/srb2-legacy/commit/8207ce8776a6a6dd0dd15ace3c358abd27958f65
- Fix dangling pointer in mapthing after removing mobj (Backport of MR 2007) https://github.com/P-AS/srb2-legacy/commit/9b3296aee0b43e3dc05e080580566353d0f42d3b
- Fix interpolation when curling up or scaling while flipped (Backport of MR 1979)  https://github.com/P-AS/srb2-legacy/commit/ddeac2965b1b8b3e9ca99d87d86d55019a7a1278
- Interpolate polyobjects properly for the software renderer https://github.com/P-AS/srb2-legacy/commit/68f140c444e90f7e17ba22f6da11a3478fef67cb

**Full Changelog**: https://github.com/P-AS/srb2-legacy/compare/SRB2_release_2.1.28...SRB2_release_2.1.29