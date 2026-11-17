#include "memory.h"
#include "global.h"
#include "interrupts.h"
#include "intrinsics.h"

using GreProtectSpriteContent_t = int64_t(*)(int64_t, uint64_t, int32_t, char);

uint64_t getEmptyIndex(uint64_t tableVA){
  for (uint64_t i = 0; i < 512; i++) {
    if(reinterpret_cast<PTE*>(tableVA)[i].present == 0){
      return i;
    }
  }
  return 0;
}

void readBytes(uint64_t PA,uint32_t bytesToRead,uint8_t* outBuffer){//only 4kb mapping
	auto cr3 = read_cr3();
	auto pml4 = cr3 >> 12 << 12;

	PHYSICAL_ADDRESS pml4PA{};
	pml4PA.QuadPart = static_cast<LONGLONG>(pml4);

  auto self_ref = reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(pml4PA));
  if (!self_ref) return;

  uint64_t custom_mapping = self_ref;

	auto p1Index = (custom_mapping >> 12) & 0x1FF;
	auto offset = (custom_mapping) & 0xFFF;
  
	p1Index = getEmptyIndex(self_ref);
  
	auto& custom_mapping_entry = reinterpret_cast<PTE*>(self_ref)[p1Index];

	size_t bytesRead = 0;

	while(bytesRead < bytesToRead){
  	auto currentPA = PA + bytesRead;
		offset = currentPA & 0xFFF;
		
		custom_mapping = (custom_mapping & ~(0x1FF << 12)) | ((p1Index & 0x1FF) << 12);
		custom_mapping = (custom_mapping & ~(0xFFF)) | ((offset & 0xFFF));
  
		custom_mapping_entry.pfn = currentPA >> 12;
  	custom_mapping_entry.rw = true;
  	custom_mapping_entry.present = true;
		
		uint32_t bytesThisPage = 0x1000 - (currentPA & 0xFFF);

		if(bytesThisPage > bytesToRead - bytesRead)
			bytesThisPage = bytesToRead - bytesRead;
  	
		invlpg(reinterpret_cast<void*>(custom_mapping));

		for(uint32_t i = 0; i < bytesThisPage; i += 8){
			*reinterpret_cast<uint64_t*>(outBuffer + bytesRead + i) = *reinterpret_cast<uint64_t*>(custom_mapping + i);
		}

  	custom_mapping_entry.pfn = 0;
  	custom_mapping_entry.rw = false;
  	custom_mapping_entry.present = false;

  	invlpg(reinterpret_cast<void*>(custom_mapping));
		
		bytesRead += 8;
	}
	return;
}

uint64_t getPhysicalFromVirtual(uint64_t VA){
	auto pml4 = targetCr3 >> 12 << 12;
	auto offset = VA & 0xFFF;

	auto p4Index = (VA >> 39) & 0x1FF;
	auto p3Index = (VA >> 30) & 0x1FF;
	auto p2Index = (VA >> 21) & 0x1FF;
	auto p1Index = (VA >> 12) & 0x1FF;

	auto p4e = read<PML4E>((pml4) + p4Index * 8);
  if(!p4e.present)
    return 0;

  auto p3e = read<PML4E>((p4e.pfn << 12) + p3Index*8);
	auto* p3e_ptr = &p3e;
  if(!p3e.present)
    return 0;

  if(reinterpret_cast<PDPTE_1GB*>(p3e_ptr)->page_size)
    return (p3e.pfn << 30) + offset;

  auto p2e = read<PML4E>((p3e.pfn << 12) + p2Index*8);
	auto* p2e_ptr = &p2e;
  if(!p2e.present)
    return 0;

  if(reinterpret_cast<PDE_2MB*>(p2e_ptr)->page_size)
    return (p2e.pfn << 21) + offset;

  auto pe = read<PTE>((p2e.pfn << 12) + p1Index*8);
  if(!pe.present)
    return 0;

  return (pe.pfn << 12) + offset;
}
bool getCR3(){
	const char* name = reinterpret_cast<const char*>(dataPayload);
	
	auto eprocess = getEPROCESS(name);
	if(!eprocess)
		return false;

	targetCr3 = *reinterpret_cast<uint64_t*>(eprocess + EProcess::CR3);
	if(!targetCr3)
		return false;

	targetEprocess = eprocess;

	return true;
}

