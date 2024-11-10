@echo off

IF NOT EXIST build mkdir build
pushd build
del *.pdb > NUL 2> NUL
cl -MT -Gm- -nologo -GR- -Oi -DINTERNAL=1 -DNONPERFORMANCE=1 -DGameWin32=1 -Z7 ../PlatformIndependent.cpp /LD /link /INCREMENTAL:NO /PDB:GameCode_%RANDOM%.pdb /EXPORT:GameGetSoundSamples /EXPORT:GameUpdate
cl -MT -Gm- -nologo -GR- -Oi -DINTERNAL=1 -DNONPERFORMANCE=1 -Z7 ../win32_GamePlatform.cpp /link -opt:ref user32.lib gdi32.lib xinput.lib winmm.lib Ws2_32.lib /NODEFAULTLIB:msvcrt.lib
popd build