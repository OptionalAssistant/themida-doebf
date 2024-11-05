#include "Instruction.h"
#include "../Operands/BaseOperand.h"

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
	DecreaseCount();
}

void Instruction::Delete()
{
	Unlink();
	DeleteFromList();
	delete this;
}

void Instruction::DecreaseCount()
{
	for (Instruction* currentInstruction = this;
		currentInstruction != nullptr;
		currentInstruction = currentInstruction->getNext()) {
		currentInstruction->setCount(currentInstruction->getCount() - 1);
	}
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

Instruction* Instruction::insertAfter(Instruction* instruction)
{
	next = instruction;
	instruction->setPrev(this);

	instruction->setCount(this->getCount() + 1);

	return instruction;
}

Instruction* Instruction::insertBefore( Instruction* instruction)
{
	instruction->setNext(this);
	
	instruction->setPrev(this->getPrev());
	
	this->setPrev(instruction);

	instruction->setCount(this->getCount() - 1);

	return instruction;
}

void Instruction::addOperand(BaseOperand* baseOperand){
	baseOperand->setParent(this);
	operand_list.push_back(baseOperand);
}

zasm::InstructionDetail Instruction::getZasmInstruction()
{
	return instruction;
}

