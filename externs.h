#pragma once
#include <cstdint>
#include <ntifs.h>

extern "C" uint64_t PsLoadedModuleList;
extern "C" void RtlInitUnicodeString(PUNICODE_STRING String1, PCWSTR);
