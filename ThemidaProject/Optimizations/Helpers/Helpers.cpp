#include "Helpers.h"

#include "../../Instruction/Instruction.h"
#include "../../Operands/BaseOperand.h"
#include "../../Operands/RegisterOperand.h"
#include "../../Operands/MemoryOperand.h"

BaseOperand* findPrevAccessRegister(Instruction* currentInstruction,  RegisterOperand* lookedRegister){


	while (currentInstruction != nullptr) {

		for (auto& op : currentInstruction->getOperands()) {

			RegisterOperand* registerOp = dynamic_cast<RegisterOperand*>(op);

			if (registerOp) {
				if (registerOp->isSameRegister(*lookedRegister))
					return registerOp;

				continue;
			}

			MemoryOperand* memoryOp = dynamic_cast<MemoryOperand*>(op);

			if (memoryOp) {
				if (memoryOp->getBase()->isSameRegister(*lookedRegister))
					return memoryOp->getBase();

				if (memoryOp->getIndex()->isSameRegister(*lookedRegister))
					return memoryOp->getIndex();
			}

		}

		currentInstruction = currentInstruction->getPrev();
	}


	return nullptr;
}

BaseOperand* findWriteRegisterInRange(Instruction* start, Instruction* end,RegisterOperand* registerOp_)
{
	while (start != end) {

		for (auto& op : start->getOperands()) {

			RegisterOperand* registerOp = dynamic_cast<RegisterOperand*>(op);

			if (registerOp) {
				if (registerOp->isSameRegister(*registerOp_) &&
					(registerOp->getOperandAccess() == OperandAction::WRITE ||
					registerOp->getOperandAccess() == OperandAction::READWRITE)) {
					return registerOp;
				}
			}

		}
		start = start->getNext();
	}

	return nullptr;
}
