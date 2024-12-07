#pragma once
#include "BaseOptimization.h"

#include "../Instruction/Instruction.h"

class SimplifyDeadcodeElimination : public BaseOptimization
{
public:
	virtual bool run(std::list<Instruction>::iterator it, std::list<Instruction>& instructions);
};

