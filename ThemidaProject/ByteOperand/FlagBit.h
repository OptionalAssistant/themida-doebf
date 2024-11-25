#pragma once

#include "OperandUnit.h"


enum class FlagAction {
	READ = 0,
	WRITE
};
class FlagBit : public OperandUnit
{
private:
	WORD flagMask;
	FlagAction flagAction;
public:
	WORD getFlagMask();
	void setFlagMask(WORD flagMask);

	FlagAction getFlagAction();
	void setFlagAction(FlagAction flagAction);


	FlagBit(WORD flagMask, FlagAction flagAction,BaseOperand* parent = nullptr) :
		flagMask(flagMask), flagAction(flagAction), OperandUnit(parent) {}
};

