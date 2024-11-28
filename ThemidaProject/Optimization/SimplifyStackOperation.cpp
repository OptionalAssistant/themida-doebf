#include "SimplifyStackOperation.h"

#include <Windows.h>
#include <iostream>

#include "../Instruction/Instruction.h"
#include "../Operands/BaseOperand.h"
#include "../utils/Logger.h"
#include "../utils/Utils.h"

bool SimplifyStackOperation::run(Instruction* instruction)
{
	bool isExtraPass = false;
	for (Instruction* currentInstruction = instruction; currentInstruction != nullptr;
		currentInstruction = currentInstruction->getNext())
	{
		//simplify push pop to mov
		if (currentInstruction->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Push &&
			currentInstruction->getNext()->getZasmInstruction().getMnemonic() == zasm::x86::Mnemonic::Pop) {

			printf("Simplifying push pop to move instruction count:%d\n", currentInstruction->getCount());

			Instruction* nextInstruction = currentInstruction->getNext();

			zasm::InstructionDetail::OperandsAccess opAccess;
			zasm::InstructionDetail::OperandsVisibility opVisibility;

			opAccess.set(0, zasm::Operand::Access::Write);
			opAccess.set(1, zasm::Operand::Access::Read);

			opVisibility.set(0, zasm::Operand::Visibility::Explicit);
			opVisibility.set(1, zasm::Operand::Visibility::Explicit);

			std::array<zasm::Operand, 10>ops;
			ops[0] = nextInstruction->getOperand(0)->getZasmOperand();
			ops[1] = currentInstruction->getOperand(0)->getZasmOperand();

			const auto& newZasmInstruction = zasm::InstructionDetail({},
				zasm::x86::Mnemonic::Mov, 2,
				ops, opAccess, opVisibility, {}, {});

			Instruction* newInstruction = new Instruction(newZasmInstruction);

			auto* dbg = nextInstruction->getOperand(0);
			newInstruction->addOperand(currentInstruction->getOperand(0));
			newInstruction->addOperand(nextInstruction->getOperand(0));

			currentInstruction->deleteOperand(currentInstruction->getOperand(0));
			nextInstruction->deleteOperand(nextInstruction->getOperand(0));

			newInstruction->insertAfter(nextInstruction);

			currentInstruction->Delete();
			nextInstruction->Delete();

			currentInstruction = newInstruction;

			
		}


	}

	return false;
}
