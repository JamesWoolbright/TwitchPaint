
// DEBUG CODE DON"T USE THIS FOR LONGER THEN A BIT
#include <WinSock2.h>
#include <signal.h>
#include <fstream>

SOCKET sock;

#define MAX_TWITCH_MESSAGE_SIZE 512
#define MAX_NUMBER_OF_WORD 256

void DEBUGShutDown() {
    closesocket(sock);
    WSACleanup();
}

void DEBUGOpenSocket() {

    ioctlsocket(sock, FIONBIO, (unsigned long *)1);

    char buffer[MAX_TWITCH_MESSAGE_SIZE];

    WSADATA wsaData;
    WORD wVersionRequired = MAKEWORD(2, 2);
    
    if (WSAStartup(wVersionRequired, &wsaData) != 0) {
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(6667);

    const char *hostname = "irc.chat.twitch.tv";
    hostent* server = gethostbyname(hostname);
    if(server == NULL) {
        WSACleanup();
        closesocket(sock);
        return;
    }

    memcpy(&client.sin_addr, server->h_addr, server->h_length);

    if (connect(sock, (sockaddr *)&client, sizeof(client)) == SOCKET_ERROR) {
        WSACleanup();
        closesocket(sock);
        return;
    }
    const char* authtoken = "PASS WDGEwadf\r\n";
    if (send(sock, authtoken, strlen(authtoken), 0) == SOCKET_ERROR) {
    }
    const char* nickname = "NICK justinfan9999\r\n";
    if (send(sock, nickname, strlen(nickname), 0) == SOCKET_ERROR) {
    }
    const char* channel = "JOIN #idontbyte01\r\n";
    if (send(sock, channel, strlen(channel), 0) == SOCKET_ERROR) {
    }

}
// DEBUG CODE DON'T USE THIS LONGER THEN A BIT
#include "GamePlatform.h"

#include <Windows.h>
#include <Xinput.h>
#include <dsound.h>

#include "win32_structs.h"


static Win32PixelBuffer GlobalPixelBuffer;

static bool running;

static LPDIRECTSOUNDBUFFER Win32SoundBuffer;

PLATFORM_FREE_FILE_MEMORY(PlatformFreeFileMemory) {
  if(Memory) {
    VirtualFree(Memory, 0, MEM_RELEASE);
  }
}

PLATFORM_READ_FILE(PlatformReadFile) {
  read_file_result Result = {};

  HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 
                                  0, OPEN_EXISTING, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER FileSize;
    if (GetFileSizeEx(FileHandle, &FileSize)) {
      Assert(FileSize.QuadPart <= 0xFFFFFFFF);
      uint32 FileSize32 = (uint32)FileSize.QuadPart;
      Result.Contents = VirtualAlloc(0, FileSize32, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      if (Result.Contents) {
        DWORD BytesRead = 0;
        if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && 
          (FileSize32 == BytesRead)) {
          Result.ContentsSize = FileSize32;
        } else {
          PlatformFreeFileMemory(Thread, Result.Contents);
          Result.Contents = 0;
        }

      }

    }
    CloseHandle(FileHandle);
  }

  return(Result);
}


PLATFORM_WRITE_ENTIRE_FILE(PlatformWriteEntireFile) {

  bool Result = false;

  HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 
                                  0, CREATE_ALWAYS, 0, 0);
  if (FileHandle != INVALID_HANDLE_VALUE) {

      DWORD BytesWritten = 0;
      if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
        Result = (BytesWritten == MemorySize);
      } else {

      }

    CloseHandle(FileHandle);
  }

  return(Result);
}

static Win32dim
Win32GetClientDimensions(HWND WindowHandle) {
  Win32dim dimensions;
  RECT ClientRect;

  GetClientRect(WindowHandle, &ClientRect);
  dimensions.Width = ClientRect.right - ClientRect.left;
  dimensions.Height = ClientRect.bottom - ClientRect.top;

  return dimensions;
}


