#include "interrupts.h"

void unregisterNmiHandler(){
	return;
}

bool registerNmiHandler(){
	PHYSICAL_ADDRESS Low = {};
	Low.QuadPart = 0;

	PHYSICAL_ADDRESS High;
	High.QuadPart = MAXULONG64;	
	
	auto AllocatedPML4= ExAllocatePool2(
    POOL_FLAG_NON_PAGED,
    4096,
    'btgP'
	);
	
	PHYSICAL_ADDRESS AllocatedPML4PA = MmGetPhysicalAddress(AllocatedPML4);

	DbgPrintEx(0,0, "Page NmiHandler: %p\n",AllocatedPML4);	
	
	auto cr3 = read_cr3();

	auto pml4 = cr3 >> 12 << 12;
	PHYSICAL_ADDRESS pml4PA{};
	pml4PA.QuadPart = static_cast<LONGLONG>(pml4);

  pml4 = reinterpret_cast<uint64_t>(MmGetVirtualForPhysical(pml4PA));
	
	DbgPrintEx(0, 0, "original pml4PA: %p\n", pml4PA);
	DbgPrintEx(0, 0, "original pml4: %p\n",	pml4);
	
	DbgPrintEx(0, 0, "shadow pml4PA: %p\n", AllocatedPML4PA);
	DbgPrintEx(0, 0, "shadow pml4: %p\n",	AllocatedPML4);

	memcpy(
    reinterpret_cast<void*>(AllocatedPML4),
    reinterpret_cast<void*>(pml4),
   4096 
	);

	shadowCr3 = AllocatedPML4PA.QuadPart;

	auto shadowPml4 = (PML4E*)AllocatedPML4;
	for(int i = 0; i < 512; i++){
		if(shadowPml4[i].present && shadowPml4[i].pfn == pml4PA.QuadPart >> 12){
			DbgPrintEx(0,0, "Found selfref\n");
			shadowPml4[i].pfn = AllocatedPML4PA.QuadPart >> 12;
			break;
		}
	}

	DbgPrintEx(0,0,"Before PA: %p CR3: %p\n", MmGetPhysicalAddress(&MmCopyMemory), read_cr3());
	
	__writecr3(shadowCr3);

	DbgPrintEx(0,0,"After PA: %p CR3: %p\n", MmGetPhysicalAddress(&MmCopyMemory), read_cr3());
	
	auto IDTR = read_idtr();
	auto NmiHandlerAddress = IDTR.base + sizeof(IDTR) * 2;
	
	auto qword1 = *reinterpret_cast<uint64_t*>(NmiHandlerAddress);
	auto qword2 = *reinterpret_cast<uint64_t*>(NmiHandlerAddress + 0x8);

	auto NmiHandler = (qword1 & 0xFFFFFFFFFFFF0000) | ((qword2 >> 32) & 0xFFFF);	

	DbgPrintEx(0, 0, "Nmi Handler: %p\n", NmiHandler);

	pml4 = read_cr3() >> 12 << 12;
	auto offset = NmiHandler & 0xFFF;

	auto p4Index = (NmiHandler >> 39) & 0x1FF;
	auto p3Index = (NmiHandler >> 30) & 0x1FF;
	auto p2Index = (NmiHandler >> 21) & 0x1FF;
	auto p1Index = (NmiHandler >> 12) & 0x1FF;

	auto p4e = read<PML4E>((pml4) + p4Index * 8);
  if(!p4e.present)
    return 0;

  auto p3e = read<PML4E>((p4e.pfn << 12) + p3Index*8);
	auto* p3e_ptr = &p3e;
  if(!p3e.present)
    return 0;

  auto p2e = read<PML4E>((p3e.pfn << 12) + p2Index*8);
	auto* p2e_ptr = &p2e;
  if(!p2e.present)
    return 0;

  if(!reinterpret_cast<PDE_2MB*>(p2e_ptr)->page_size)
		return 0;
	
	DbgPrintEx(0,0,"p2e pfn: %p\n", p2e.pfn);
	//COPY PDPT to shadow
	auto AllocatedPDPT= ExAllocatePool2(
    POOL_FLAG_NON_PAGED,
    4096,
    'tpdP'
	);
	
	PHYSICAL_ADDRESS PDPTPa{};
	PDPTPa.QuadPart = p4e.pfn << 12;
  auto PDPTVa = MmGetVirtualForPhysical(PDPTPa);

	memcpy(
    reinterpret_cast<void*>(AllocatedPDPT),
    reinterpret_cast<void*>(PDPTVa),
   4096 
	);	
	//COPY PD to shadow	
	auto AllocatedPD= ExAllocatePool2(
    POOL_FLAG_NON_PAGED,
    4096,
    'BTdP'
	);
	
	PHYSICAL_ADDRESS PDPa{};
	PDPa.QuadPart = p4e.pfn << 12;
  auto PDVa = MmGetVirtualForPhysical(PDPa);
	
	memcpy(
    reinterpret_cast<void*>(AllocatedPD),
    reinterpret_cast<void*>(PDVa),
   4096 
	);

	auto AllocatedPT= ExAllocatePool2(
    POOL_FLAG_NON_PAGED,
    4096,
    'BTtP'
	);
	
	auto PTPa = MmGetPhysicalAddress(AllocatedPT);
	
	auto pt = reinterpret_cast<PTE*>(AllocatedPT);
	for(int i = 0; i < 512; i++){
		pt[i].present = 1;
		pt[i].rw = p2e.rw;
		pt[i].us = p2e.us;
		pt[i].pwt = p2e.pwt;
		pt[i].pcd = p2e.pcd;
		pt[i].pat = 0;
		pt[i].nx = p2e.nx;
		pt[i].accessed = p2e.accessed;
		pt[i].dirty = reinterpret_cast<PDE_2MB*>(p2e_ptr)->dirty;
		pt[i].global = reinterpret_cast<PDE_2MB*>(p2e_ptr)->global;
		pt[i].pfn = p2e.pfn + i;
	}

	pt[p1Index].rw = 1;
	pt[p1Index].accessed = 1;
	pt[p1Index].dirty = 1;

  auto p2eShadow = reinterpret_cast<PML4E*>(PDVa)[p2Index];
  auto* p2eShadowPtr = reinterpret_cast<PDE_2MB*>(&p2eShadow);

  p2eShadowPtr->page_size = 0;
  p2eShadowPtr->pat = 0;
  p2eShadow.present = 1;
  p2eShadow.rw = 1;
  p2eShadow.us = 1;
  p2eShadow.nx = 0;
  p2eShadow.pfn = PTPa.QuadPart >> 12;
    
  reinterpret_cast<PML4E*>(PDVa)[p2Index] = p2eShadow;

	invlpg(reinterpret_cast<void*>(NmiHandler));

	*reinterpret_cast<uint8_t*>(NmiHandler) = 0xCC;

	__writecr3(cr3);

	return 1;
}
