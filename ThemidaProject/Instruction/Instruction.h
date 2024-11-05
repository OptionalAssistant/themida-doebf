#pragma once
#include <vector>

#include <zasm/zasm.hpp>

class BaseOperand;

class Instruction
{
private:
	std::vector<BaseOperand*>operand_list;
	Instruction* prev;
	Instruction* next;
	zasm::InstructionDetail instruction;
	uintptr_t count;
public:
	std::vector<BaseOperand*>& getOperands();
	BaseOperand* getOperand(uintptr_t index);
	void Unlink();
	void DeleteFromList();
	void Delete();
	void DecreaseCount();

	Instruction* getNext()const;
	Instruction* getPrev()const;
	
	Instruction(zasm::InstructionDetail instruction) : prev(nullptr), next(nullptr),
		instruction(instruction),count(0) {}

	 uintptr_t getCount();
	 void setCount(uintptr_t count);

	 void LinkInstruction();

	 void setPrev(Instruction* instruction);
	 void setNext( Instruction* instruction);

	 Instruction* insertAfter( Instruction* instruction);
	 Instruction* insertBefore( Instruction* instruction);

	 void addOperand(BaseOperand* baseOperand);

	 zasm::InstructionDetail getZasmInstruction();
};