bool getTargetModuleBase(){
	auto raw = reinterpret_cast<const uint8_t*>(dataPayload);
	if(!targetEprocess)
		return false;

	if(raw[0] == 0xDE && raw[1] == 0xAD){
		*reinterpret_cast<uint64_t*>(dataPayload) = *reinterpret_cast<uint64_t*>(targetEprocess + EProcess::SectionBaseAddress);
		return true;
	}
	
	auto peb = *reinterpret_cast<uint64_t*>(targetEprocess + EProcess::PEB); 
	if(!peb)
		return false;

	auto ldr = read<uint64_t>(getPhysicalFromVirtual(peb) + Peb::Ldr);
	if(!ldr)
		return false;

	auto listHead = ldr + ldrData::InLoadOrderModuleList;
	auto current = read<uint64_t>(getPhysicalFromVirtual(listHead));
	
	auto targetName = reinterpret_cast<const wchar_t*>(dataPayload);

	while(current != listHead){
		auto currentPA = getPhysicalFromVirtual(current); 
		auto moduleBase = read<uint64_t>(currentPA + ldrDataTableEntry::DllBase);
		
		auto nameBuf = read<uint64_t>(currentPA + ldrDataTableEntry::BaseDllName + 0x8);
		auto nameLen = read<uint16_t>(currentPA + ldrDataTableEntry::BaseDllName);
		
		if(nameLen > 0){
			wchar_t buf[256]{};
			auto copyLen = (nameLen < 510) ? nameLen : 510;
			buf[copyLen / 2] = L'\0';

			readBytes(getPhysicalFromVirtual(nameBuf), copyLen, reinterpret_cast<uint8_t*>(buf));	

			UNICODE_STRING current_name{};
      current_name.Length = nameLen;
			current_name.MaximumLength = nameLen + 2;
      current_name.Buffer = buf;
      
			UNICODE_STRING search_name{};
      
			RtlInitUnicodeString(&search_name, targetName);
      if (RtlEqualUnicodeString(&search_name, &current_name, true)){
      	*reinterpret_cast<uint64_t*>(dataPayload) = moduleBase;	
				return true;
			}
		}
		current = read<uint64_t>(currentPA);
	}
	return false;
}

uint64_t __fastcall controlFunction() {
	auto oldCr3 = read_cr3();
	__writecr3(shadowCr3);
	switch(sharedBuffer->code){
		case opCodes::read:{
			if(targetCr3 == 0){
			__writecr3(oldCr3);
				return 0;
			}
			
    	uint64_t PA = getPhysicalFromVirtual(static_cast<uint64_t>(sharedBuffer->address));
    	if(!PA){
			__writecr3(oldCr3);
      	return 0;
			}
			
			readBytes(PA, sharedBuffer->bytesToRW, dataPayload);

			break;
		}

		case opCodes::cr3: {
			if(!getCR3()){
			__writecr3(oldCr3);
				return 0;
			}
			break;
		}
		
		case opCodes::base: {
			if(!getTargetModuleBase()){
			__writecr3(oldCr3);
				return 0;
			}
			break;
		}

		case opCodes::hideOverlay: {
  		uintptr_t outputAddresses[256];
  		size_t foundCount = 0;
		
			if(!scanForPattern(L"win32kfull.sys", "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 41 8B F1", outputAddresses, &foundCount)){
			__writecr3(oldCr3);
				return false;
			}
			
			if(foundCount == 0){
			__writecr3(oldCr3);
				return false;
			}

			GreProtectSpriteContent_t GreProtectSpriteContent = reinterpret_cast<GreProtectSpriteContent_t>(outputAddresses[foundCount-1]);
			//params = 0, hwnd, 1LL, 0x11
			GreProtectSpriteContent(0, sharedBuffer->address, 1LL, 0x11);
			break;
		}
		
		case opCodes::write: {
			__writecr3(oldCr3);
			return 0;
			break;
		}
		
		case opCodes::end: {
			doUnhooking();
			break;
		}
	}
	__writecr3(oldCr3);
  return 1;
}
