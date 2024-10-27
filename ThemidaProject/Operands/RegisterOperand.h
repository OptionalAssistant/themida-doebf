#pragma once
#include <zasm/zasm.hpp>

#include "BaseOperand.h"


class RegisterOperand : public BaseOperand
{
private:
	zasm::Reg m_register;
public:
	virtual void LinkOperand()override;
	virtual void destroy()override;

	RegisterOperand(const zasm::Reg& reg) : m_register(reg) {}
};

