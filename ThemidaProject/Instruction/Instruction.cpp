#include "Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../utils/Utils.h"

std::vector<BaseOperand*>& Instruction::getOperands() 
{
	return operand_list;
}

BaseOperand* Instruction::getOperand(uintptr_t index)
{
	return operand_list[index];
}

void Instruction::Unlink()
{
	for (auto& op : operand_list) {
		op->destroy();
	}
}

void Instruction::DeleteFromList()
{
	this->getNext()->setPrev(prev);
	this->getPrev()->setNext(next);
}

void Instruction::Delete()
{
	DeleteFromList();
	Unlink();
	delete this;
}


Instruction* Instruction::getNext() const
{
	return next;
}

Instruction* Instruction::getPrev() const
{
	return prev;
}

uintptr_t Instruction::getCount()
{
	return count;
}

void Instruction::setCount(uintptr_t count)
{
	this->count = count;
}

void Instruction::LinkInstruction()
{
	for (auto& op: operand_list) {
		op->LinkOperand();
	}
}

void Instruction::setPrev(Instruction* instruction)
{
	prev = instruction;
}

void Instruction::setNext( Instruction* instruction)
{
	next = instruction;
}

Instruction* Instruction::insertAfter(Instruction* previous)
{
	printf("Insert after instructions: %s\n", formatInstruction(previous->getZasmInstruction()).c_str());
	this->setPrev(previous);

	this->setNext(previous->getNext());

	if(previous->getNext())
	previous->getNext()->setPrev(this);


	previous->setNext(this);


	this->setCount(previous->getCount() + 1);

	return this;
}

Instruction* Instruction::insertBefore( Instruction* next)
{
	this->setPrev(next->getPrev());
	this->setNext(next);
	next->setPrev(this);

	setCount(next->getCount());

	return this;
}

void Instruction::addOperand(BaseOperand* baseOperand){
	baseOperand->setParent(this);
	operand_list.push_back(baseOperand);
}

zasm::InstructionDetail Instruction::getZasmInstruction()
{
	return instruction;
}

