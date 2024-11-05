#pragma once
#include <zasm/zasm.hpp>

#include "BaseOperand.h"


class RegisterOperand : public BaseOperand
{
public:
	virtual void LinkOperand()override;
	virtual void destroy()override;
	
	bool isSameRegister( RegisterOperand& register_);

	RegisterOperand(const zasm::Operand& operand,
		OperandAction op_action) 
		: BaseOperand(op_action,operand) {}

	
};

