#include "helpers.h"

uint64_t getEPROCESS(const char* processName){
    uint64_t entry = reinterpret_cast<uint64_t>(PsInitialSystemProcess) + EProcess::ActiveProcessLinksOffset;
    for (uint64_t currentEntry = *reinterpret_cast<uint64_t*>(entry); currentEntry != entry;) {

        uint64_t eprocess = currentEntry - EProcess::ActiveProcessLinksOffset;
        char* name = reinterpret_cast<char*>(eprocess + EProcess::ImageFileNameOffset);

        if (_stricmp(name, processName) == 0) {
          if(*reinterpret_cast<uint64_t*>(eprocess + EProcess::ExitTimeOffset) == 0)
            return eprocess;
        }

        currentEntry = *reinterpret_cast<uint64_t*>(currentEntry);
    }
    return 0;
}

uint64_t getKernelModuleBase(const UNICODE_STRING& name) {
    uint64_t listHead = PsLoadedModuleList;
    uint64_t currentEntry = *reinterpret_cast<uint64_t*>(listHead);

    while (currentEntry != listHead) {
        uint64_t entryBase = currentEntry;

        if (RtlEqualUnicodeString(&name, reinterpret_cast<UNICODE_STRING*>(entryBase + ldrDataTableEntry::BaseDllName), 1)) {
            return *reinterpret_cast<uint64_t*>(entryBase + ldrDataTableEntry::DllBase);
        }

        currentEntry = *reinterpret_cast<uint64_t*>(currentEntry); // Flink
    }

    return 0;
}

std::uint64_t W32GetSessionState() {
  auto processSessionId = 1;

  uintptr_t outputAddresses[256];
  size_t foundCount = 0;

  //For 0 session id some different array
  if(processSessionId){
    if(!scanForPattern(L"win32k.sys", "48 8B 15", outputAddresses, &foundCount))
      return 0;

    uintptr_t gSessionGlobalSlotsPtr = 0;

    for (size_t i = 0; i < foundCount; i++) {
      gSessionGlobalSlotsPtr = outputAddresses[i];
    }

    //Parse out pointer from instruction
    auto relativeAddress = *reinterpret_cast<int32_t*>(gSessionGlobalSlotsPtr + 3);
    gSessionGlobalSlotsPtr = gSessionGlobalSlotsPtr + 7 + static_cast<uintptr_t>(relativeAddress);

    std::uint64_t* gSessionGlobalSlots = *reinterpret_cast<std::uint64_t**>(gSessionGlobalSlotsPtr);

    return gSessionGlobalSlots[processSessionId - 1];
  }
  else
  {
    if(!scanForPattern(L"win32k.sys", "48 8D 15 ? ? ? ? 48 8B 0C FA", outputAddresses, &foundCount))
      return 0;

    uintptr_t gLowSessionGlobalSlots = 0;

    for (size_t i = 0; i < foundCount; i++) {
      gLowSessionGlobalSlots = outputAddresses[i];
    }

    //Parse out pointer from instruction
    auto relativeAddress = *reinterpret_cast<int32_t*>(gLowSessionGlobalSlots + 3);
    gLowSessionGlobalSlots = gLowSessionGlobalSlots + 7 + static_cast<uintptr_t>(relativeAddress);

    return *reinterpret_cast<uint64_t*>(gLowSessionGlobalSlots);
  }
}

bool readFromSysReg(PCWSTR registerPath, PCWSTR valueName, uint64_t* outBuffer){
	UNICODE_STRING regPath, valName;
  OBJECT_ATTRIBUTES objAttr;
  void* hKey;

  RtlInitUnicodeString(&regPath, registerPath);

  objAttr.Length = sizeof(OBJECT_ATTRIBUTES);
  objAttr.RootDirectory = nullptr;
  objAttr.ObjectName = &regPath;
  objAttr.Attributes = 64 | 512;
  objAttr.SecurityDescriptor = nullptr;
  objAttr.SecurityQualityOfService = nullptr;

  RtlInitUnicodeString(&valName, valueName);

  if(ZwOpenKey(&hKey, 0xF003F, &objAttr) == STATUS_UNSUCCESSFUL)
		return false;

	ULONG size = 0;
	UCHAR stackBuf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(uint64_t)]{};
  if(ZwQueryValueKey(hKey,&valName,KeyValuePartialInformation,stackBuf,sizeof(stackBuf),&size) == STATUS_UNSUCCESSFUL)
		return false;
  
	if(ZwClose(hKey) == STATUS_UNSUCCESSFUL)
		return false;
	
  auto info = reinterpret_cast<PKEY_VALUE_PARTIAL_INFORMATION>(stackBuf);
  *outBuffer = *reinterpret_cast<uint64_t*>(info->Data);

	return true;
}
