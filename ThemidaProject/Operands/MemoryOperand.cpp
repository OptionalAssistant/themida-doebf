#include "MemoryOperand.h"
#include "RegisterOperand.h"

#include "../ByteOperand/MemoryByte.h"

#include "../Instruction/Instruction.h"

void MemoryOperand::Link()
{
	if (this->getParent()->getCount() == 985)
		printf("");

    base->Link();
    index->Link();

    Instruction* prev = this->getParent()->getPrev();

    if (!prev)
        return;


	std::vector<OperandUnit*> memoryBytes = this->getOperandUnits();
	do
	{
		for (auto& op : prev->getOperands()) {
			MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(op);

			if (!memoryOperand)
				continue;

			const OperandAction opAccess = memoryOperand->getOperandAccess();

			if (opAccess == OperandAction::WRITE ||
				opAccess == OperandAction::READWRITE) {
				std::vector<OperandUnit*>& prevMemoryBytes = memoryOperand->getOperandUnits();

				for (auto it = memoryBytes.begin(); it != memoryBytes.end(); /* no increment here */) {
					auto memoryByte = dynamic_cast<MemoryByte*>(*it);

					auto foundIt = std::find_if(prevMemoryBytes.begin(), prevMemoryBytes.end(),
						[&](OperandUnit* operandUnit) {
							MemoryByte* memBytePrev = dynamic_cast<MemoryByte*>(operandUnit);
							return memBytePrev->getMemoryAddress() == memoryByte->getMemoryAddress();
						});

					if (foundIt != prevMemoryBytes.end()) {
						MemoryByte* foundByteRegister = dynamic_cast<MemoryByte*>(*foundIt);

						memoryByte->setPrev(foundByteRegister);

						if (this->getOperandAccess() == OperandAction::READ ||
							this->getOperandAccess() == OperandAction::READWRITE) {
							foundByteRegister->addUse(memoryByte);
						}

						if (this->getOperandAccess() == OperandAction::WRITE ||
							this->getOperandAccess() == OperandAction::READWRITE) {
							foundByteRegister->setNext(memoryByte);
						}

						// Erase `registerByte` from `registerBytes`
						it = memoryBytes.erase(it); // Returns the next valid iterator
					}
					else {
						++it; // Increment iterator if no deletion
					}
				}
			}
		}

		prev = prev->getPrev();

	} while (prev != nullptr && !memoryBytes.empty());
}


void MemoryOperand::Unlink()
{
	if (base->getZasmOperand().get<zasm::Reg>().getId() != zasm::Reg::Id::None)
		base->Unlink();
	if (index->getZasmOperand().get<zasm::Reg>().getId() != zasm::Reg::Id::None)
		index->Unlink();

	BaseOperand::Unlink();
}

void MemoryOperand::destroy()
{
	Unlink();
}

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

void MemoryOperand::setBase(RegisterOperand* op) {
    base = op;
}

void MemoryOperand::setIndex(RegisterOperand* op) {
    index = op;
}

void MemoryOperand::setDisplacement(ConstantOperand* op) {
    displacement = op;
}

void MemoryOperand::setScale(ConstantOperand* op) {
    scale = op;
}

void MemoryOperand::setMemoryAddress(uintptr_t memAddress) {
    this->memoryAddress = memAddress;
}

uintptr_t MemoryOperand::getMemoryAddress()
{
    return memoryAddress;
}







