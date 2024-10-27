#pragma once
#include "../Instruction/Instruction.h"
#include <vector>

class BaseOperand
{
private:
	Instruction* parent;
	std::vector<BaseOperand*>use_list;
	BaseOperand* next;
	BaseOperand* prev;
public:
	Instruction* getParent()const;
	bool hasUses()const;

	virtual BaseOperand* getNext()const;
	virtual BaseOperand* getPrev()const;

	virtual void LinkOperand() = 0;

	virtual void destroy() = 0;
	
};

