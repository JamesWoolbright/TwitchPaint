
#ifndef win32struct_h
#define win32struct_h

struct Win32SoundInformation {
  unsigned int RunningSampleIndex;
  int SamplesPerSecond;
  int BytesPerSample;
  int BufferSize;
  int SafetyBytes;
};

struct Win32PixelBuffer {
  BITMAPINFO bitmapInfo;
  void *BitMapMemory;
  int Bitmapwidth;
  int Bitmapheight;
  int bytesPerPixel;
  int BitMapMemorySize;
};

struct win32_game_functions {
  HMODULE GameLibraryDll;
  FILETIME LastWriteTime;
  game_update *Update;
  game_get_sound_samples *GetSoundSamples;

  bool Vaild;
};

#define WIN32_FILENAME_COUNT MAX_PATH
struct win32_record_state {

  char EXEFileName[WIN32_FILENAME_COUNT];
  char *OnePastLastSlash;

};

struct Win32dim {
  int Width;
  int Height;
};

#endif