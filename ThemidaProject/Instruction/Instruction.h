#pragma once
#include <vector>
#include <Windows.h>
#include <zasm/zasm.hpp>
#include "../utils/types.h"

class Operand {
private:
	zasm::Operand operand;
public:
	void setOperand(const zasm::Operand& op);
	zasm::Operand getZasmOperand();

	Operand(const zasm::Operand& op) : operand(op) {}

};

class MemoryOperand : public Operand {
	uintptr_t memoryAddress;
public:
	uintptr_t getMemoryAddress();
	void setMemoryAddress(uintptr_t memAddress);

	MemoryOperand(const zasm::Operand& op, uintptr_t memAddress = 0) 
		: Operand(op), memoryAddress(memAddress) {}

};

class Instruction
{
private:
	zasm::InstructionDetail instruction;
	std::vector<OperandVariant>operands;
	uintptr_t count;
	uintptr_t address;
	std::array<uintptr_t, 17> registerValues;
	WORD rFlags;
public:
	void setCount(uintptr_t count);
	uintptr_t getCount();
	void setAddress(uintptr_t address);
	uintptr_t getAddress();

	void addOperand(const OperandVariant& op);

	std::vector<OperandVariant>& getOperands();

	zasm::InstructionDetail& getZasmInstruction();
	void setZasmInstruction(zasm::InstructionDetail& instruction);

	OperandVariant& getOperand(uintptr_t index);

	void setOperand(uintptr_t index, const OperandVariant& op);

	std::array<uintptr_t, 17>& getRegistersArray();
	void setRegisterValues(std::array<uintptr_t, 17> registers);

	WORD& getRflags();
	void setRFlags(WORD flags);
};

