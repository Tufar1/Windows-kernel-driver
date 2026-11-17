#include "hook.h"

void* originalFunction = 0;
void** ptrStoredOn = 0;

bool doHooking(){
	if(!registerNmiHandler())
		return false;

	auto umEprocess = getEPROCESS(usermodeName);
	if(!umEprocess)
		return false;
	
	uintptr_t outputAddresses[256];
  size_t foundCount = 0;

	if(!scanForPattern(L"ntoskrnl.exe", "ff e2", outputAddresses, &foundCount))//jmp rdx pridat point na add rdx, ?? 
		return false;

	if(foundCount <= 0)
		return false;

	uintptr_t jmpAddress = outputAddresses[foundCount-1];
	if(jmpAddress == 0)
		return false;

  auto sessionState = W32GetSessionState();
	if(sessionState == 0)
		return false;

  auto tableOne = *reinterpret_cast<uint64_t*>(sessionState+0x88);
  if(tableOne == 0)
		return false;

	auto tableTwo = *reinterpret_cast<uint64_t*>(tableOne+0x1B0);
  if(tableTwo == 0)
		return false;

	ptrStoredOn = reinterpret_cast<void**>(tableTwo+0x30);
	if(ptrStoredOn == 0)
		return false;

	uint64_t bufferVA = 0;
	if(!readFromSysReg(L"\\Registry\\Machine\\Software\\ezappka", L"buffah", &bufferVA))	
		return false;

	targetCr3 = *reinterpret_cast<uint64_t*>(umEprocess + EProcess::CR3);

	PHYSICAL_ADDRESS bufferPA{};
	bufferPA.QuadPart = getPhysicalFromVirtual(bufferVA);
	sharedBuffer = reinterpret_cast<umRequest*>(MmMapIoSpace(bufferPA, 0x1000, MEMORY_CACHING_TYPE::MmNonCached));

	DbgPrintEx(0,0,"Shared KM: %p\n", sharedBuffer);
	DbgPrintEx(0,0,"Shared PA: %p\n", bufferPA);

	if(!sharedBuffer)
		return false;
	
	KAPC_STATE apcState;
  KeStackAttachProcess(reinterpret_cast<PRKPROCESS>(umEprocess), &apcState);

  originalFunction = _InterlockedExchangePointer(ptrStoredOn, reinterpret_cast<void*>(jmpAddress));

	KeUnstackDetachProcess(&apcState);

	sharedBuffer->code = opCodes::init;
	sharedBuffer->address = reinterpret_cast<uint64_t>(&controlFunction);

	dataPayload = reinterpret_cast<uint8_t*>(sharedBuffer) + sizeof(umRequest);

	return true;
}

void doUnhooking(){
	unregisterNmiHandler();

	KAPC_STATE apcState = {};
 
	if(getEPROCESS(usermodeName)){
		KeStackAttachProcess(reinterpret_cast<PRKPROCESS>(getEPROCESS(usermodeName)), &apcState); 
		originalFunction = _InterlockedExchangePointer(ptrStoredOn, originalFunction);
  	KeUnstackDetachProcess(&apcState);
	}

	MmUnmapIoSpace(sharedBuffer, 0x1000);

	return;
}
