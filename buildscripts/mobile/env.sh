# VibeScore mobile toolchain environment (bash/zsh): source buildscripts/mobile/env.sh
# The toolchain lives in ~/osm/toolchain (~/osm must be a space-free path to the project folder)
export VS_ROOT="${VS_ROOT:-$HOME/osm}"
export VS_TOOLCHAIN="$VS_ROOT/toolchain"
export JAVA_HOME="${JAVA_HOME:-/usr/lib/jvm/java-21-openjdk}"
export ANDROID_SDK_ROOT="$VS_TOOLCHAIN/android-sdk"
export ANDROID_HOME="$ANDROID_SDK_ROOT"
export ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/27.2.12479018"
export QT_VERSION_VS="6.10.2"
export QT_HOST_PATH="$VS_TOOLCHAIN/Qt/$QT_VERSION_VS/gcc_64"
export QT_ANDROID_X86_64="$VS_TOOLCHAIN/Qt/$QT_VERSION_VS/android_x86_64"
export QT_ANDROID_ARM64="$VS_TOOLCHAIN/Qt/$QT_VERSION_VS/android_arm64_v8a"
export PATH="$VS_TOOLCHAIN/venv/bin:$ANDROID_SDK_ROOT/platform-tools:$ANDROID_SDK_ROOT/emulator:$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$PATH"
# Release signing key (kept outside the repository), see buildscripts/cmake/SetupAndroidPackaging.cmake
[ -f "$VS_TOOLCHAIN/keys/keystore.env" ] && . "$VS_TOOLCHAIN/keys/keystore.env"

# Builds share the computer with the user: few parallel jobs, idle CPU and I/O priority.
# VS_JOBS=N overrides the number of parallel jobs.
export VS_JOBS="${VS_JOBS:-4}"
# Gradle (APK/AAB packaging) must not leave a daemon behind
export GRADLE_OPTS="${GRADLE_OPTS:--Dorg.gradle.daemon=false -Dorg.gradle.workers.max=2}"
vs_lowprio() {
    chrt -i 0 nice -n 39 ionice -c 3 "$@"
}
