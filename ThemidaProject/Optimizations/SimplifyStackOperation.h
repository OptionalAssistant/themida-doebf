#pragma once
#include "BaseOptimization.h"

class Instruction;

class SimplifyStackOperation : public BaseOptimization  
{
	virtual bool run(Instruction* instruction)override;
};

