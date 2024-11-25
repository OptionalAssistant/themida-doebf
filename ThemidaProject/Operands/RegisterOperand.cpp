#include <algorithm>

#include "RegisterOperand.h"

#include "../ByteOperand/OperandUnit.h"
#include "../ByteOperand/RegisterByte.h"

#include "../Instruction/Instruction.h"

void RegisterOperand::Link()
{
	Instruction* prev = this->getParent()->getPrev();

	if (!prev || this->getZasmOperand().get<zasm::Reg>().getId() == zasm::Reg::Id::None)
		return;

	std::vector<RegisterByte*> registerBytes = this->getRegisterBytes();
	do
	{
		for (auto& op : prev->getOperands()) {
			RegisterOperand* registerOperand = dynamic_cast<RegisterOperand*>(op);

			if (!registerOperand)
				continue;

			if (!this->isSameRegister(*registerOperand))
				continue;

			const OperandAction opAccess = registerOperand->getOperandAccess();

			if (opAccess == OperandAction::WRITE ||
				opAccess == OperandAction::READWRITE) {
				std::vector<RegisterByte*>& prevRegisterBytes = registerOperand->getRegisterBytes();

				for (auto it = registerBytes.begin(); it != registerBytes.end(); /* no increment here */) {
					auto registerByte = *it;

					auto foundIt = std::find_if(prevRegisterBytes.begin(), prevRegisterBytes.end(),
						[&](RegisterByte* regByte) {
							return regByte->getIndex() == registerByte->getIndex();
						});

					if (foundIt != prevRegisterBytes.end()) {
						RegisterByte* foundByteRegister = *foundIt;

						registerByte->setPrev(foundByteRegister);

						if (this->getOperandAccess() == OperandAction::READ ||
							this->getOperandAccess() == OperandAction::READWRITE) {
							foundByteRegister->addUse(registerByte);
						}

						if (this->getOperandAccess() == OperandAction::WRITE ||
							this->getOperandAccess() == OperandAction::READWRITE) {
							foundByteRegister->setNext(registerByte);
						}

						// Erase `registerByte` from `registerBytes`
						it = registerBytes.erase(it); // Returns the next valid iterator
					}
					else {
						++it; // Increment iterator if no deletion
					}
				}
			}
		}

		prev = prev->getPrev();

	} while(prev != nullptr && !registerBytes.empty());
}


void RegisterOperand::Unlink()
{
}


void RegisterOperand::destroy()
{
}

std::vector<RegisterByte*>& RegisterOperand::getRegisterBytes()
{
	return registerBytes;
}

void RegisterOperand::setRegisterByte(std::vector<RegisterByte*>& registerBytes)
{
	this->registerBytes = registerBytes;
}


bool RegisterOperand::isSameRegister(RegisterOperand& register_)
{
	zasm::Reg registerValue = register_.getZasmOperand().get<zasm::Reg>();

	if (!registerValue.isGp())
		return false;

	auto& gpRegisterCompared = registerValue.as<zasm::x86::Gp>();

	auto& gpRegister = this->getZasmOperand().get<zasm::Reg>().as<zasm::x86::Gp>();

	return gpRegisterCompared == gpRegister.r8() ||
		gpRegisterCompared == gpRegister.r8hi() ||
		gpRegisterCompared == gpRegister.r8lo() ||
		gpRegisterCompared == gpRegister.r16() ||
		gpRegisterCompared == gpRegister.r32() ||
		gpRegisterCompared == gpRegister.r64();
}

