#include "scanner.h"

auto patternToBytes = [](const char *pattern, short *out_bytes,
                             size_t &out_length) -> void {

    auto is_hex = [](const char c) constexpr -> bool {
      return (c >= '0' && c <= '9') || ((c | 0x20) >= 'a' && (c | 0x20) <= 'f');
    };

    auto hex_value = [](const char c) constexpr -> int {
      return (c >= '0' && c <= '9') ? (c - '0') : ((c | 0x20) - 'a' + 10);
    };

    const char *q = pattern;
    out_length = 0;

    while (*q) {
      
      if (*q == '?') {
        ++q;
        if (*q == '?')
          ++q;
        out_bytes[out_length++] = -1;
      } else if (is_hex(*q)) {

        const int hi = hex_value(*q++);
        int lo = 0;
        if (is_hex(*q))
          lo = hex_value(*q++);
        out_bytes[out_length++] = static_cast<short>((hi << 4) | lo);
      } else {
        ++q;
      }
    }
  };

bool scanForPattern(const wchar_t* module, const char* pattern, uintptr_t* output, size_t* found) {
    UNICODE_STRING scanModuleString;
    RtlInitUnicodeString(&scanModuleString, module);

    uint64_t moduleBase = getKernelModuleBase(scanModuleString);
    if (!moduleBase)
        return false;

    uint64_t ntHeaders = moduleBase + *reinterpret_cast<uint32_t*>(moduleBase + imageDosHeader::e_lfanew);

    uint64_t textSection = 0;

    for (int i = 0; i < *reinterpret_cast<uint16_t*>(ntHeaders + imageFileHeader::NumberOfSections); i++) {
        uint64_t section = (ntHeaders + imageNtHeaders::OptionalHeader + *reinterpret_cast<uint16_t*>(ntHeaders + imageFileHeader::Size)) + (i * imageSectionHeader::Size);
        const char* name = reinterpret_cast<const char*>(section);

        if (strncmp(name, ".text", 8) == 0) {
            textSection = section;
            break;
        }
    }

    if (!textSection)
        return false;

    short bytesBuffer[64];
    size_t length = 0;
    patternToBytes(pattern, bytesBuffer, length);

    auto characteristics = *reinterpret_cast<uint32_t*>(textSection + imageSectionHeader::Characteristics);

    if (!MmIsAddressValid(reinterpret_cast<void*>(moduleBase + *reinterpret_cast<uint32_t*>(textSection + imageSectionHeader::VirtualAddress))))
        return false;

    bool isCode = characteristics & 0x00000020;
    bool isRead = characteristics & 0x40000000;
    bool isExec = characteristics & 0x20000000;
    bool isDiscardable = characteristics & 0x02000000;

    if (!isCode || !isRead || !isExec || isDiscardable)
        return false;

    auto baseAddress = reinterpret_cast<char*>(moduleBase + *reinterpret_cast<uint32_t*>(textSection + imageSectionHeader::VirtualAddress));

    for (unsigned int i = 0; i < *reinterpret_cast<uint32_t*>(textSection + imageSectionHeader::VirtualSize) - length; i++) {
        for (size_t j = 0; j < length; j++) {
            if (bytesBuffer[j] != -1 &&
                bytesBuffer[j] != static_cast<unsigned char>(baseAddress[i + j])) {
                break;
            }

            if (j == length - 1) {
                output[*found] = reinterpret_cast<uintptr_t>(baseAddress + i);
                (*found)++;
            }
        }
    }

    return true;
}
