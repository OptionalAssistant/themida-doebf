#pragma once
#include <Windows.h>
#include <vector>

#include "OperandUnit.h"

class MemoryByte : public OperandUnit
{
private:
	uintptr_t memoryAddress;
public:
	uintptr_t getMemoryAddress();
	void setMemoryAddress(uintptr_t memoryAddress);

	MemoryByte(uintptr_t memoryAddress,
		BaseOperand* parent = nullptr) 
		: memoryAddress(memoryAddress), OperandUnit(parent) {}
};

