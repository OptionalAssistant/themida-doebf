#include <algorithm>

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

void BaseOperand::BaseDestroy()
{
    BaseOperand* previous = this->getPrev();

    
    if (previous && (this->getOperandAccess() == OperandAction::READ ||
        this->getOperandAccess() == OperandAction::READWRITE)) {
        auto& previousUseList = previous->getUseList();

        auto foundUse = std::find(previousUseList.begin(), previousUseList.end(), this);

        if (foundUse != previousUseList.end())
            previousUseList.erase(foundUse);
        else
            printf("Error");
     
    }
    /*if (this->getOperandAccess() == OperandAction::WRITE ||
        this->getOperandAccess() == OperandAction::READWRITE)
    {
         if(previous)
        previous->setNext(this->getNext());

        this->ClearUseList();

        if(this->getNext())
        this->getNext()->setPrev(previous);

    }*/
    delete this;
}

void BaseOperand::DeleteAllUses()
{
}

void BaseOperand::ClearUseList()
{
    use_list.clear();
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

    operand->ClearUseList();
}

void BaseOperand::replaceNextPrev(BaseOperand* operand)
{
    next = operand->getNext();
    prev = operand->getPrev();
}

void BaseOperand::deleteAllUses()
{
    for (auto& use : use_list) {
        use->setPrev(nullptr);
    }

    use_list.clear();
}
