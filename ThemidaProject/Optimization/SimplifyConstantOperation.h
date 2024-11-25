#pragma once
#include "BaseOptimization.h"

class SimplifyConstantOperation : public BaseOptimization 
{
public:
	virtual bool run(Instruction* instruction)override;
};

