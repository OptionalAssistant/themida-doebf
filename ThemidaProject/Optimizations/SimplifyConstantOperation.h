#pragma once
#include "BaseOptimization.h"

class Instruction;

class SimplifyConstantOperation : public BaseOptimization 
{
	virtual bool run(Instruction* instruction)override;
};

