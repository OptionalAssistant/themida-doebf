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


	std::vector<MemoryByte*> memoryBytes = this->getMemoryBytes();
	do
	{
		for (auto& op : prev->getOperands()) {
			MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(op);

			if (!memoryOperand)
				continue;

			const OperandAction opAccess = memoryOperand->getOperandAccess();

			if (opAccess == OperandAction::WRITE ||
				opAccess == OperandAction::READWRITE) {
				std::vector<MemoryByte*>& prevMemoryBytes = memoryOperand->getMemoryBytes();

				for (auto it = memoryBytes.begin(); it != memoryBytes.end(); /* no increment here */) {
					auto memoryByte = *it;

					auto foundIt = std::find_if(prevMemoryBytes.begin(), prevMemoryBytes.end(),
						[&](MemoryByte* memBytePrev) {
							return memBytePrev->getMemoryAddress() == memoryByte->getMemoryAddress();
						});

					if (foundIt != prevMemoryBytes.end()) {
						MemoryByte* foundByteRegister = *foundIt;

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
}

void MemoryOperand::destroy()
{
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

std::vector<MemoryByte*>& MemoryOperand::getMemoryBytes()
{
    return memoryBytes;
}

void MemoryOperand::setMemoryBytes(std::vector<MemoryByte*>& memoryBytes)
{
    this->memoryBytes = memoryBytes;
}







