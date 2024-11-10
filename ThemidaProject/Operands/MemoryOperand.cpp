#include "MemoryOperand.h"
#include "RegisterOperand.h"
#include "ConstantOperand.h"

RegisterOperand* MemoryOperand::getBase()
{
    return base;
}

RegisterOperand* MemoryOperand::getIndex()
{
    return index;
}

ConstantOperand* MemoryOperand::getDisplacement()
{
    return displacement;
}

ConstantOperand* MemoryOperand::getScale()
{
    return scale;
}

void MemoryOperand::setBase(RegisterOperand* op){
    base = op;
}

void MemoryOperand::setIndex(RegisterOperand* op){
    index = op;
}

void MemoryOperand::setDisplacement(ConstantOperand* op){
    displacement = op;
}

void MemoryOperand::setScale(ConstantOperand* op){
    scale = op;
}

void MemoryOperand::setMemoryAddress(uintptr_t memAddress){
    this->memoryAddress = memAddress;
}

uintptr_t MemoryOperand::getMemoryAddress()
{
    return memoryAddress;
}

void MemoryOperand::LinkOperand()
{
    base->LinkOperand();
    index->LinkOperand();

    Instruction* previous = this->getParent()->getPrev();

    if (!previous)
        return;

    do {

        for (auto& op : previous->getOperands()) {
            
            MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(op);

            if (memoryOperand) {
                uintptr_t memoryAddress = memoryOperand->getMemoryAddress();

                if (memoryAddress == this->getMemoryAddress()) {

                    if (op->getOperandAccess() == OperandAction::WRITE ||
                        op->getOperandAccess() == OperandAction::READWRITE) {

                        this->setPrev(op);

                        if (this->getOperandAccess() == OperandAction::READ ||
                            this->getOperandAccess() == OperandAction::READWRITE) {
                            op->addUse(this);
                        }
                         if (this->getOperandAccess() == OperandAction::WRITE ||
                            this->getOperandAccess() == OperandAction::READWRITE) {
                            op->setNext(this);
                        }
                        return;

                    }
                }
            }
        }
     
       previous = previous->getPrev();
    
    } while (previous != nullptr);
}

void MemoryOperand::destroy()
{
    base->destroy();
    index->destroy();
    displacement->destroy();
    scale->destroy();
    BaseDestroy();
}
