#pragma once

#include "structs.h"
#include <cstdint>

inline uint64_t targetCr3;
inline uint64_t targetEprocess;
inline ULONG64 shadowCr3;

inline const char* usermodeName;

inline uint8_t* dataPayload;

inline umRequest* sharedBuffer;
