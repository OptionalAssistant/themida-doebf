#include "BaseOperand.h"


Instruction* BaseOperand::getParent() const
{
    return parent;
}

void BaseOperand::setParent(Instruction* instruction)
{
    parent = instruction;
}

uintptr_t BaseOperand::getIndex()
{
    return index;
}

void BaseOperand::setIndex(uintptr_t index)
{
    this->index = index;
}

OperandAction BaseOperand::getOperandAccess()
{
    return op_action;
}

void BaseOperand::setOperandAction(OperandAction action)
{
    op_action = action;
}

zasm::Operand BaseOperand::getZasmOperand()
{
    return operand;
}