#pragma once
#include "BaseOperand.h"

#include <zasm/zasm.hpp>

class RegisterOperand;
class ConstantOperand;

class MemoryOperand : public BaseOperand
{
private:
	RegisterOperand* base;
	RegisterOperand* index;
	ConstantOperand* displacement;
	ConstantOperand* scale;

	uintptr_t memoryAddress;
public:
	RegisterOperand* getBase();
	RegisterOperand* getIndex();
	ConstantOperand* getDisplacement();
	ConstantOperand* getScale();

	void setBase(RegisterOperand* op);
	void setIndex(RegisterOperand* op);
	void setDisplacement(ConstantOperand* op);
	void setScale(ConstantOperand* op);
	void setMemoryAddress(uintptr_t memAddress);
	uintptr_t getMemoryAddress();

	virtual void LinkOperand()override;
	virtual void destroy()override;

	MemoryOperand(RegisterOperand* base, RegisterOperand* index,
		ConstantOperand* displacement, ConstantOperand* scale,
		OperandAction op_action,const zasm::Operand& operand)
		: base(base), index(index),scale(scale), displacement(displacement),
		BaseOperand(op_action,operand) {}

	MemoryOperand() = default;
};

