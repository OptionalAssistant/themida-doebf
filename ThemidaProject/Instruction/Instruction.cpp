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

void BaseOperand::setOperand(const zasm::Operand& op)
{
	this->operand = op;
}

zasm::Operand BaseOperand::getZasmOperand()
{
	return this->operand;
}

void BaseOperand::setOperandAccess(zasm::detail::OperandAccess op_access)
{
	this->op_access = op_access;
}

zasm::detail::OperandAccess BaseOperand::getOperandAccess()
{
	return this->op_access;
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

void Instruction::addOperand( BaseOperand* op)
{
	operands.push_back(op);
}

std::vector<BaseOperand*>& Instruction::getOperands()
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

BaseOperand* Instruction::getOperand(uintptr_t index)
{
	return this->operands[index];
}


void Instruction::setOperand(uintptr_t index,  BaseOperand* op)
{
	this->operands[index] = op;
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
