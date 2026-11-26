# VelocityFX v0.2.0 — Android64

Target: Geode v6.0.0, Geometry Dash 2.2091, Android64 (arm64-v8a).

## What's implemented in this source
- Left-middle floating VFX tab with an expandable dark panel.
- FPS interval request toggle and preset cycling.
- Player-sprite afterimage trail toggle and intensity cycling.
- Settings are stored using Geode's mod settings API.
- GitHub Actions workflow to build an Android64 `.geode` artifact.

## Important limitations
- The trail is a lightweight sprite afterimage, not true full-screen optical motion blur.
- FPS interval requests do not bypass the phone's display refresh rate, thermal limits, or Geometry Dash's own constraints.
- The source has not been compiled or tested on your exact Android/Geode installation. Cocos/Geode API differences may require fixes after the first build.
- The panel is fixed at the left-middle in this version; dragging/repositioning is not yet implemented.

## Build on Android64
Install Geode CLI, the Geode v6.0.0 SDK binaries, and Android NDK. Set `GEODE_SDK` and `ANDROID_NDK_ROOT`, then run:

```bash
geode sdk install-binaries -p android64
geode build -p android64
```

Copy the output `.geode` file from `build-android64` to:

```text
/storage/emulated/0/Android/media/com.geode.launcher/game/geode/mods/
```

## Build using GitHub Actions
Push this project to a GitHub repository on the `main` branch, or run the workflow manually from the Actions tab. Download the `VelocityFX-Android64` artifact from the completed workflow run.
