# VibeScore mobile toolchain environment (fish): source buildscripts/mobile/env.fish
set -q VS_ROOT; or set -gx VS_ROOT $HOME/osm
set -gx VS_TOOLCHAIN $VS_ROOT/toolchain
set -q JAVA_HOME; or set -gx JAVA_HOME /usr/lib/jvm/java-21-openjdk
set -gx ANDROID_SDK_ROOT $VS_TOOLCHAIN/android-sdk
set -gx ANDROID_HOME $ANDROID_SDK_ROOT
set -gx ANDROID_NDK_ROOT $ANDROID_SDK_ROOT/ndk/27.2.12479018
set -gx QT_VERSION_VS 6.10.2
set -gx QT_HOST_PATH $VS_TOOLCHAIN/Qt/$QT_VERSION_VS/gcc_64
set -gx QT_ANDROID_X86_64 $VS_TOOLCHAIN/Qt/$QT_VERSION_VS/android_x86_64
set -gx QT_ANDROID_ARM64 $VS_TOOLCHAIN/Qt/$QT_VERSION_VS/android_arm64_v8a
fish_add_path -gP $VS_TOOLCHAIN/venv/bin $ANDROID_SDK_ROOT/platform-tools $ANDROID_SDK_ROOT/emulator $ANDROID_SDK_ROOT/cmdline-tools/latest/bin
# Release signing key (kept outside the repository), see buildscripts/cmake/SetupAndroidPackaging.cmake
if test -f $VS_TOOLCHAIN/keys/keystore.env
    for line in (string match -r '^export .*' < $VS_TOOLCHAIN/keys/keystore.env)
        set -l kv (string replace 'export ' '' $line)
        set -gx (string split -m 1 '=' $kv)
    end
end
