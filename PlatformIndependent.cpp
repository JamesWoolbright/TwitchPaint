#include "GamePlatform.h"
#include "interinsics_win32.h"


static void UpdateGameSound(GameSoundBuffer *Buffer,GameState *gameState, int Hz) {

    if (Hz <= 1) {
      return;
    }

    short ToneVolume = 8000;
    int WavePeriod = Buffer->SamplesPerSecond/Hz;

    short *SampleOut = Buffer->Samples;
    for (int SampleIndex = 0; SampleIndex < Buffer->SampleOut; ++SampleIndex) {
        float32 SineValue = sinf(gameState->tSine);
        short SampleValue = (short)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        gameState->tSine += 2.0f*PI32*1.0f/(float32)WavePeriod;
        if(gameState->tSine > 2.0f*PI32) {
          gameState->tSine -= 2.0f*PI32;
        }
    }
}

/*
  Value converted [1, Max] to new range [1, NewMax]. Result not rounded.
*/

/*
Replace this with RGB struct where R,G,B are clamped to one no matter what so that I don't do something
Stupid in the future
*/
static void
SetPixel(PixelBuffer *Buffer, uint32 X, uint32 Y, RGB color) {

  float32 r = (color.R > 1.0f) ? 1.0f : color.R;
  float32 g = (color.G > 1.0f) ? 1.0f : color.G;
  float32 b = (color.B > 1.0f) ? 1.0f : color.B;

    
  if (X >= Buffer->Bitmapwidth || Y >= Buffer->Bitmapheight) {
        return; // Out of bounds, do nothing
  }

  int Pitch = Buffer->Bitmapwidth*Buffer->bytesPerPixel;
  unsigned char *Row = (unsigned char *)Buffer->BitMapMemory;

  int MappedY = (Pitch * Y);
  unsigned int *Pixel = (unsigned int *)(Row + MappedY);
  Pixel += X;

  r = r * 255.0f;
  g = g * 255.0f;
  b = b * 255.0f;

  uint32 Color = ((uint32)RoundFloat(r) << 16) | // Red
                ((uint32)RoundFloat(g) << 8) |  // Green
                ((uint32)RoundFloat(b) << 0);   // Blue

  *Pixel = Color;

}

static void
GetPixel(PixelBuffer *Buffer, RGB *Color, uint32 X, uint32 Y) {

  if (X >= Buffer->Bitmapwidth || Y >= Buffer->Bitmapheight) {
        return; // Out of bounds, do nothing
  }

  int Pitch = Buffer->Bitmapwidth*Buffer->bytesPerPixel;
  unsigned char *Row = (unsigned char *)Buffer->BitMapMemory;


    // Calculate the offset for the specific pixel
  unsigned char *Pixel = Row + (Y * Pitch) + (X * Buffer->bytesPerPixel);


  Color->B = Pixel[0];
  Color->G = Pixel[1];
  Color->R = Pixel[2];

  Color->B = NormalizeValue(Pixel[0], 255, 1);
  Color->G = NormalizeValue(Pixel[1], 255, 1);
  Color->R = NormalizeValue(Pixel[2], 255, 1);

  return;

}

#pragma pack(push, 1)
struct bitmap_header {
  uint16 FileType;
  uint32 FileSize;
  uint16 Reserved1;
  uint16 Reserved2;
  uint32 BitmapOffset;
  uint32 Size;
  uint32 Width;
  uint32 Height;
  uint16 Planes;
  uint16 BitsPerPixel;
  uint32 Compression;
  uint32 ImageSize;
  uint32 XPixelsPerM;
  uint32 ColorsUsed;
  uint32 ImportantColor;
  uint32 ColorTable;

  uint32 RedMask;
  uint32 GreenMask;
  uint32 BlueMask;
};
#pragma pack(pop)


