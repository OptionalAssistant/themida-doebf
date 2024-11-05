#include "BaseOperand.h"

Instruction* BaseOperand::getParent() const
{
    return parent;
}

void BaseOperand::setParent(Instruction* instruction)
{
    parent = instruction;
}

bool BaseOperand::hasUses() const
{
    return !use_list.empty();
}

std::vector<BaseOperand*>& BaseOperand::getUseList()
{
    return use_list;
}

BaseOperand* BaseOperand::getNext()const
{
    return next;
}

BaseOperand* BaseOperand::getPrev()const
{
    return prev;
}

void BaseOperand::setPrev(BaseOperand* opPrev){
    prev = opPrev;
}

void BaseOperand::setNext(BaseOperand* opNext){
    next = opNext;
}

void BaseOperand::addUse(BaseOperand* useOperand)
{
    use_list.push_back(useOperand);
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

void BaseOperand::replaceAllUses(BaseOperand* operand)
{
    use_list = operand->getUseList();
    
    for (auto& use : use_list) {
        use->setPrev(this);
    }

    operand->deleteAllUses();
}

void BaseOperand::replaceNextPrev(BaseOperand* operand)
{
    next = operand->getNext();
    prev = operand->getPrev();
}

void BaseOperand::deleteAllUses()
{
    use_list.clear();
}
