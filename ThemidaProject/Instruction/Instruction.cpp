#include <Windows.h>

#include "Instruction.h"

uintptr_t MemoryOperand::getMemoryAddress()
{
	return this->memoryAddress;
}

void MemoryOperand::setMemoryAddress(uintptr_t memAddress)
{
	this->memoryAddress = memAddress;
}

void Operand::setOperand(const zasm::Operand& op)
{
	this->operand = op;
}

zasm::Operand Operand::getZasmOperand()
{
	return this->operand;
}

void Instruction::setCount(uintptr_t count)
{
	this->count = count;
}

uintptr_t Instruction::getCount()
{
	return this->count;
}

void Instruction::setAddress(uintptr_t address)
{
	this->address = address;
}

uintptr_t Instruction::getAddress()
{
	return address;
}

void Instruction::addOperand(const OperandVariant& op)
{
	operands.push_back(op);
}

std::vector<OperandVariant>& Instruction::getOperands()
{
	return operands;
}

zasm::InstructionDetail& Instruction::getZasmInstruction()
{
	return this->instruction;
}

void Instruction::setZasmInstruction(zasm::InstructionDetail& instruction)
{
	this->instruction = instruction;
}

OperandVariant& Instruction::getOperand(uintptr_t index)
{
	return operands[index];
}

void Instruction::setOperand(uintptr_t index,const OperandVariant& op)
{
	operands[index] = op;
}

std::array<uintptr_t, 17>& Instruction::getRegistersArray()
{
	return registerValues;
}

void Instruction::setRegisterValues(std::array<uintptr_t, 17> registers)
{
	this->registerValues = registers;
}

WORD& Instruction::getRflags()
{
	return rFlags;
}

void Instruction::setRFlags(WORD flags)
{
	this->rFlags = rFlags;
}
