#include <ntifs.h>
#include <ntstatus.h>

#include "global.h"
#include "hook.h"

NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING*) {
	usermodeName = "usermode.exe";
	
	if(!doHooking())
		return STATUS_UNSUCCESSFUL;
  
	return STATUS_SUCCESS;
}
