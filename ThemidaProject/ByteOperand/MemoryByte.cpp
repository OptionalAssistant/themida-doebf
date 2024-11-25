#include "MemoryByte.h"

uintptr_t MemoryByte::getMemoryAddress()
{
	return memoryAddress;
}

void MemoryByte::setMemoryAddress(uintptr_t memoryAddress)
{
	this->memoryAddress = memoryAddress;
}
