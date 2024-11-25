#pragma once
#include <vector>

#include "BaseOperand.h"

class FlagBit;

class FlagsOperand : public BaseOperand
{
private:
	std::vector<FlagBit*>flags;
public:
	virtual void Link() override;
	virtual void Unlink() override;
	virtual void destroy() override;


	FlagsOperand(const zasm::Operand& operand, uintptr_t index,
		OperandAction op_action)
		: BaseOperand(op_action, operand, index) {}


	std::vector<FlagBit*>& getFlagBits();

	void setFlagBits(std::vector<FlagBit*>& flagBits);

};

