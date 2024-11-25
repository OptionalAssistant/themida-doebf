#pragma once
#include <Windows.h>
#include <vector>

#include "BaseOperand.h"

class RegisterByte;

class RegisterOperand : public BaseOperand
{
private:
	std::vector<RegisterByte*>registerBytes;
public:
	virtual void Link() override;
	virtual void Unlink() override;
	virtual void destroy() override;

	RegisterOperand(const zasm::Operand& operand, uintptr_t index,
		OperandAction op_action)
		: BaseOperand(op_action, operand, index) {}

	std::vector<RegisterByte*>& getRegisterBytes();

	void setRegisterByte(std::vector<RegisterByte*>& registerBytes);
	bool isSameRegister(RegisterOperand& register_);
};

