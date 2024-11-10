#ifndef GamePlatform_h
#define GamePlatform_h

/*
Service based design. Inspired by Casey Muratori's
Infact most of this code is yonked from the Handmade hero
series, bar some personal changes.
*/ 
/*

INTERNAL:

  0 - Build for the public
  1 - Build for develpours

NONPERFORMANCE:

  0 - Code that is supposed to be fast
  1 - Code that is intentional slow

*/
#ifndef COMPILER_MSVC
#define COMPILER_MSVC 1
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif


#if COMPILER_MSVC 
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif

#include <math.h>
#include <stdio.h>
#include <stdint.h>

typedef float float32;
typedef double float64;

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;

#define PI32 3.14159265359f

#define Array_Count(Arr) (sizeof(Arr) / sizeof(Arr[0]))

#define Kilabytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilabytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#if NONPERFORMANCE
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif


extern "C" struct read_file_result {
  uint32 ContentsSize;
  void *Contents;
};

struct thread_context {
  int Placeholder;
};

#define PLATFORM_READ_FILE(name) read_file_result name(thread_context *Thread, char *FileName)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) bool name(thread_context *Thread, char *Filename, uint64 MemorySize, void *Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

struct GameSoundBuffer {
    int SamplesPerSecond;
    int SampleOut;
    short *Samples;
};

struct PixelBuffer {
  void *BitMapMemory;
  uint32 Bitmapwidth;
  uint32 Bitmapheight;
  uint32 bytesPerPixel;
  uint32 BitMapMemorySize;
  uint32 Pitch;
};

struct gameButtonState {
  int TransitionCount;
  bool EndedDown;
};

struct Pos {
  float32 X;
  float32 Y;
};

struct gameController {
  
  float32 LStartX;
  float32 LEndX;

  float32 LMinX;
  float32 LMaxX;

  float32 LMinY;
  float32 LMaxY;

  float32 LEndY;
  float32 LStartY;

  float32 RStartX;
  float32 REndX;

  float32 RMinX;
  float32 RMaxX;

  float32 RMinY;
  float32 RMaxY;

  float32 REndY;
  float32 RStartY;


  union 
  {
    gameButtonState controllerButtons[14];
    struct {
      gameButtonState Dpad_Down;
      gameButtonState Dpad_Left;
      gameButtonState Dpad_Right;
      gameButtonState Dpad_Up;
      gameButtonState Start_Button;
      gameButtonState Back_Button;
      gameButtonState A_Button;
      gameButtonState Y_Button;
      gameButtonState B_Button;
      gameButtonState X_Button;
      gameButtonState Left_Shoulder;
      gameButtonState Right_Shoulder;
      gameButtonState Left_Thumb;
      gameButtonState Right_Thumb;
    };
  };
  

  char LTrigger;
  char RTrigger;
};

struct KeyStorage {
  bool EndedDown;
  uint32 Key;
};

struct LoadedBmp {
  uint32 BitsPerPixel;
  uint32 Width;
  uint32 Height;
  uint32 Compression;
  uint32 *Pixels;
};

struct gameKeyboard {

  int MouseX;
  int MouseY;

  int PrevMouseX;
  int PrevMouseY;

  KeyStorage KeyStates[15];
  int TransitionCount;
};

/*
When we get a keyboard event. We send it to the List of keyStates.
The next frame we check if the most recent key ended down then add the next 
key to the next index in the key states.
*/

/*
CLASSIC RGB STRUCT
*/
struct RGB {
  float32 R;
  float32 G;
  float32 B;
};

struct TwitchInput {

  bool DrawLineCommand;
  bool DrawCircleCommand;
  bool DrawSquareCommand;

  uint32 x1;
  uint32 y1;
  uint32 x2;
  uint32 y2;

  RGB Color;

  uint32 Width;
  uint32 Height;
  uint32 Radius;

};

struct gameInput {
  // Clock value here
  float32 SecondsElapsed;
  gameKeyboard Keyboard;
  TwitchInput TwitchPlayer;
  gameController Controllers[1];
};

struct GameState {

  Pos PlayerPosition;
  Pos CameraPosition;

  uint32 SeedForRandomNumber;
  float32 tSine;

};


struct GameMemory {
  bool Initialized; // Is Init but for half Asleep people.
  uint64 PermStorageSize;
  void* PermStorage; // REQUIRED TO BE ZERO AT START UP.
  uint64 TransientStorageSize;
  void* TransientStorage;


  platform_write_entire_file *PlatformWriteEntireFile;
  platform_read_file *PlatformReadFile;
  platform_free_file_memory *PlatformFreeFileMemory;
};


// Remove the L from this and remove the game part. 
// Timing, controller/keyboard input, bitmap buffer, and sound

#define GAME_UPDATE(name) void name(thread_context *Thread, GameMemory *Memory, PixelBuffer *Buffer, gameInput *Input)
typedef GAME_UPDATE(game_update);
GAME_UPDATE(GameUpdateStub) {
  return;
}

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, GameMemory *Memory, GameSoundBuffer *SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
GAME_GET_SOUND_SAMPLES(GameGetSoundSamplesStub) {
  return;
}


#endif /*Gameplatform include guard*/ 