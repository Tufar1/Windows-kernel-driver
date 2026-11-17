#pragma once

#include "offsets.h"
#include "externs.h"
#include "helpers.h"

bool scanForPattern(const wchar_t* module, const char* pattern, uintptr_t* output, size_t* found);
