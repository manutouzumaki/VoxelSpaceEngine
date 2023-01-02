@echo off

if not exist ..\build mkdir ..\build

pushd ..\build
    cl -Od ..\src\*.cpp /Fe.\VoxelSpaceEngine /Zi /link User32.lib Gdi32.lib Winmm.lib Ole32.lib Xinput.lib Xaudio2.lib d3d11.lib d3dcompiler.lib
popd