static LoadedBmp
DebugLoadBmp(thread_context *Thread, platform_read_file *PlatformReadFile, char *FileName) {

  LoadedBmp Result = {};

  read_file_result ReadResult = PlatformReadFile(Thread, FileName);

  bitmap_header *Header = (bitmap_header *)ReadResult.Contents;


  if(ReadResult.ContentsSize != 0) {
    Result.BitsPerPixel = Header->BitsPerPixel;
    Result.Width = Header->Width;
    Result.Height = Header->Height;
    uint32 *Pixels = (uint32 *)((uint8 *)ReadResult.Contents + Header->BitmapOffset);
    Result.Pixels = Pixels;

    // Compression 3 BMPs have their byte order determined by a mask.
    // Since my art program only prints out Compression 3 BMPs we will assume
    // A mask value is give and shift accordingly.

    if (Header->Compression != 3) {
      printf("THIS BMP LOADER WAS BUILT WITH COMPRESSION 3 MASKS CAUSE PROGRAMS SUCK");
      return Result;
    }

    
    uint32 RedShift = 0;
    uint32 RedMask = Header->RedMask;
    BitScanForward(&RedShift, RedMask);

    uint32 GreenShift = 0;
    uint32 GreenMask = Header->GreenMask;
    BitScanForward(&GreenShift, GreenMask);

    uint32 BlueShift = 0;
    uint32 BlueMask = Header->BlueMask;
    BitScanForward(&BlueShift, BlueMask);

    uint32 AlphaShift = 0;
    uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
    BitScanForward(&AlphaShift, AlphaMask);


    uint32 *SourceDest = Pixels;
    for (int Y = 0; Y < Header->Height; ++Y) {
      for (int X = 0; X < Header->Width; ++X) {
        uint32 C = *SourceDest;
        *SourceDest++ = (((C >> AlphaShift) & 0xFF) << 24 | 
                         ((C >> RedShift) & 0xFF)   << 16 |
                         ((C >> GreenShift) & 0xFF)  << 8 |
                         ((C >> BlueShift) & 0xFF));
      }
    }

  } 

  return Result;
}

static void
DrawBmp(PixelBuffer *Buffer, LoadedBmp *Bmp, int32 X, int32 Y, uint32 ClipWidth, uint32 ClipHeight, 
        uint32 SourceX,uint32 SourceY) {

  int32 DestX = X;
  int32 DestY = Y;

  int32 ClipX = 0;
  int32 ClipY = 0;


  int32 BltHeight = Bmp->Height + DestY;
  int32 BltWidth = Bmp->Width + DestX;

  if (BltHeight > Buffer->Bitmapheight) {
    BltHeight = Buffer->Bitmapheight;
  }

  if (BltWidth > Buffer->Bitmapwidth) {
    BltWidth = Buffer->Bitmapwidth;
  }

  if (DestX < 0) {
    ClipX += abs(DestX);
    DestX = 0;
  }

  if (DestY < 0) {
    ClipY += abs(DestY);
    DestY = 0;
  }

  if (BltWidth < 0) {
    BltWidth = 0;
  }

  if (BltHeight < 0) {
    BltHeight = 0;
  }

  if (ClipX > Bmp->Width || ClipY > Bmp->Height) {
    return;
  }

  uint32 *SourceRow = Bmp->Pixels + (Bmp->Height - ((uint32)ClipY + 1)) * Bmp->Width + (uint32)ClipX;
  uint8 *DestRow = (uint8 *)Buffer->BitMapMemory + ((uint32)DestY * Buffer->Pitch) + ((uint32)DestX * Buffer->bytesPerPixel);
  // How much you can actually draw

  for (int32 y = DestY; y < BltHeight; ++y) {
    uint32 *Dest = (uint32 *)DestRow;
    uint32 *Source = SourceRow;
    for (int32 x = DestX; x < BltWidth; ++x) {

      // Linear Blending with the formula: (1 - t)A + tB
      // where t is a precentage and B and A are two points or number
      // This equation gives you a smooth blend between the two number A and B.
      RGB Color = {};
      float32 Alpha = ((*Source >> 24) & 0xFF) / 255.0f;

      Color.R = 
      (1.0f-Alpha)*((*Dest >> 16) & 0xFF) + (Alpha * ((*Source >> 16) & 0xFF));

      Color.G = 
      (1.0f-Alpha)*((*Dest >> 8) & 0xFF) + (Alpha * ((*Source >> 8) & 0xFF));

      Color.B = 
      (1.0f-Alpha)*((*Dest >> 0) & 0xFF) + (Alpha * ((*Source >> 0) & 0xFF));

      
      *Dest = ((uint32)Color.R << 16) | ((uint32)Color.G << 8) | (uint32)(Color.B);
      Source++;
      Dest++;

    }
    DestRow += Buffer->Pitch;
    SourceRow -= Bmp->Width;
  }
}

static void
FillBuffer(PixelBuffer *Buffer, RGB Color) {
  for(int Y = 0; Y < Buffer->Bitmapheight; Y++) {
    for(int X = 0; X < Buffer->Bitmapwidth; X++) {
      SetPixel(Buffer, X, Y, Color);
    }
  }
}

static void
DrawSquare(PixelBuffer *Buffer, int X, int Y, int Width, int Height,
           RGB Color) {

  int HalfWidth = Width/2;
  int HalfHeight = Height/2;


  for (int h = 0; h < Height; h++) {
    for (int w = 0; w < Width; w++) {
      SetPixel(Buffer, (X + (w - HalfWidth)), (Y - (h - HalfHeight)), Color);
    }
  }
}

