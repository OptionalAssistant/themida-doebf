#include "Instruction.h"
#include "../Operands/BaseOperand.h"

std::vector<BaseOperand*> Instruction::getOperands() const
{
	return operand_list;
}

void Instruction::destroy()
{
	for (auto& op : operand_list) {
		op->destroy();
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

	return instruction;
}

Instruction* Instruction::insertBefore( Instruction* instruction)
{
	instruction->setNext(this);
	
	instruction->setPrev(this->getPrev());
	
	this->getPrev()->setNext(this);

	return instruction;
}

void Instruction::addOperand(BaseOperand* baseOperand){
	operand_list.push_back(baseOperand);
}
