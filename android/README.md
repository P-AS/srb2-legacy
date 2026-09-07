# SRB2 Legacy Android Port

Based off [bitten2up/Kart-Public/android](https://github.com/bitten2up/Kart-Public/tree/android). The port is in a stable state with some limitations.

## Status

- [x] Compiles
- [x] Runs
- [x] Software renderer
- [ ] OpenGL renderer
     - gl4es is not compatible with SRB2 in its current state.
- [x] Gamepad controls
     - Works, but remapping might be required. This is the recommended way to play this port.
- [x] Keyboard controls
- [ ] Touch controls
     - Not planned. It wouldn't play very well anyway.
     - Note that you can use touch to move the camera like you would with a mouse.
- [x] On-screen keyboard for text input
     - Somewhat buggy, it sometimes appears when it shouldn't
- [x] Netplay
- [x] Logs
     - latest-log.txt works, dated logs are not yet working
- [x] Addon support
- [x] Native Resolutions
- [x] Full digital music support (libopenmpt, libgme)
- [ ] MIDI music support
- [x] Correct app icon
- [x] Non-hardcoded CMake
     - Paths to libraries are hardcoded on Android, but non-Android builds are untouched.
- [x] SAF support
- [ ] Prepackaged assets
- [x] Distributable build
- [x] Merge into next
- [x] 16 KB page size support