static void
Win32ResizeDIBSection(Win32PixelBuffer *GlobalPixelBuffer, int Width, int Height) {

  if(GlobalPixelBuffer->BitMapMemory) {
    VirtualFree(GlobalPixelBuffer->BitMapMemory, 0, MEM_RELEASE);
  }

  GlobalPixelBuffer->Bitmapwidth = Width;
  GlobalPixelBuffer->Bitmapheight = Height;

  GlobalPixelBuffer->bitmapInfo.bmiHeader.biSize = sizeof(GlobalPixelBuffer->bitmapInfo.bmiHeader);
  GlobalPixelBuffer->bitmapInfo.bmiHeader.biWidth = GlobalPixelBuffer->Bitmapwidth;
  GlobalPixelBuffer->bitmapInfo.bmiHeader.biHeight = -GlobalPixelBuffer->Bitmapheight;
  GlobalPixelBuffer->bitmapInfo.bmiHeader.biPlanes = 1;
  GlobalPixelBuffer->bitmapInfo.bmiHeader.biBitCount = 32;
  GlobalPixelBuffer->bitmapInfo.bmiHeader.biCompression = BI_RGB;


  GlobalPixelBuffer->bytesPerPixel = 4;
  GlobalPixelBuffer->BitMapMemorySize = 
  (GlobalPixelBuffer->Bitmapwidth*GlobalPixelBuffer->Bitmapheight)*GlobalPixelBuffer->bytesPerPixel;

  GlobalPixelBuffer->BitMapMemory = VirtualAlloc(0, GlobalPixelBuffer->BitMapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

}

static void
Win32UpdateScreen(HDC DeviceContext, Win32PixelBuffer *Buffer, int Windowwidth, int Windowheight) {
  // L ratio, do some math nerd. (Aspect ratio)
  StretchDIBits(
    DeviceContext,
    0,0,Buffer->Bitmapwidth,Buffer->Bitmapheight,
    0,0,Buffer->Bitmapwidth,Buffer->Bitmapheight,
    Buffer->BitMapMemory,
    &Buffer->bitmapInfo,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static void
Win32InitDSound(HWND WindowHandle, int SoundBufferSize, int SamplesPerSecond) {

  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  if(DSoundLibrary) {
    direct_sound_create *DirectSoundCreate_ = (direct_sound_create *)
                  GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    LPDIRECTSOUND DirectSound;
    if(SUCCEEDED(DirectSoundCreate_(0, &DirectSound, 0))) {

        WAVEFORMATEX WaveFormat;
        WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
        WaveFormat.nChannels = 2;
        WaveFormat.wBitsPerSample = 16;
        WaveFormat.nSamplesPerSec = SamplesPerSecond;
        WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
        WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
        WaveFormat.cbSize = 8;

      if(SUCCEEDED(DirectSound->SetCooperativeLevel(WindowHandle, DSSCL_PRIORITY))) {
        DSBUFFERDESC BuffScript = {};
        BuffScript.dwSize = sizeof(BuffScript);
        BuffScript.dwFlags = DSBCAPS_PRIMARYBUFFER;

        
        LPDIRECTSOUNDBUFFER PrimaryBuffer;

        if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BuffScript, &PrimaryBuffer, 0))) {
          if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
            } else {
              //Error log.
            }
        }
      } 
      else {
        //Error log this at some point.
      }


      DSBUFFERDESC BuffScript = {};
      BuffScript.dwSize = sizeof(BuffScript);
      BuffScript.dwFlags = 0;
      BuffScript.dwBufferBytes = SoundBufferSize;
      BuffScript.lpwfxFormat = &WaveFormat;

      if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BuffScript, &Win32SoundBuffer, 0))) {
      } else {
        //Error log something.
      }

    } else {
      //Error log anything.
    }
  }
}

static Win32SoundInformation SoundInfo;

