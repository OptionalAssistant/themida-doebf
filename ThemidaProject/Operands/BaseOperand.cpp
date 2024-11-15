#include <algorithm>
#include <stdexcept>


#include "BaseOperand.h"
#include "ConstantOperand.h"

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

void BaseOperand::ClearUseList()
{
    use_list.clear();
}

void BaseOperand::replaceOneUse(BaseOperand* oldUse, BaseOperand* newUse)
{
    this->deleteUse(oldUse);

    this->addUse(newUse);
}

void BaseOperand::replaceOperandWith(BaseOperand* op){
    this->replaceAllUses(op);


    this->setPrev(op->getPrev());
    this->setNext(op->getNext());

    if (op->getPrev())
        op->getPrev()->setNext(this);

    if (op->getNext())
        op->getNext()->setPrev(this);

}

void BaseOperand::deleteUse(BaseOperand* operand){
    auto& useList = this->getUseList();

    if (useList.empty())
        return;

    auto it = std::find(useList.begin(), useList.end(), operand);

    if (it == useList.end())
        throw std::runtime_error("Error during replaceOneUse");

    useList.erase(it);
}

void BaseOperand::transferAllUses(BaseOperand* operand)
{
    auto& useList = this->getUseList();

    for (auto& use : useList) {
        if(operand)
        operand->addUse(use);

        use->setPrev(operand);
    }

    useList.clear();
}

void BaseOperand::makeConstant(ConstantOperand* constantOp){

    Instruction* parent = this->getParent();
    
    parent->replaceOperand(this, constantOp);
    
}

void BaseOperand::emplaceBetween()
{
    if (this->getNext())
        this->getNext()->setPrev(this->getPrev());

    if (this->getPrev())
        this->getPrev()->setNext(this->getNext());

}

bool BaseOperand::hasOneUse()
{
    return use_list.size() == 1;
}

bool BaseOperand::hasOneUser()
{
    if (hasOneUse())
        return true;

    Instruction* firstUser = use_list[0]->getParent();
    for (auto& use : use_list) {
        Instruction* user = use->getParent();

        if (user != firstUser)
            return false;
    }

    return true;
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

void BaseOperand::replaceAllUses(BaseOperand* operand)
{
    use_list = operand->getUseList();
    
    for (auto& use : use_list) {
        use->setPrev(this);
    }

    operand->ClearUseList();
}