static void
DrawCircle(PixelBuffer *Buffer, int X, int Y, int radius,
           RGB Color) {
  
  for (int w = -radius; w < radius; w++) {
    for (int j = -radius; j < radius; j++) {
      if(((w * w) + (j * j)) <= (radius * radius)) {
        SetPixel(Buffer, X + j, Y + w, Color);
      }
    }
  }
}
static void
DrawLine(PixelBuffer *Buffer, uint32 X1, uint32 Y1, uint32 X2, uint32 Y2, RGB Color, int32 Width) {
  int dx = abs((int)X2 - (int)X1);
  int dy = abs((int)Y2 - (int)Y1);
  int sx = (X1 < X2) ? 1 : -1;
  int sy = (Y1 < Y2) ? 1 : -1; 
  int err = dx - dy;

  while (true) {
    for (int W = -Width; W < Width; W++) {
      SetPixel(Buffer, X1 + W, Y1 + W, Color);
    }

    if (X1 == X2 && Y1 == Y2) break;
    int e2 = 2 * err;
    if (e2 > -dy) { 
      err -= dy; 
      X1 += sx; 
    }
    if (e2 < dx) { 
      err += dx; 
      Y1 += sy; 
    }
  }
}


static uint32
RandNumWithMax(GameState *gameState, uint32 Max) {

  int Seed = RandNum(gameState->SeedForRandomNumber);
  gameState->SeedForRandomNumber = Seed;

  uint32 Modulas = 2000000000;
  uint32 Multiplier = 48271;
  uint32 Result;
  uint32 Maximum;

  if (Seed == 0) {
    Result = 1;
  } else {
    Result = Seed; 
  }

  Result = ((Multiplier*Result) % Modulas);

  if (Max == 0) {
    Maximum = Modulas;
  } else {
    Maximum = Max;
    Result = NormalizeValue(Result, Modulas, Maximum);
  }

  return (Result);
}

static Pos
GetPosOnTileMap(uint32 *Tilemap, Pos Position) {

  Pos Result = {};

  uint32 x = (uint32)FloorFloat(Position.X);
  uint32 y = (uint32)FloorFloat(Position.Y);

  uint32 Tilewidth = 64;
  uint32 Tileheight = 64;

  Result.X = x * Tilewidth;
  Result.Y = y * Tileheight;

}

 extern "C" GAME_UPDATE(GameUpdate) {

    Assert(sizeof(GameState) <= Memory->PermStorageSize);

    GameState *gameState = (GameState *)Memory->PermStorage;
    if(!Memory->Initialized) {
      gameState->PlayerPosition.X = Buffer->Bitmapwidth / 2;
      gameState->PlayerPosition.Y = Buffer->Bitmapheight / 2;
      gameState->CameraPosition.X = gameState->PlayerPosition.X - (Buffer->Bitmapwidth / 2);
      gameState->CameraPosition.Y = gameState->PlayerPosition.Y - (Buffer->Bitmapheight / 2);
      
      gameState->SeedForRandomNumber = 9314;
      Memory->Initialized = true;

      RGB Color = {};
      Color.R = 0.3f;
      Color.G = 0.3f;
      Color.B = 0.3f;
      FillBuffer(Buffer,Color);
    }

    gameController *Input0 = &Input->Controllers[0];
    gameKeyboard *Keyboard = &Input->Keyboard;

    for (int Index = 0; Index < Array_Count(Input->Keyboard.KeyStates); Index ++) {
      if (Input->Keyboard.KeyStates[Index].EndedDown) {
        if(Input->Keyboard.KeyStates[Index].Key == 'W') {
          RGB Color = {};
          FillBuffer(Buffer, Color);
        }
      }
    }

    if (Input->TwitchPlayer.DrawCircleCommand) {
      DrawCircle(Buffer, Input->TwitchPlayer.x1, Input->TwitchPlayer.y1, Input->TwitchPlayer.Radius, Input->TwitchPlayer.Color);
      Input->TwitchPlayer.DrawCircleCommand = false;
    }

    if (Input->TwitchPlayer.DrawSquareCommand) {
      DrawSquare(Buffer, Input->TwitchPlayer.x1, Input->TwitchPlayer.y1, 
      Input->TwitchPlayer.Width, Input->TwitchPlayer.Height, Input->TwitchPlayer.Color);
      Input->TwitchPlayer.DrawSquareCommand = false;
    }

    if (Input->TwitchPlayer.DrawLineCommand) {
      DrawLine(Buffer, Input->TwitchPlayer.x1, Input->TwitchPlayer.y1, Input->TwitchPlayer.x2, Input->TwitchPlayer.y2, Input->TwitchPlayer.Color, Input->TwitchPlayer.Width);
      Input->TwitchPlayer.DrawLineCommand = false;
    }

    


    
};

 extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
    GameState *gameState = (GameState *)Memory->PermStorage;

    UpdateGameSound(SoundBuffer,  gameState, 0);
}