static void
Win32ClearSoundBuffer(Win32SoundInformation *SoundInfo) {
  
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if (SUCCEEDED(Win32SoundBuffer->Lock(0, SoundInfo->BufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {

        char *DestSample = (char *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
          *DestSample++ = 0;

        }
        
        DestSample = (char *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
          *DestSample++ = 0;
        }

        Win32SoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

static void 
Win32FillSoundBuffer(Win32SoundInformation *SoundInfo, DWORD BytesToLock, DWORD BytesToWrite, GameSoundBuffer *SoundBuffer) {

      VOID *Region1;
      DWORD Region1Size;
      VOID *Region2;
      DWORD Region2Size;

      if (SUCCEEDED(Win32SoundBuffer->Lock(BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0))) {
        short *DestSample = (short *)Region1;
        short *SourceSample = SoundBuffer->Samples;
        DWORD Region1SampleCount = Region1Size / SoundInfo->BytesPerSample;

        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
          *DestSample++ = *SourceSample++;
          *DestSample++ = *SourceSample++;

          ++SoundInfo->RunningSampleIndex;
        }

        DestSample = (short *)Region2;
        DWORD Region2SampleCount = Region2Size / SoundInfo->BytesPerSample;

        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
          *DestSample++ = *SourceSample++;
          *DestSample++ = *SourceSample++;

          ++SoundInfo->RunningSampleIndex;
        }

        Win32SoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
      
    }
}

inline FILETIME
Win32GetLastWriteTime(char *Filename) {

  FILETIME Result = {};

  WIN32_FIND_DATAA FindData;

  HANDLE FindHandle = FindFirstFileA(Filename, &FindData);
  if (FindHandle != INVALID_HANDLE_VALUE) {
    Result = FindData.ftLastWriteTime;
    FindClose(FindHandle);
  }


  return (Result);

}

static win32_game_functions
Win32LoadGameCodeLibrary(char *GameDllPath, char *TempDllPath) {

  win32_game_functions Result = {};


  CopyFileA(GameDllPath, TempDllPath, FALSE);

  Result.GameLibraryDll = LoadLibraryA(TempDllPath);

  if(Result.GameLibraryDll) {
    Result.Update = (game_update *)GetProcAddress(Result.GameLibraryDll, "GameUpdate");
    Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameLibraryDll, "GameGetSoundSamples");
    Result.LastWriteTime = Win32GetLastWriteTime(GameDllPath);

    Result.Vaild = (Result.Update && Result.GetSoundSamples);
  }

  if (!Result.Vaild) {
    Result.Update = GameUpdateStub;
    Result.GetSoundSamples = GameGetSoundSamplesStub;
  }

  return Result;
}

static void
Win32UnloadGameFunctions(win32_game_functions *GameFunctions) {
  if(GameFunctions->GameLibraryDll) {
    FreeLibrary(GameFunctions->GameLibraryDll);
    GameFunctions->GameLibraryDll = 0;
  }

  GameFunctions->Vaild = false;
  GameFunctions->GetSoundSamples = GameGetSoundSamplesStub;
  GameFunctions->Update = GameUpdateStub;

}

#define X_INPUT_GET_STATE(name) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_STATE* pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStub) {
  return(0);
}

X_INPUT_SET_STATE(XInputSetStub) {
  return(0);
}

static x_input_get_state *XInputGetState_ = XInputGetStub;
static x_input_set_state *XInputSetState_ = XInputSetStub;
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

static void
Win32LoadXInput(void) {
  HMODULE XInputLibrary = LoadLibraryA("xinput9_1_0.dll"); 
  if(XInputLibrary) {
    XInputGetState_ = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState_ = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    return;
  }
}

static void
Win32ProcessControllerInput(int ButtonIndex, DWORD ButtonBit, WORD Buttons,
                            gameButtonState *NewState, gameButtonState *OldState) 
{
    NewState->TransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
    NewState->EndedDown = ((Buttons & ButtonBit) == ButtonBit);
}

static float32
Win32ProcessXInputStick(SHORT ThumbStick, int Deadzone) {
  
  float32 Result = 0;
  if ((ThumbStick) < (-Deadzone)) {
    Result = (float32)(ThumbStick) / 32768.0f;
  } else if ((ThumbStick) > Deadzone) {
    Result = (float32)(ThumbStick) / 32767.0f;
  }
  return Result;
}

static void
Win32ProcessKeyboardInput(gameKeyboard *Keyboard, uint32 KeyBit, int Index, bool IsDown) 
{
  Keyboard->KeyStates[Index].EndedDown = IsDown;
  Keyboard->KeyStates[Index].Key = KeyBit;

  ++Keyboard->TransitionCount;
}

LRESULT CALLBACK 
Win32WindowProcedure(HWND WindowHandle, UINT Message, 
        WPARAM wParameter, LPARAM lParameter) {
  LRESULT result = 0; 

  switch (Message) {
    case WM_SIZE: {
    } break;
    case WM_QUIT: 
    {
      running = false;
      PostQuitMessage(0);
    } break;

    case WM_DESTROY: 
    {
      running = false;
      PostQuitMessage(0);
    } break;
    case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
      Win32dim ClientDim = Win32GetClientDimensions(WindowHandle);

      FillRect(DeviceContext, &Paint.rcPaint, (HBRUSH) (COLOR_WINDOW + 3));
      Win32UpdateScreen(DeviceContext,&GlobalPixelBuffer,ClientDim.Width, ClientDim.Height);

      EndPaint(WindowHandle, &Paint);
    } break;
    default : 
    {
      result = DefWindowProc(WindowHandle, Message, wParameter, lParameter);
    } break;
  }

  return(result);
}


