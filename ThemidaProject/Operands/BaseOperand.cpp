#include "BaseOperand.h"

#include "../ByteOperand/OperandUnit.h"
#include "../Operands/FlagsOperand.h"
#include "../ByteOperand/FlagBit.h"

Instruction* BaseOperand::getParent() const
{
    return parent;
}

void BaseOperand::setParent(Instruction* instruction)
{
    parent = instruction;
}

std::vector<OperandUnit*>& BaseOperand::getOperandUnits()
{
    return operandUnits;
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

BaseOperand::~BaseOperand()
{
    for (auto op : operandUnits) {
        delete op;
    }
}

void BaseOperand::Unlink()
{
     OperandAction opAccess = this->getOperandAccess();

    FlagsOperand* flagsOperand = dynamic_cast<FlagsOperand*>(this);
    
    if (flagsOperand) {
        auto& opUnits = flagsOperand->getOperandUnits();

        for (auto opUnit : opUnits) {
            FlagBit* flagBit = dynamic_cast<FlagBit*>(opUnit);

            FlagAction opAction = flagBit->getFlagAction();

            if (opAction == FlagAction::READ) {
                if(flagBit->getPrev())
                flagBit->getPrev()->deleteUse(flagBit);
            }
            else if (opAction == FlagAction::WRITE) {
                for (auto& use : flagBit->getUseList()) {
                    use->setPrev(flagBit->getPrev());
                    if (flagBit->getPrev()){
                        flagBit->getPrev()->addUse(use);
                        flagBit->getPrev()->setNext(flagBit->getNext());
                    }
                    if (flagBit->getNext()) {
                        flagBit->getNext()->setPrev(flagBit->getPrev());
                    }
                }
            }
        }
    }
    else {
        if (opAccess == OperandAction::READ ||
            opAccess == OperandAction::READWRITE) {

            for (auto& operandUnit : this->getOperandUnits()) {
                if(operandUnit->getPrev())
                operandUnit->getPrev()->deleteUse(operandUnit);
            }
        }

        if (opAccess == OperandAction::WRITE ||
            opAccess == OperandAction::READWRITE) {

            for (auto& operandUnit : this->getOperandUnits()) {
                for (auto& use : operandUnit->getUseList()) {
                    use->setPrev(operandUnit->getPrev());
                    if (operandUnit->getPrev()) {
                        operandUnit->getPrev()->addUse(use);
                        operandUnit->getPrev()->setNext(operandUnit->getNext());
                    }
                    if (operandUnit->getNext()) {
                        operandUnit->getNext()->setPrev(operandUnit->getPrev());
                    }

                }
            }
            
        }
    }
    delete this;
}

bool BaseOperand::hasUses()
{
    for (auto& operandUnit : operandUnits) {
        if (operandUnit->hasUses())
            return true;
    }
    return false;
}

zasm::Operand BaseOperand::getZasmOperand()
{
    return operand;
}