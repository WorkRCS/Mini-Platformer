cmake_minimum_required(VERSION 3.20)
project(WildboundNative LANGUAGES CXX)
# Unity is now the primary game/runtime environment. src/ contains the legacy raylib prototype.
enable_testing()
add_subdirectory(NativeCore)