static void
ProcessMessageQueue(gameKeyboard *Keyboard, win32_record_state *State) {

    MSG message = {};

    while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
      if (message.message == WM_QUIT) {
        running = false;
      }

      switch (message.message) {
        case WM_MOUSEMOVE: {

          Keyboard->MouseX = message.lParam & 0xFFFF;
          Keyboard->MouseY = (message.lParam >> 16) & 0xFFFF;

        } break;

        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
          uint32 VKCODE = (uint32)message.wParam;
          bool IsDown = ((message.lParam & (1 << 31)) == 0);
          bool WasDown = ((message.lParam & (1 << 30)) != 0);
          bool AltKeyIsDown = ((message.lParam & (1 << 29)) != 0);

          if(WasDown != IsDown) {

            bool FoundKey = false;

            for (int Index = 0; Index < Array_Count(Keyboard->KeyStates); ++Index) {
              if (Keyboard->KeyStates[Index].Key == VKCODE) {
                Win32ProcessKeyboardInput(Keyboard, VKCODE, Index,IsDown);
                FoundKey = true;

                break;
              }
            }

            if (!FoundKey) {
              for (int Index = 0; Index < Array_Count(Keyboard->KeyStates); ++Index) {
                if (!Keyboard->KeyStates[Index].EndedDown) {
                  Win32ProcessKeyboardInput(Keyboard, VKCODE, Index,IsDown);

                  break;
                }
              }
            }

            if(!WasDown) {

            }

          }

          if ((VKCODE == VK_F4) && AltKeyIsDown) {
            running = false;
            break;
          }

        }break;
        default:
        {
          TranslateMessage(&message);
          DispatchMessageA(&message);
        } break;

      }
    }
}

static int64 PerfFrequency;

inline LARGE_INTEGER
Win32GetWallClock() {
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);

  return Result;
}

inline float32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
    float32 Result = 
    ((float32)(End.QuadPart - Start.QuadPart) / (float32)PerfFrequency);

    return Result;
}

static void
CarStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount,    char *Dest) {

  for (int Index = 0; 
       Index < SourceACount; 
       ++Index) 
  {
    *Dest++ = *SourceA++;
  }

  for (int Index = 0;
       Index < SourceBCount;
       ++Index) 
  {
    *Dest++ = *SourceB++;
  }

  *Dest++ = 0;

}

static void
Win32GetEXEFilepath(win32_record_state *State) {

  DWORD SizeOfFileName = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
  State->OnePastLastSlash = State->EXEFileName;
  for(char *Scan = State->EXEFileName; *Scan; ++Scan) {
    if(*Scan == '\\') {
      State->OnePastLastSlash = Scan + 1;
    }
  }

}

static void
Win32BuildEXEPathFileName(win32_record_state *State, char *FileName,
                          int DestCount, char *Dest) {

  CarStrings(State->OnePastLastSlash - State->EXEFileName, State->EXEFileName, 
             sizeof(FileName),                      FileName,
             DestCount,                                 Dest);
}

