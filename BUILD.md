# Build Cheat NG

## Host Compiling

No special steps are required to build for the host.

## Cross Compiling

## Nintendo Switch

1. [Install Devkitpro on Linux](https://devkitpro.org/wiki/devkitPro_pacman).
2. Ensure these enviroments variables are set:
   * DEVKITPRO
   * DEVKITARM
   * DEVKITPPC
3. Ensure these packages/groups are installed:
   * switch-dev
   * switch-glfw
   * switch-libdrm_nouveau
4. Run cmake with `--toolchain cmake/toolchains/switch.cmake`

## Android

1. Install NDK to your system.
2. set `ANDROID_NDK` enviroment variable to the NDK path.
   * `$ANDROID_NDK/toolchains/llvm` should be a valid path.
3. Run cmake with `--toolchain cmake/toolchains/android.cmake`
