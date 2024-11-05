#include "RegisterOperand.h"
#include "MemoryOperand.h"

void RegisterOperand::LinkOperand()
{
   Instruction* previous = this->getParent()->getPrev();

   if (!previous || this->getZasmOperand().get<zasm::Reg>().getId() == zasm::Reg::Id::None)
	   return;

   do {
	 
	   for (auto& op : previous->getOperands()) {
		   
		    RegisterOperand* registerOperand = dynamic_cast<RegisterOperand*>(op);

		   if (registerOperand) {

			   if (this->isSameRegister(*registerOperand)) {
					
				   const OperandAction opAccess = op->getOperandAccess();

				   if (opAccess == OperandAction::WRITE) {
					   this->setPrev(op);
					   
					   if (this->getOperandAccess() == OperandAction::WRITE) {
						   op->setNext(this);
					   }
					   else if (this->getOperandAccess() == OperandAction::READ) {
						   op->addUse(this);
					   }

					   return;
				   }
			   }
			   
		   }

	   }

	   previous = previous->getPrev();

   } while (previous != nullptr);
}

void RegisterOperand::destroy()
{
	deleteAllUses();
	delete this;
}

bool RegisterOperand::isSameRegister( RegisterOperand& register_)
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
