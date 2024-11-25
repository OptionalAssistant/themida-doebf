#include "FlagsOperand.h"

#include "../ByteOperand/FlagBit.h"
#include "../Instruction/Instruction.h"

void FlagsOperand::Link()
{
	Instruction* prev = this->getParent()->getPrev();

	if (!prev)
		return;


	std::vector<FlagBit*> flagBites = this->getFlagBits();
	do
	{
		for (auto& op : prev->getOperands()) {
			FlagsOperand* flagsOperand = dynamic_cast<FlagsOperand*> (op);

			if (!flagsOperand)
				continue;

			std::vector<FlagBit*>& prevFlagBites = flagsOperand->getFlagBits();

			for (auto it = flagBites.begin(); it != flagBites.end(); /* no increment here */) {
					auto flagBit = *it;

					auto foundIt = std::find_if(prevFlagBites.begin(), prevFlagBites.end(),
						[&](FlagBit* prevFlagBit) {
							return prevFlagBit->getFlagMask() == flagBit->getFlagMask();
						});

					if (foundIt != prevFlagBites.end()) {
						FlagBit* foundBitFlag = *foundIt;

						flagBit->setPrev(foundBitFlag);

						if (flagBit->getFlagAction() == FlagAction::READ) {
							foundBitFlag->addUse(flagBit);
						}

						if (flagBit->getFlagAction() == FlagAction::WRITE) {
							foundBitFlag->setNext(flagBit);
						}

						// Erase `registerByte` from `registerBytes`
						it = flagBites.erase(it); // Returns the next valid iterator
					}
					else {
						++it; // Increment iterator if no deletion
					}
				}
			
		}

		prev = prev->getPrev();

	} while (prev != nullptr && !flagBites.empty());
}

void FlagsOperand::Unlink()
{
}

void FlagsOperand::destroy()
{
}

std::vector<FlagBit*>& FlagsOperand::getFlagBits()
{
    return flags;
}

void FlagsOperand::setFlagBits(std::vector<FlagBit*>& flagBits)
{
    this->flags = flagBits;
}

