#ifndef INTERINSICS_H
#define INTERINSICS_H

#include <math.h>

/*
IMPLEMENT THESE FUNCTIONS AND OPTIMIZE.
*/
static float32
FloorFloat(float32 Value) {
  float32 Result = floorf(Value);
  return (Result);
}

static int32
TruncFloat(float32 Value) {
  int32 Result = (int32)(Value + 0.5f);
  return (Result);
}

static uint32
RoundFloat(float32 Value) {
  uint32 Result = (uint32)(Value + 0.5f);

  return Result;
}

static float32
Sin(float32 Angle) {
  float32 Result = sinf(Angle);

  return Result;
}

static float32
Cos(float32 Angle) {
  float32 Result = cosf(Angle);

  return Result;
}

static float32
Atan2(float32 Y, float32 X) {
 float32 Result = atan2f(Y, X);

 return Result;
}

static float32
NormalizeValue(uint32 Value, uint32 Max, uint32 NewMax) {
  float32 NewNumber = (((float32)Value/(float32)Max) * (float32)NewMax); 

  return (float32)(NewNumber);
}

static uint32
RandNum(uint32 Seed) {
  uint32 Modulas = 2000000000;
  uint32 Multiplier = 48271;
  uint32 Result;

  if (Seed == 0) {
    Result = 1;
  } else {
    Result = Seed; 
  }

  Result = ((Multiplier*Result) % Modulas);

  return (Result);

}

static float32
NumberBetween1and0(uint32 Seed) {
  float32 Result = 0;

  uint32 RandomValue = Seed;
  uint32 Modulas = 2000000000;
  uint32 Multiplier = 48271;

  RandomValue = ((Multiplier*RandomValue) % Modulas);

  Result = NormalizeValue(RandomValue, Modulas, 1);

  return Result;
}

static bool
RandBool(uint32 Seed) {

  uint32 Modulas = 2000000000;
  uint32 Multiplier = 48271;
  uint32 Value;

  if (Seed == 0) {
    Value = 1;
  } else {
    Value = Seed; 
  }

  Value = ((Multiplier*Value) % Modulas);

  float32 normalizedValue = NormalizeValue(Value, Modulas, 1.0f);

  return (normalizedValue < 0.5) ? false : true;
}

inline bool
BitScanForward(uint32 *Index, uint32 Value) {
  bool Found = false;
#if COMPILER_MSVC

  Found = _BitScanForward((unsigned long *)Index, (unsigned long)Value);
#else
  for (uint32 Test = 0; Test < 32; ++Test) {
    if(Value & (1 << Test)) {
      *Index = Test;
      Found = true;
      break;
    }
  }
#endif
  return Found;

}

#endif // Include Guard bozo