static void
ReciveFromTwitchIRC(TwitchInput &Twitch) {
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    struct timeval timeout = {0, 100000};

    int PollResult = select(sock + 1, &readfds, NULL, NULL, &timeout);

    if (PollResult < 0) {
      printf("select() failed: attempting a reconnect");
      DEBUGShutDown();
      DEBUGOpenSocket();
    }

    if (PollResult > 0 && FD_ISSET(sock, &readfds)) {
      char messageBuffer[MAX_TWITCH_MESSAGE_SIZE];
      int errorcode = recv(sock, messageBuffer, sizeof(messageBuffer) - 1, 0);
      if (errorcode > 0)  {
        messageBuffer[errorcode] = '\0';

        if(strncmp(messageBuffer, "PING", 4) == 0) {
            const char* PONG = "PONG :tmi.twitch.tv\r\n";
            send(sock, PONG, strlen(PONG), 0);
            return;
        }

        int indexColon = -1;
        for (int i = errorcode - 1; i >= 0; i--) {
            if (messageBuffer[i] == ':') {
                indexColon = i;
                break;
            }
        }

        if (indexColon != -1) {
          char *Message = &messageBuffer[indexColon + 1];
          int CommandIndice = 0;
          int ImportantIndices[MAX_NUMBER_OF_WORD]{};
          int NumberOfWords = 0;
          bool InsideParentheses = 0;
          char Command[MAX_TWITCH_MESSAGE_SIZE]{};

          for (int i = 0; i < strlen(Message); i++) {
            if (Message[i] == '-') {
              printf("No negatives blud");
              CommandIndice = 0;
              InsideParentheses = false;
              break;
            }           

            if (Message[i] == '(') {
              ImportantIndices[NumberOfWords++] = i + 1;
              InsideParentheses = true;
            }

            if (Message[i] == ',' && InsideParentheses) {
              ImportantIndices[NumberOfWords++] = i + 1;
            }

            if (Message[i] == ')') {
              InsideParentheses = false;
            }

            if (Message[i] == Message[0]) {
              ImportantIndices[i] = 0;
              NumberOfWords = 1;
            }

            if ((NumberOfWords == 1) && (Message[i] == ' ')) {
               CommandIndice = i;
            }

          }

          if (InsideParentheses == true) {
            printf("format is wrong. Need to close Parentheses");
            return;
          }

          for (uint32 LetterIndex = 0; LetterIndex < CommandIndice; LetterIndex++) {
            Command[LetterIndex] += Message[LetterIndex];
          }

          if (strcmp(Command, "!drawline") == 0 || strcmp(Command, "drawline") == 0) {

            Twitch.x1 = strtoul(Message + ImportantIndices[1], NULL, 10);
            Twitch.y1 = strtoul(Message + ImportantIndices[2], NULL, 10);
            Twitch.x2 = strtoul(Message + ImportantIndices[3], NULL, 10);
            Twitch.y2 = strtoul(Message + ImportantIndices[4], NULL, 10);

            if (NumberOfWords > 5) {
              Twitch.Width = strtoul(Message + ImportantIndices[5], NULL, 10);
              if (Twitch.Width > 100) {
                Twitch.Width = 100;
              }
            } else {
              Twitch.Width = 1;
            }

            if (NumberOfWords > 8) {
              Twitch.Color.R = (strtoul(Message + ImportantIndices[6], NULL, 10) / 255.0f);
              Twitch.Color.G = (strtoul(Message + ImportantIndices[7], NULL, 10) / 255.0f);
              Twitch.Color.B = (strtoul(Message + ImportantIndices[8], NULL, 10) / 255.0f);
            } else {
              Twitch.Color.R = 1.0f;
              Twitch.Color.G = 1.0f;
              Twitch.Color.B = 1.0f;
            }


            if (Twitch.x1 > Twitch.x2) {
              uint32 Temp = Twitch.x2;
              Twitch.x2 = Twitch.x1;
              Twitch.x1 = Temp;
            }

            if ((Twitch.y1 > Twitch.y2)) {
              uint32 Temp = Twitch.y2;
              Twitch.y2 = Twitch.y1;
              Twitch.y1 = Temp;
            }

            Twitch.DrawLineCommand = true;

          }

          if (strcmp(Command, "!drawsquare") == 0 || strcmp(Command, "drawsquare") == 0) {

            Twitch.x1 = strtoul(Message + ImportantIndices[1], NULL, 10);
            Twitch.y1 = strtoul(Message + ImportantIndices[2], NULL, 10);

            Twitch.x2 = 0;
            Twitch.y2 = 0;


            if (NumberOfWords > 4) {
              Twitch.Width = strtoul(Message + ImportantIndices[3], NULL, 10);
              if (Twitch.Width > 100) {
                Twitch.Width = 100;
              }
              Twitch.Height = strtoul(Message + ImportantIndices[4], NULL, 10);
              if (Twitch.Height > 100) {
                Twitch.Height = 100;
              }
            } else {
              Twitch.Width = 1;
              Twitch.Height = 1;
            }

            if (NumberOfWords > 7) {
              Twitch.Color.R = (strtoul(Message + ImportantIndices[5], NULL, 10) / 255.0f);
              Twitch.Color.G = (strtoul(Message + ImportantIndices[6], NULL, 10) / 255.0f);
              Twitch.Color.B = (strtoul(Message + ImportantIndices[7], NULL, 10) / 255.0f);
            } else {
              Twitch.Color.R = 1.0f;
              Twitch.Color.G = 1.0f;
              Twitch.Color.B = 1.0f;
            }

            Twitch.DrawSquareCommand = true;

          }

          if (strcmp(Command, "!drawcircle") == 0 || strcmp(Command, "drawcircle") == 0) {

            Twitch.x1 = strtoul(Message + ImportantIndices[1], NULL, 10);
            Twitch.y1 = strtoul(Message + ImportantIndices[2], NULL, 10);

            Twitch.x2 = 0;
            Twitch.y2 = 0;

            if (NumberOfWords > 3) {
              Twitch.Radius = strtoul(Message + ImportantIndices[3], NULL, 10);
              if (Twitch.Radius > 100) {
                Twitch.Radius = 100;
              }
            } else {
              Twitch.Radius = 1;
            }

            if (NumberOfWords > 6) {
              Twitch.Color.R = (strtoul(Message + ImportantIndices[4], NULL, 10) / 255.0f);
              Twitch.Color.G = (strtoul(Message + ImportantIndices[5], NULL, 10) / 255.0f);
              Twitch.Color.B = (strtoul(Message + ImportantIndices[6], NULL, 10) / 255.0f);
            } else {
              Twitch.Color.R = 1.0f;
              Twitch.Color.G = 1.0f;
              Twitch.Color.B = 1.0f;
            }


            Twitch.DrawCircleCommand = true;

          }

          }
          fflush(stdout);
      }
    }
}

