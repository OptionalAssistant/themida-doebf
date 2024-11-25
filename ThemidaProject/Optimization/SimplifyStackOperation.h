#pragma once
#include "BaseOptimization.h"

class SimplifyStackOperation : public BaseOptimization
{
public:
	virtual bool run(Instruction* instruction)override;
};

