#pragma once
#include "externs.h"
#include "offsets.h"
#include "scanner.h"
#include "structs.h"
#include <cstdint>

uint64_t getKernelModuleBase(const UNICODE_STRING& name);
uint64_t getEPROCESS(const char* processName);
std::uint64_t W32GetSessionState();
bool readFromSysReg(PCWSTR registerPath, PCWSTR valueName, uint64_t* outBuffer);