int CALLBACK WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
) {

  thread_context Thread = {};

  win32_record_state RecordState = {};

  Win32GetEXEFilepath(&RecordState);

  char GameDllName[] = "pb_maina.dll";
  char GameDllPath[MAX_PATH];

  Win32BuildEXEPathFileName(&RecordState, GameDllName, sizeof(GameDllPath), GameDllPath);


  char TempDllName[] = "pb_maina_copy.dll";
  char TempDllPath[MAX_PATH];

  Win32BuildEXEPathFileName(&RecordState, TempDllName, sizeof(TempDllPath), TempDllPath);

// Set scheduler granularity to 1ms for accurate Sleep timing in games.
  timeBeginPeriod(1);

  WNDCLASSA windowClass = {};
  windowClass.lpfnWndProc = Win32WindowProcedure;
  windowClass.hInstance = hInstance;
  windowClass.lpszClassName = "TTS window";
  windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

  if(!RegisterClassA(&windowClass)) {
    OutputDebugStringA("PROBLEM");
  }

  HWND windowHandle = CreateWindowExA(
    0,
    windowClass.lpszClassName,
    "TTS window",
    WS_POPUP | WS_VISIBLE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    960,
    540,
    0,
    0,
    hInstance,
    0
  );

  #define PIXEL_BUFFER_WIDITH 960
  #define PIXEL_BUFFER_HEIGHT 540
  Win32ResizeDIBSection(&GlobalPixelBuffer, PIXEL_BUFFER_WIDITH, PIXEL_BUFFER_HEIGHT);

  if (!windowHandle) {
    OutputDebugStringA("PROBLEM");
  }

  HDC DeviceContext = GetDC(windowHandle);

  running = true; 


  float32 MonitorRefreshRate = 60;
  float32 Win32RefreshRate = GetDeviceCaps(DeviceContext, VREFRESH);
  if (Win32RefreshRate > 1) {
    MonitorRefreshRate = Win32RefreshRate;
  }
  float32 GameUpdateRate = MonitorRefreshRate / 2;
  float32 SecondsElapsedPerFrame = 1.0f / (float32)GameUpdateRate;

  Win32SoundInformation SoundInfo = {};
  SoundInfo.SamplesPerSecond = 48000;
  SoundInfo.BytesPerSample = (sizeof(short) * 2);
  SoundInfo.BufferSize = SoundInfo.SamplesPerSecond * SoundInfo.BytesPerSample;
  SoundInfo.SafetyBytes = (int)(((float32)SoundInfo.SamplesPerSecond*(float32)SoundInfo.BytesPerSample) 
  / GameUpdateRate) / 3;
  Win32LoadXInput();
  Win32InitDSound(windowHandle, SoundInfo.BufferSize, SoundInfo.SamplesPerSecond);

  short *Samples = (short *)VirtualAlloc(0, SoundInfo.BufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if INTERNAL
    LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
    LPVOID BaseAddress = 0;
#endif

  
  GameMemory Memory = {};
  Memory.PermStorageSize = Megabytes(16);
  Memory.TransientStorageSize = Gigabytes(4);

  Memory.PlatformFreeFileMemory = PlatformFreeFileMemory;
  Memory.PlatformReadFile = PlatformReadFile;
  Memory.PlatformWriteEntireFile = PlatformWriteEntireFile;

  int TotalSize = Memory.PermStorageSize + Memory.TransientStorageSize;

  Memory.PermStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

  Memory.TransientStorage = ((uint8 *)Memory.PermStorage + Memory.PermStorageSize);

  Win32ClearSoundBuffer(&SoundInfo);
  Win32SoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

  gameInput NewInput = {};
  gameInput OldInput = {};

  LARGE_INTEGER LastCounter = Win32GetWallClock();
  LARGE_INTEGER FlipWallClock = Win32GetWallClock();

  uint64 LastCycleCount = __rdtsc();

  LARGE_INTEGER FrequancyStore;
  QueryPerformanceFrequency(&FrequancyStore);
  PerfFrequency = FrequancyStore.QuadPart;

  DWORD LastPlayCursor = 0;
  bool SoundIsVaild = false;


  win32_game_functions Game_Functions = Win32LoadGameCodeLibrary(GameDllPath, TempDllPath);


  while(running) {
    NewInput.SecondsElapsed = SecondsElapsedPerFrame;
    FILETIME NewestWriteTime = Win32GetLastWriteTime(GameDllPath);

    if (CompareFileTime(&NewestWriteTime, &Game_Functions.LastWriteTime) != 0) {
      Win32UnloadGameFunctions(&Game_Functions);
      Game_Functions = Win32LoadGameCodeLibrary(GameDllPath, TempDllPath);
    }
    
    gameKeyboard *NewKeyboard = &NewInput.Keyboard;
    gameKeyboard *OldKeyboard = &OldInput.Keyboard;
    gameKeyboard ZeroKeyboard = {};
    *NewKeyboard = ZeroKeyboard;

    for (int Index = 0; 
         Index < Array_Count(NewKeyboard->KeyStates); 
         ++Index) {
      
      NewKeyboard->KeyStates[Index].EndedDown = 
      OldKeyboard->KeyStates[Index].EndedDown;

      if (NewKeyboard->KeyStates[Index].EndedDown) {
        NewKeyboard->KeyStates[Index].Key = OldKeyboard->KeyStates[Index].Key;
      }
    }

    NewKeyboard->PrevMouseX = OldKeyboard->MouseX;
    NewKeyboard->PrevMouseY = OldKeyboard->MouseY;

    ProcessMessageQueue(NewKeyboard, &RecordState);


    for(DWORD ControllerIndex = 0; ControllerIndex < 1; ControllerIndex++) {
      XINPUT_STATE ControllerState;
      if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

        gameController *OldController = &OldInput.Controllers[0]; 
        gameController *NewController = &NewInput.Controllers[0]; 
        
        
        Win32ProcessControllerInput(0, XINPUT_GAMEPAD_DPAD_DOWN, Pad->wButtons, 
                                    &NewController->controllerButtons[0], &OldController->controllerButtons[0]);
        Win32ProcessControllerInput(1, XINPUT_GAMEPAD_DPAD_LEFT, Pad->wButtons, 
                                    &NewController->controllerButtons[1], &OldController->controllerButtons[1]);
        Win32ProcessControllerInput(2, XINPUT_GAMEPAD_DPAD_RIGHT, Pad->wButtons, 
                                    &NewController->controllerButtons[2], &OldController->controllerButtons[2]);
        Win32ProcessControllerInput(3, XINPUT_GAMEPAD_DPAD_UP, Pad->wButtons, 
                                    &NewController->controllerButtons[3], &OldController->controllerButtons[3]);
        Win32ProcessControllerInput(4, XINPUT_GAMEPAD_START, Pad->wButtons, 
                                    &NewController->controllerButtons[4], &OldController->controllerButtons[4]);
        Win32ProcessControllerInput(5, XINPUT_GAMEPAD_BACK, Pad->wButtons, 
                                    &NewController->controllerButtons[5], &OldController->controllerButtons[5]);
        Win32ProcessControllerInput(6, XINPUT_GAMEPAD_A, Pad->wButtons, 
                                    &NewController->controllerButtons[6], &OldController->controllerButtons[6]);
        Win32ProcessControllerInput(7, XINPUT_GAMEPAD_Y, Pad->wButtons, 
                                    &NewController->controllerButtons[7], &OldController->controllerButtons[7]);
        Win32ProcessControllerInput(8, XINPUT_GAMEPAD_B, Pad->wButtons, 
                                    &NewController->controllerButtons[8], &OldController->controllerButtons[8]);
        Win32ProcessControllerInput(9, XINPUT_GAMEPAD_X, Pad->wButtons, 
                                    &NewController->controllerButtons[9], &OldController->controllerButtons[9]);
        Win32ProcessControllerInput(10, XINPUT_GAMEPAD_RIGHT_SHOULDER, Pad->wButtons, 
                                    &NewController->controllerButtons[10], &OldController->controllerButtons[10]);
        Win32ProcessControllerInput(11, XINPUT_GAMEPAD_LEFT_SHOULDER, Pad->wButtons, 
                                    &NewController->controllerButtons[11], &OldController->controllerButtons[11]);
        Win32ProcessControllerInput(12, XINPUT_GAMEPAD_LEFT_THUMB, Pad->wButtons, 
                                    &NewController->controllerButtons[12], &OldController->controllerButtons[12]);
        Win32ProcessControllerInput(13, XINPUT_GAMEPAD_RIGHT_THUMB, Pad->wButtons, 
                                    &NewController->controllerButtons[13], &OldController->controllerButtons[13]);

        float32 X = Win32ProcessXInputStick(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        float32 Y = Win32ProcessXInputStick(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

        NewController->LStartX = OldController->LEndX;
        NewController->LStartY = OldController->LEndY;

        NewController->LMinX = NewController->LMaxX = NewController->LEndX = X;
        NewController->LMinY = NewController->LMaxY = NewController->LEndY = Y;

         
      } else {
        // Controller not connected or disconnected
      }
    }

    DWORD BytesToWrite = 0;


    PixelBuffer Buffer = {};
    Buffer.Bitmapheight = GlobalPixelBuffer.Bitmapheight;
    Buffer.Bitmapwidth = GlobalPixelBuffer.Bitmapwidth;
    Buffer.BitMapMemory = GlobalPixelBuffer.BitMapMemory;
    Buffer.bytesPerPixel = 4;
    Buffer.Pitch = Buffer.Bitmapwidth * Buffer.bytesPerPixel;
    Buffer.BitMapMemorySize = GlobalPixelBuffer.BitMapMemorySize;

    ReciveFromTwitchIRC(NewInput.TwitchPlayer);

    Game_Functions.Update(&Thread, &Memory, &Buffer, &NewInput);

    LARGE_INTEGER AudioWallClock = Win32GetWallClock();
    float32 BeginAudioSeconds = 1000.0f*Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

    DWORD PlayCursor;
    DWORD WriteCursor;
    if ((Win32SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)) == DS_OK) {

        if (!SoundIsVaild) {
          SoundInfo.RunningSampleIndex = WriteCursor / SoundInfo.BytesPerSample;
          SoundIsVaild = true;
        }
        DWORD FoorseenBytesPerFrame = 
        (int)(((float32)SoundInfo.SamplesPerSecond * (float32)SoundInfo.BytesPerSample) / GameUpdateRate);

        DWORD SecondsLeftTillFlip = (SecondsElapsedPerFrame - BeginAudioSeconds);
        DWORD ExpectedSoundBytesTillFlip = 
        (DWORD)((SecondsLeftTillFlip/SecondsElapsedPerFrame)*
        (float32)FoorseenBytesPerFrame);

        DWORD FoorseenFrameBoundaryByte = PlayCursor + ExpectedSoundBytesTillFlip;
        DWORD SafeWriteCursor = WriteCursor;
        if(SafeWriteCursor < PlayCursor) {
          SafeWriteCursor += SoundInfo.BufferSize;
        }

        SafeWriteCursor += SoundInfo.SafetyBytes;
        bool WeakAudioCard = (SafeWriteCursor >= FoorseenFrameBoundaryByte);

        DWORD BytesToLock = 
        (SoundInfo.RunningSampleIndex*SoundInfo.BytesPerSample) % SoundInfo.BufferSize;

        DWORD TargetCursor = 0;
        if (WeakAudioCard) {
          TargetCursor = 
          ((WriteCursor + FoorseenBytesPerFrame + SoundInfo.SafetyBytes) % SoundInfo.BufferSize);
        } else {
          TargetCursor =
          ((FoorseenFrameBoundaryByte + FoorseenBytesPerFrame) % SoundInfo.BufferSize);
        }

        if(BytesToLock == TargetCursor) {
          BytesToWrite = 0;
        }

        if(BytesToLock > TargetCursor) {
          BytesToWrite = (SoundInfo.BufferSize - BytesToLock);
          BytesToWrite += TargetCursor;
        } else {
          BytesToWrite = TargetCursor - BytesToLock;
        }

        GameSoundBuffer SoundBuffer = {};
        SoundBuffer.SamplesPerSecond = SoundInfo.SamplesPerSecond;
        SoundBuffer.SampleOut = BytesToWrite / SoundInfo.BytesPerSample;
        SoundBuffer.Samples = Samples;
        Game_Functions.GetSoundSamples(&Thread, &Memory, &SoundBuffer);

        Win32FillSoundBuffer(&SoundInfo, BytesToLock, BytesToWrite, &SoundBuffer);

  #if INTERNAL

        Win32SoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

        DWORD UnwrappedCursor = WriteCursor;
        if (UnwrappedCursor < PlayCursor) {
          UnwrappedCursor += SoundInfo.BufferSize;
        }
        DWORD AudioLatacnyinBytes = (UnwrappedCursor - PlayCursor);
        float32 AudioLatencyinSeconds =
        (AudioLatacnyinBytes / SoundInfo.BytesPerSample) / SoundInfo.SamplesPerSecond;
  #endif


    } else {
      SoundIsVaild = false;
    }


    LARGE_INTEGER EndCounter = Win32GetWallClock();
    float32 SecondsElapsedCom = Win32GetSecondsElapsed(LastCounter, EndCounter);


    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;

    if (SecondsElapsedCom < SecondsElapsedPerFrame) {
      while (SecondsElapsedCom < SecondsElapsedPerFrame) {
        DWORD SleepMS = (DWORD)(1000.0f * (SecondsElapsedPerFrame - SecondsElapsedCom));
        if (SleepMS > 0) {
          Sleep(SleepMS);
        }
        SecondsElapsedCom = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
      }
    } else {
      //Missed Frame
    }

    POINT Position = {};
    GetCursorPos(&Position);


    EndCounter = Win32GetWallClock();
    LastCounter = EndCounter;

    Win32dim ClientDim = Win32GetClientDimensions(windowHandle);
    Win32UpdateScreen(DeviceContext, &GlobalPixelBuffer, ClientDim.Width, ClientDim.Height);

    FlipWallClock = Win32GetWallClock();

    uint64 EndCycleCount = __rdtsc();
    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
    LastCycleCount = EndCycleCount;

    gameInput Temp = OldInput;
    OldInput = NewInput;
    NewInput = Temp;

  }

  return(0);
  // change
}