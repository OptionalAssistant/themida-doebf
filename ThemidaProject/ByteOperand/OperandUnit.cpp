#include <algorithm>
#include <stdexcept>

#include "OperandUnit.h"

BaseOperand* OperandUnit::getParent() const
{
    return parent;
}

void OperandUnit::setParent(BaseOperand* operand)
{
    parent = operand;
}

bool OperandUnit::hasUses() const
{
    return !use_list.empty();
}

std::vector<OperandUnit*>& OperandUnit::getUseList()
{
    return use_list;
}

OperandUnit* OperandUnit::getNext()const
{
    return next;
}

OperandUnit* OperandUnit::getPrev()const
{
    return prev;
}

void OperandUnit::setPrev(OperandUnit* opPrev) {
    prev = opPrev;
}

void OperandUnit::setNext(OperandUnit* opNext) {
    next = opNext;
}

void OperandUnit::addUse(OperandUnit* useOperand)
{
    use_list.push_back(useOperand);
}

void OperandUnit::ClearUseList()
{
    use_list.clear();
}

void OperandUnit::replaceOneUse(OperandUnit* oldUse, OperandUnit* newUse)
{
    this->deleteUse(oldUse);

    this->addUse(newUse);
}

void OperandUnit::replaceOperandWith(OperandUnit* op) {
    this->replaceAllUses(op);


    this->setPrev(op->getPrev());
    this->setNext(op->getNext());

    if (op->getPrev())
        op->getPrev()->setNext(this);

    if (op->getNext())
        op->getNext()->setPrev(this);

}

void OperandUnit::deleteUse(OperandUnit* operand) {
    auto& useList = this->getUseList();

    if (useList.empty())
        return;

    auto it = std::find(useList.begin(), useList.end(), operand);

    if (it == useList.end())
        throw std::runtime_error("Error during replaceOneUse");

    useList.erase(it);
}

void OperandUnit::transferAllUses(OperandUnit* operand)
{
    auto& useList = this->getUseList();

    for (auto& use : useList) {
        if (operand)
            operand->addUse(use);

        use->setPrev(operand);
    }

    useList.clear();
}

void OperandUnit::makeConstant() {


}

void OperandUnit::emplaceBetween()
{
    if (this->getNext())
        this->getNext()->setPrev(this->getPrev());

    if (this->getPrev())
        this->getPrev()->setNext(this->getNext());

}

bool OperandUnit::hasOneUse()
{
    return use_list.size() == 1;
}


void OperandUnit::replaceAllUses(OperandUnit* operand)
{
    use_list = operand->getUseList();

    for (auto& use : use_list) {
        use->setPrev(this);
    }

    operand->ClearUseList();
}