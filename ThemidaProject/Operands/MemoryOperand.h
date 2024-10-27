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
	zasm::BitSize bsOp;
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
	void setBsOp(const zasm::BitSize& bs);

	virtual void LinkOperand()override;
	virtual void destroy()override;
};

