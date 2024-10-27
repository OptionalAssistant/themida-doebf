#pragma once
#include "BaseOperand.h"

class ConstantOperand  : public BaseOperand
{
public:
	virtual void LinkOperand()override;
	virtual void destroy()override;
};

