#pragma once
#include <vector>
#include <Windows.h>
#include <zasm/zasm.hpp>
#include <list>
inline uintptr_t bbCount = 0;
class BaseOperand {
private:
	zasm::Operand operand;
	zasm::detail::OperandAccess op_access;
public:
	void setOperand(const zasm::Operand& op);
	zasm::Operand getZasmOperand();

	virtual ~BaseOperand() = default;

	BaseOperand(const zasm::Operand& op) : operand(op) {}
	void setOperandAccess(zasm::detail::OperandAccess op_access);
	zasm::detail::OperandAccess getOperandAccess();

};

class MemoryOperand : public BaseOperand {
	uintptr_t memoryAddress;
public:
	uintptr_t getMemoryAddress();
	void setMemoryAddress(uintptr_t memAddress);

	MemoryOperand(const zasm::Operand& op, uintptr_t memAddress = 0) 
		: BaseOperand(op), memoryAddress(memAddress) {}

};

class Instruction
{
private:
	zasm::InstructionDetail instruction;
	std::vector<BaseOperand*>operands;
	uintptr_t count;
	uintptr_t address;
	std::array<uintptr_t, 17> registerValues;
	WORD rFlags;
public:
	void setCount(uintptr_t count);
	uintptr_t getCount();
	void setAddress(uintptr_t address);
	uintptr_t getAddress();

	void addOperand( BaseOperand* op);

	std::vector<BaseOperand*>& getOperands();

	zasm::InstructionDetail& getZasmInstruction();
	void setZasmInstruction(zasm::InstructionDetail& instruction);

	BaseOperand* getOperand(uintptr_t index);

	void setOperand(uintptr_t index,  BaseOperand* op);

	std::array<uintptr_t, 17>& getRegistersArray();
	void setRegisterValues(std::array<uintptr_t, 17> registers);

	WORD& getRflags();
	void setRFlags(WORD flags);
};


struct BasicBlock {
	uintptr_t count;
	BasicBlock* pass1;
	BasicBlock* pass2;

	BasicBlock() : pass1(nullptr), pass2(nullptr),count(bbCount++) {}

	std::list<Instruction>instructions;
};
