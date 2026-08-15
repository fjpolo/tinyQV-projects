#pragma once

#include <stdint.h>

#define STATE_VECTOR_LENGTH 624
#define STATE_VECTOR_M      397 /* changes to STATE_VECTOR_LENGTH also require changes to this */

typedef struct tagMTRand {
  uint32_t mt[STATE_VECTOR_LENGTH];
  int32_t index;
} MTRand;

void seedRand(MTRand* rand, uint32_t seed);
uint32_t genRand32(MTRand* rand);
double genRand(MTRand* rand);
