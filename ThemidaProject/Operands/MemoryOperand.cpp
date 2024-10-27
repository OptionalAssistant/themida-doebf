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

void MemoryOperand::setBsOp(const zasm::BitSize& bs)
{
    this->bsOp = bs;
}

void MemoryOperand::LinkOperand()
{
}

void MemoryOperand::destroy()
{
    base->destroy();
    index->destroy();
    displacement->destroy();
    scale->destroy();
}
