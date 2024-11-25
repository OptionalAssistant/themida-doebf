#pragma once
#include "BaseOperand.h"

class ConstantOperand : public BaseOperand
{
public:
	virtual void Link() override;
	virtual void Unlink() override;
	virtual void destroy() override;

	ConstantOperand(const zasm::Operand& operand, uintptr_t index)
		: BaseOperand(OperandAction::READ, operand, index) {}


};

