#pragma once

#include "global.h"
#include "helpers.h"
#include "scanner.h"
#include "structs.h"
#include "hook.h"
#include "intrinsics.h"
#include <cstdint>

uint64_t getPhysicalFromVirtual(uint64_t VA);
uint64_t getEmptyIndex(uint64_t tableVA);
uint64_t __fastcall controlFunction();

template<typename T>
T read(uint64_t PA){//only 4kb mapping
	auto cr3 = read_cr3();
	auto pml4 = cr3 >> 12 << 12;

	PHYSICAL_ADDRESS pml4PA{};
	pml4PA.QuadPart = static_cast<LONGLONG>(pml4);

  auto self_ref = reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(pml4PA));
  if (!self_ref) return static_cast<T>(0);

  uint64_t custom_mapping = self_ref;

	auto p1Index = (custom_mapping >> 12) & 0x1FF;
	auto offset = (custom_mapping) & 0xFFF;
  
	p1Index = getEmptyIndex(self_ref);
  offset = PA & 0xFFF;

	custom_mapping = (custom_mapping & ~(0x1FF << 12)) | ((p1Index & 0x1FF) << 12);
	custom_mapping = (custom_mapping & ~(0xFFF)) | ((offset & 0xFFF));
  
	auto& custom_mapping_entry = reinterpret_cast<PTE*>(self_ref)[p1Index];

  custom_mapping_entry.pfn = PA >> 12;
  custom_mapping_entry.rw = true;
  custom_mapping_entry.present = true;

  invlpg(reinterpret_cast<void*>(custom_mapping));

  T result = *reinterpret_cast<T*>(custom_mapping);

  custom_mapping_entry.pfn = 0;
  custom_mapping_entry.rw = false;
  custom_mapping_entry.present = false;

  return result;
}

template<typename T>
void write(uint64_t PA, const T& value)
{
    auto cr3 = read_cr3();
    auto pml4 = cr3 >> 12 << 12;

    PHYSICAL_ADDRESS pml4PA{};
    pml4PA.QuadPart = static_cast<LONGLONG>(pml4);

    auto self_ref = reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(pml4PA));
    if (!self_ref)
        return;

    uint64_t custom_mapping = self_ref;

    auto p1Index = getEmptyIndex(self_ref);
    auto offset = PA & 0xFFF;

    custom_mapping = (custom_mapping & ~(0x1FFull << 12)) | ((p1Index & 0x1FF) << 12);
    custom_mapping = (custom_mapping & ~0xFFFull) | offset;

    auto& custom_mapping_entry = reinterpret_cast<PTE*>(self_ref)[p1Index];

    custom_mapping_entry.pfn = PA >> 12;
    custom_mapping_entry.rw = true;
    custom_mapping_entry.present = true;

    invlpg(reinterpret_cast<void*>(custom_mapping));

    *reinterpret_cast<T*>(custom_mapping) = value;

    custom_mapping_entry.pfn = 0;
    custom_mapping_entry.rw = false;
    custom_mapping_entry.present = false;

    invlpg(reinterpret_cast<void*>(custom_mapping));
}
