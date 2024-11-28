#pragma once
#include <Windows.h>
#include <vector>

#include "BaseOperand.h"

class MemoryByte;
class RegisterOperand;
class ConstantOperand;


class MemoryOperand : public BaseOperand
{
private:
	RegisterOperand* base;
	RegisterOperand* index;
	ConstantOperand* scale;
	ConstantOperand* displacement;

	uintptr_t memoryAddress;
public:
	virtual void Link() override;
	virtual void Unlink() override;
	virtual void destroy() override;


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

	void setMemoryBytes(std::vector<MemoryByte*>& memoryBytes);
	
	MemoryOperand(RegisterOperand* base, RegisterOperand* index,
		ConstantOperand* displacement, ConstantOperand* scale,
		OperandAction op_action, const zasm::Operand& operand, uintptr_t index_op)
		: base(base), index(index), scale(scale), displacement(displacement),
		BaseOperand(op_action, operand, index_op) {}

	MemoryOperand() = default;

};

