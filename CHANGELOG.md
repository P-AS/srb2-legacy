# 2.1.30
### Major new additions

- We now have an Android port!
     - This port is fully functional, but there are some limitations. There are no touch controls, so a gamepad (or KB/M) must be used instead. The OpenGL renderer is also not supported, so Software must be used instead. See [android/README.md](https://github.com/srb2-preservation/srb2-legacy/blob/next/android/README.md) for more info on the status of the port.
     - Assets are not yet packaged in the APK, they must be placed in `/storage/emulated/0/SRB2 Legacy`.
- We now have a [Web port](https://srb2-preservation.github.io/srb2-legacy/)! (#147)
     - The OpenGL renderer doesn't work here either, and neither do netgames.
- Marathon Run (#158) 
     - Live Event Backups have not been included. 
- 2.2 Level Select Platter (#178)
     - Both this and Marathon Run benefit from new assets backported from 2.2. The new `legacy.pk3` can be downloaded from our [assets repo](https://codeberg.org/srb2-preservation/assets/src/branch/legacy/legacy.pk3). This is optional, and the game is playable without it loaded.
- Various camera features from SRB2-Banpyura (#172)
     - `cam_exact`: Exact camera aiming.
     - `cam_clipping`: `Off` to disable clipping, `Vanilla` for default behavior, and `Exact` for clipping based on a raycast from the player's position.
- Native SRB2 home and WAD directory paths on \*nix and macOS (#177)
     - `~/.local/share/srb2-legacy` (on \*nix) and `~/Library/Application Support/srb2-legacy` (on macOS) are the new default paths for SRB2 Legacy's home folder. Previously made `~/.srb2_21` folders will still work.
     - The WAD directory is now found by searching `$XDG_DATA_DIRS/games/srb2-legacy` and `$XDG_DATA_DIRS/srb2-legacy`. By default this includes `/usr/local/share/games/srb2-legacy`, `/usr/share/games/srb2-legacy`, `/usr/local/share/srb2-legacy`, `/usr/share/srb2-legacy`.
- GIF recording improvements (9bf32e371489732dfe2ac744e31ceb142708d029)
     - `gif_maxsize`: Sets a filesize limit on GIF movies.
     - `gif_rolling`: Splits GIFs into multiple files after filesize limit is set.
     - There is now a HuD showing the length and filesize of a movie.
- Jingles and Invincibility/Speed shoes music no longer force the stage's music to restart, and fade in and out. (78da8e06dd737dd93260bd7103813a57915a84fa)

### Minor new additions
 
- The `find` console command, to search for variables, commands, and aliases. (#159)
- Lua Polyobjects (#175)
- Lua titlescreen and titlecard HUD Hooks (#176)
- Borderless window toggle (92b7ef661b4a42264799d5e7842287b715666bc2)
- Add option to enable/disable title screen music (2aed23ba2258281e494cc222cd9747a1363c28b2)
- Add `-workdir` to manually change SRB2's home folder (f789fc43206269e45f0283e71afdbc2d8b2cc2bc)
- Analog stick deadzones (7d4f5b4612eb73c7784f11296f319cc4bcadbc02)
     - Fixes a bug where voting for a map in the SUGOI trilogy was impossible if the player moved the camera on a gamepad. 
- Makefile: rudimentary macOS support (8d67c327ce26d703b7a95a4757b1455306731eae)
- Add configurable minimum sector brightness (b7dca22b14f0a87fc08c3f5bf5e426da10e449b2)
- Expose `I_Error` to Lua (876eda061b351f0be93a4e1817aafca53bb8e5eb)
- Legacyslop. (ae0f4a2385dfece185a83f83bcc1a3cb997e3c55)
- Add support for using custom SDL2 mappings (63cb77a889cf755db2c4e51a7b4a73d32a5e0229)
- Add control options menu (373a06fc5e1325f63b498290eb8e25c13c1c9cd0)

### Improvements

- New `M_Random` implementation (3e16d8fb48a14b702923e0c165e7a4fe59dfe12e)
- Target `x86-64-v2` instead of `nocona` for 64-bit Windows (08bd25e44713a0e35e400d3618823e50490212a4)
- Skip base archives from displaying on the addon list (#167)
- Signal handler minor refactor (c6f92ee8cb2c9225b36db57e4c163790f1aa0f47)
- Build Android port on all supported ABIs (4cfb2093c671534300becebe6c58ba5d67587ded)
- Don't force DirectSound for SDL versions from 2.26.5 on Windows (10a74172f7d4ab87cdf22088c1e36a08c34be206)
- Update SDL2 mixer on Windows to 2.8.1 (fe87225e59d6219cc178ee86fa65a061e5e8e554)
- Update SDL2 on Windows to 2.32.10 (6744bb2b55943305af510b1175c404a1e43a8cf8)
- Make black console back blacker (darker) (dc2cde9f6204cd672158b342a1ca1f807763c763)
- GCC 15/16 warning fixes (73280e012f6b510cffc94b14d786ebf3c6c470b6)
- Use Command instead of Control for ctrldown on macOS (dac8f40a299b57bd219927e0776eb17b1d471c4b)
- Support 16 KB page size on Android (9edb80be5408e9c346596c66a6d6dde37681807e, a8f330618a27cd5974674bfc158dc76e8029019e, 74e6c5ea048cbd607259a802d5966e6b798ae634)

### Bug/Regression fixes

- Allow `mobjinfo_t` to use custom variables again (#165)
- Fix using accelerometer as arrow keys on Android (#146)
- Fix sky culling in the OpenGL renderer (996a6daa3c07603c515ea136909fa5679363db4a)
- Only use aligned_alloc on macOS >= 10.15 (b96025dba3e69fd47b69a29d9ee8d739ca4068e4)
- Fix crash on Intel macOS on resolutions that are not multiples of 32 (9f58e8a6c88479c2b9414451ec5c9612cb025dd6)
- Fix OS-specific behavior caused by integer overflow on Lua numbers (c935c51370d1c9823eed572396d481a92884cf1d)
- Fix menucompnote buffer overflow (f88adf7ab9335c4933f23df802a9535f5efa334c)
- replace `vsprintf` with `vsnprintf` in `CONS_Printf` (bcb4bfb1ae98b65d5a3e62f3b7aaf0e2eaea9e8f)
- fix: multiple additional buffer overruns (e5f4b4f4a51dac008a069e60842600c8c85f8198)
- Fix double byte-swap in `SV_MakeTic` (16780f6629f7b9d5a3354d6d24811ae1380618a6)
     - Fixes a regression which made the game unplayable on big endian platforms. 
- Fix objectplace next/prev scrolling too fast (62a7aeb12276de61259e2dfb5ef5c69c262ee21a)
- Fix AppImages with filename spaces not opening (b3fcbcc86ceed623767b097b8a5abeff566e81bf)
- Don't let the user switch to OpenGL if NOHW (8d8c2754e007323610b156506d459cfa9321a174)
- Fix OpenMPT versions strings leaking (2c6f450c04ff19d9e1611aa5df6c04a8ce3ec572)
- Fix bug where SRB2 would check size of current directory instead of srb2home (78223f746d9576a637e86b2a6a7b8472339c27d5)
- Fix freeze after intro sequence that no one watches (a3e56eefa5b4d9327b1954b359bac56f6ce3af77)
- Make curve shader (slightly) less disorienting (f18c344914fc5128c7fbe052ccba0179329c76e9)
- Don't do the special stage fade to white when resending gamestate 72302c3c82f6783594fc92a23ee65cd3dd81b4dc)
- Fix using CTRL+V into connect IP textbox (e89513b59292d64fbc7af72659dc99d1d4e92953)
- Fix ip address menu OOB string crashes (8d8bd23f4ff27d7c7ca11b59b380cba2723fa838)
- Fix interpolation of the "IT" sign (348dc71eb9feecfe925258374b8cb24f4425def7)
- Fix OpenGL precip not rendering (6ab9f640b58a3eff922a2ca4fad421fce6289ddb)
- Make borderless window less shit (ff475fc24f71bd618111945fe7c5a90b9b2d3952)
- GENUINELY fix the intro (eed74dfcdb5e47cffe57a4504a70b1d00ec1195b)
- Fix level interpolators on midgame join (d8b0942a6fb559cd045f5cc40f5454c8940fbe7d)
- Stop evil handkerchiefs from destroying our computer's performance (8e2cc50dc95f8f582fc30d16abf39e071eed7c69)
     - Fixes a performance regression in Cutout Warehouse Zone in KIMOKAWAIII
- Fix colormap textures not being updated for palette shaders (05b777bbf668c74fdaa94d5f006120a887ac355c)

**Full Changelog**: https://github.com/srb2-preservation/srb2-legacy/compare/SRB2_release_2.1.29...SRB2_release_2.1.30

# 2.1.29 R1
### Bug/Regression fixes
- Fix sky culling in the OpenGL renderer https://github.com/srb2-preservation/srb2-legacy/commit/996a6daa3c07603c515ea136909fa5679363db4a 

**Full Changelog**: https://github.com/srb2-preservation/srb2-legacy/compare/SRB2_release_2.1.29...SRB2_release_2.1.29R1

# 2.1.29
### Major new additions

- Renderer Switching has (finally) been added, you can now swap between renderers ingame #124
  - To go along with that, setting the renderer via `renderer.txt` has been removed due to it being redundant with renderer switching https://github.com/srb2-preservation/srb2-legacy/commit/fab29e1261b8e6b06c53381e9c7e8603c022d641
- Palette Rendering #126
- A new information screen has been added before joining a server #97
- Custom skincolor support #114, Try the Kart skincolors lua [here](https://file.garden/Z8jIwHTgmgo8vTto/kartcolors.lua)!
  - `netcompat` has been removed to issues with custom skincolors and general instability
  - All custom skincolor related Lua functions are exposed
- `perfstats` from 2.2 has been backported #91
- Improved color settings #94
  - Notably, this removes OpenGL gamma correction
- Changing the FOV is now supported in Software https://github.com/srb2-preservation/srb2-legacy/commit/e7d37547433b30b55c950106990c50871a71dfa5
  - `fovchange` is supported too
- Orbital camera has been backported, can be enabled with `cam_orbit on`
- Downhill slope adjustment has been backported, enabled by default and can be toggled with `cam_adjust`
- Connect IP Menu has been replaced with a textbox https://github.com/srb2-preservation/srb2-legacy/commit/db1f37ea68b6d8824bec70c7f43e48dee1ca8707  <!-- This is actually *not* the 2.2 version -->
- Wireframe mode in OpenGL https://github.com/srb2-preservation/srb2-legacy/commit/592067a43e7db3af640f4aac53293ca24ef058c8
- Model blend textures have been updated to support translation colormaps https://github.com/srb2-preservation/srb2-legacy/commit/82aa4f33149778498e58b187a91b97c32f76fc9b
  <img src="https://github.com/user-attachments/assets/1bbff964-aca6-4564-aaf3-68da5793b576" width=30%/>
- New Lua string alignment options: https://github.com/srb2-preservation/srb2-legacy/commit/5b75724dbfb4ce047362e14b8799c468fb8f79a4
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
- Lua I/O support https://github.com/srb2-preservation/srb2-legacy/commit/9224e8f56dd736a6f0a397ce14c2a1573b815630
  - Including the I/O library from 2.2, backwards compatible with 2.1 mods, like SUBARASHII #134
- Paired down OS Lua library #108
- Text colormaps from SRB2Kart https://github.com/srb2-preservation/srb2-legacy/commit/53fde3604c03182fb1fb85525154c6d325e19f36
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
- `V_DrawFill` now supports translucency flags https://github.com/srb2-preservation/srb2-legacy/commit/5e36e902e21f390f29144d808f0406ed7ad2116c
- Lua HUD Drawlists github.com/srb2-preservation/srb2-legacy/commit/d10d32d4115907cdbebe4a885fab1190635b4a56
- Resend gamestate #109
- The size of the FPS and TPS counter can be changed #111
- Software Skydome https://github.com/srb2-preservation/srb2-legacy/commit/a07be0264632296cb7045e1074c1fff70986f056
- Add a "-noaudio" parm to cover "-nomusic" and "-nosound" https://github.com/srb2-preservation/srb2-legacy/commit/4e4aea687d36fc8695ccee9f8e0ec6f819173f53
- Menu tooltips https://github.com/srb2-preservation/srb2-legacy/commit/eeded20462058c82a349d367cb8319a072acf413
- The pause screen has been updated to the one in 2.2 https://github.com/srb2-preservation/srb2-legacy/commit/bb559c31ef213c8f89706ccdb76ab89d08472186
  - The original pause screen can be reenabled https://github.com/srb2-preservation/srb2-legacy/commit/33c5ebf555719b945ff14026a55e5b0fcfb67e10
- Different resolutions can be set for full screen and windowed mode https://github.com/srb2-preservation/srb2-legacy/commit/00e86819606216c8fabc0767aa44b53aba855329
- Models now support interpolation flags, which fixes interpolation if using the softpoly model pack or models from 2.2 https://github.com/srb2-preservation/srb2-legacy/commit/785c384dc9f5a27b1c11fd6a201b2d99f5bccc17
- `ffloorclip` has been added to Software, improving performance in maps with a large amount of FOFs
- Various netcode improvements #117
- Fixed issues with joining netgames during intermission
- `ALAM_LIGHTING` has been totally obliterated https://github.com/srb2-preservation/srb2-legacy/commit/9d96c7cfba49386a629cf70019e4a5278000c46e
- Horizon lines have been added and they work in both renderers https://github.com/srb2-preservation/srb2-legacy/commit/0dcb55e9b20fbc99e8b16ceda6c8266a11ac4f63 https://github.com/srb2-preservation/srb2-legacy/commit/027e8a7fb60d1ebb9836e2ad46aff16325167055 https://github.com/srb2-preservation/srb2-legacy/commit/3abf2355885162424b23304150652648a9b194c4
- Added a menu option to join last server https://github.com/srb2-preservation/srb2-legacy/commit/51cb6e21238e4fb0c8d4ab3a760d53b683ec062a
- Statistics Menu has been overhauled to be a single page https://github.com/srb2-preservation/srb2-legacy/commit/7204769ed85f9ebeb89f4393aca39382e3804029

### Improvements

- `P_DivlineSide` has been optimized, allowing for faster line of sight checks #90
- OpenGL transparency sorting has been significantly optimized #92
- OpenGL sprite sorting has also been optimized https://github.com/srb2-preservation/srb2-legacy/commit/9e2994b55e9bb02bbd23f76f3cd8d50ee4f02e0a
- View rolling #98
- Software now supports ripples on sloped planes https://github.com/srb2-preservation/srb2-legacy/commit/e017baf9dece7bed0e75197cc410ed4984afc996
- PK3 loading fixed backported from Kart #99
  - You can now load non game modifying add-ons, like music files or shaders from PK3's without marking the game as modified, and create PK3's loadable by the game using an external tool, rather than having to use SLADE
- Port Kart's dedicated server idle #102
- Make the server information screen gamepad friendly https://github.com/srb2-preservation/srb2-legacy/commit/4b26215b5febc1408b55467f34b869aad47af31d
- SDL2: Set application name hint if SDL >= 2.0.22 https://github.com/srb2-preservation/srb2-legacy/commit/49c266fec599bd699373b25ec7a25553bf850339
- Allow FPS Cap Values https://github.com/srb2-preservation/srb2-legacy/commit/68a1b4aa704e998164a80e27eac0e4fe252aeee0
- Removed a lot of useless files and code from the repo #121 #129
- Make execinfo.h optional (fixes musl libc build) https://github.com/srb2-preservation/srb2-legacy/commit/a12b5000ed9e4ecaed1ed3fcd7cb04e58a2510b4
  - On Linux systems with musl libc, use the `NOEXECINFO=1` Makefile option or `-DSRB2_CONFIG_EXECINFO=OFF` CMake option when compiling.
- OpenBSD support, for both Makefile and CMake https://github.com/srb2-preservation/srb2-legacy/commit/000c13cf4af36ab93a447817e85aae1cbcd03ce9 https://github.com/srb2-preservation/srb2-legacy/commit/16ea10d290b71f5e1f162b323e2ca9968ce47a6c
- NiGHTs render distance now only applies to hoops and not *all* objects https://github.com/srb2-preservation/srb2-legacy/commit/0610afc0af76d5ae42178a8f56e748820cb10c73
- When loading a mod that changes the title screen in any way, the title screen will no longer be a mishmashed amalgamation https://github.com/srb2-preservation/srb2-legacy/commit/ae8bafde1d082023faaeaa20fd54029c254b692f
- Fixed OpenGL debug logging, `ogllog.txt` works now https://github.com/srb2-preservation/srb2-legacy/commit/cb144b066c3be68d9c290010a6fd89ae79b3dde4
- Objectplace now uses Weapon Next/Prev to cycle between objects https://github.com/srb2-preservation/srb2-legacy/commit/9ea0060b67935ba5eceebeadb96b7f5b90c90817
- Added `DEVMODE` only 2d mode toggle, `toggletwod` https://github.com/srb2-preservation/srb2-legacy/commit/9f6019ab9009bad820e07b58caa0d2ef6d876cb5
- Added the `add` command (lol) to increment a variables value https://github.com/srb2-preservation/srb2-legacy/commit/5d174738288b27184571ba6769f35010dbf83eab

### Bug/Regression fixes

- Important: "Fix loss of momentum when travelling up multiple steps in a tic" (#5) has been [reverted](https://github.com/srb2-preservation/srb2-legacy/commit/9018b0d1060b2c16a90cd55544cbe85e6f871331) as it [negativley affected gameplay](https://file.garden/ZuUiCPHqjV5ssvQM/dump/brokendsz2waterfall.gif) and made SRB2 Legacy no longer have true vanilla 2.1 gameplay. 
  - Regrettably, this means demos recorded in 2.1.27 and 2.1.28 may no longer sync correctly in 2.1.29 and later.
    Note for any contributors or future contributors, changes that affect gameplay or physics will not be accepted!!!
- Shadows in OpenGL combined with shaders have been fixed https://github.com/srb2-preservation/srb2-legacy/commit/3cd73a5da27d805462b68c6c89b8c7c3800cdce8
- Multithreading on macOS has been properly enabled https://github.com/srb2-preservation/srb2-legacy/commit/46b5689f9a277de031c7b2551574b8e520cc6ed6
- Multithreading on Haiku has been fixed https://github.com/srb2-preservation/srb2-legacy/commit/da06c002ffa33379664306b39ac9fe868807b865
- The character select menu scrolling too fast with uncapped has been fixed #95
- Fix issues with uninitialized variables on polyobject thinkers #100
- In OpenGL mode, the default size of the depth buffer has been increased to (hopefully) fix Z-fighting on some GPUs #101
- Various fixes for CMake, especially benefiting macOS users
  - CMake modules for finding libopenmpt and libgme have been fixed https://github.com/srb2-preservation/srb2-legacy/commit/b4afb3b9395e6bf90b0a45bcaeefdc88f8bdc176 https://github.com/srb2-preservation/srb2-legacy/commit/82b7bc28c3ef00a4e1710a438b4c3c85528c067f
  - Fullscreen mode is no longer broken on CMake builds https://github.com/srb2-preservation/srb2-legacy/commit/68e5c3e462f4d45e42e289629183b6920c495e4c
  - `ld: library 'SDL2' not found` when building on macOS has been fixed https://github.com/srb2-preservation/srb2-legacy/commit/7ab608a25b02ca6aff4d698751ed4ccd3ed169b8
  - rpath issues have been fixed https://github.com/srb2-preservation/srb2-legacy/commit/3fb1630f8f5a40cd3c1acf483196a5b1aad77530
  - `Info.plist` properties in the macOS bundle are now being set https://github.com/srb2-preservation/srb2-legacy/commit/a98236414bab42a2ebb05380280eda2fe50fee07
  - An option to configure CMake without the presence of assets in `assets/installer` has been added, since it complicates the process and often isn't necessary just to get a working build. The old behavior can be re-enabled with `-DSRB2_CONFIG_EXTERNAL_ASSETS=ON`. https://github.com/srb2-preservation/srb2-legacy/commit/d0991281caa03fd8f86f98ac29e409ecc30a2882
  - CMake: don't configure before CPACK_PACKAGE_DESCRIPTION_SUMMARY is set https://github.com/srb2-preservation/srb2-legacy/commit/9548e8e6274f33097a9a8f37412e89f2a55b890d
- Toggling VSync at runtime is fixed if SDL >= 2.0.18 https://github.com/srb2-preservation/srb2-legacy/commit/3ab9b8bb5e3f028eeb97a70a49107879bf330f0e https://github.com/srb2-preservation/srb2-legacy/commit/4ee85286edd423566095aeb9fb3df3e4128cdec8
- Fix dedicated build #104
- Fix compilation with C23/gnu23 #106
- Fix text input mode not being reset after closing the menu https://github.com/srb2-preservation/srb2-legacy/commit/0da349ab68dfe21063c578930ff3ffc21eacb1a7
- Fix InterScreen level header parameter https://github.com/srb2-preservation/srb2-legacy/commit/3bb4b6fac14e4c6248df5807345d9eb47b46202e
- Clamp OpenGL lightlevel stuff https://github.com/srb2-preservation/srb2-legacy/commit/a9587f55cd7d50134ffc74f6db02e1ef54c304eb
- Fix custom addon directory being ignored when downloading addons https://github.com/srb2-preservation/srb2-legacy/commit/ade636ef4de555fbaea7c32a4e6d092802dfaa44
- Register the variables in more sane places https://github.com/srb2-preservation/srb2-legacy/commit/bf961b4819c03865a72ee634d3d7db51076f0036
- Store last ticcmd in a buffer rather than in netcmd https://github.com/srb2-preservation/srb2-legacy/commit/63e0fcc67d67c5fa5ff6cd47da5cadf61202cf6c
  - This fixes a consistent one frame input delay caused by #88
- Move HWR_ClearClipper() call after SetTransform()
  - Fixes bugged culling in OpenGL when fov is set to >90
- Fix issues with fog blocks when shaders are enabled https://github.com/srb2-preservation/srb2-legacy/commit/3317bb98b0d70dd8b343f97738e06ba7d1706dfa
- Fix issues with extremely large rooms, in both OpenGL and Software https://github.com/srb2-preservation/srb2-legacy/commit/f92e4be1a23e305f1f42e21363bf927c5ca3a3c0 5a767c521b93fe125a2113cb458d76e3c6ef678b 
- Reduced amplitude on underwater wave in OpenGL https://github.com/srb2-preservation/srb2-legacy/commit/911fe530ae9f1c07c64cd39f5f59e372d0532c59
- Fixed OpenGL sprite scale not interpolating https://github.com/srb2-preservation/srb2-legacy/commit/2221a5b5252f3cb873b3eda5ea246445b265be8c
- Fixed level interpolators jittering during objectplace https://github.com/srb2-preservation/srb2-legacy/commit/8d2b499681f044a5bbb2a4e2ab17f7dcdc816141
- Fixed explosion ring interpolation https://github.com/srb2-preservation/srb2-legacy/commit/cc21d16a16817a97f580f1ba8a9d3f55323b0b65
- Improved relative teleport interpolation https://github.com/srb2-preservation/srb2-legacy/commit/82a00d230930ad3f456e641f881c5ba974237318
- Fixed Mario Block interpolation https://github.com/srb2-preservation/srb2-legacy/commit/5f147418b360df9f3fb4fefd903e3f0355fab05c
- Fixed menu highlights with scrolling menus https://github.com/srb2-preservation/srb2-legacy/commit/7dcf0333a52393ed9e70735d20f88215aafbe62f
- Fixed dependencies on Makefile not working properly https://github.com/srb2-preservation/srb2-legacy/commit/f468400a7ee9afe9de802a35c54d8673e99cb274
- Fix using `exec .` crashing on Linux https://github.com/srb2-preservation/srb2-legacy/commit/d82d88724b08b7f0c5b47abe8577cc7db36e4c2f
- A bunch of unsafe R_PointinSubsector calls causing issues have been fixed
- Fix TERMIOS buffer overflow https://github.com/srb2-preservation/srb2-legacy/commit/6fcd97967722c96299dc9bf62eaf4a55d9bba91f
- Fix segfault when removing mobjs while iterating thinglist (Backport of MR 2293) https://github.com/srb2-preservation/srb2-legacy/commit/282e9702554264f4ac0d3635139e71ed10acea2a
- check if LUA field exists before accessing it (Backport of MR 2261)  https://github.com/srb2-preservation/srb2-legacy/commit/8207ce8776a6a6dd0dd15ace3c358abd27958f65
- Fix dangling pointer in mapthing after removing mobj (Backport of MR 2007) https://github.com/srb2-preservation/srb2-legacy/commit/9b3296aee0b43e3dc05e080580566353d0f42d3b
- Fix interpolation when curling up or scaling while flipped (Backport of MR 1979)  https://github.com/srb2-preservation/srb2-legacy/commit/ddeac2965b1b8b3e9ca99d87d86d55019a7a1278
- Interpolate polyobjects properly for the software renderer https://github.com/srb2-preservation/srb2-legacy/commit/68f140c444e90f7e17ba22f6da11a3478fef67cb

**Full Changelog**: https://github.com/P-AS/srb2-legacy/compare/SRB2_release_2.1.28...SRB2_release_2.1.29
