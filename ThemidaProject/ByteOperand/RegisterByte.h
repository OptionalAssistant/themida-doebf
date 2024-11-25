#pragma once
#include "OperandUnit.h"

class RegisterByte : public OperandUnit
{
private:
	BYTE registerIndex;
public:
	BYTE getIndex();
	void setIndex(BYTE index);

	RegisterByte(BYTE registerIndex,
		BaseOperand* parent = nullptr) 
		: registerIndex(registerIndex),OperandUnit(parent){ }
};

