#pragma once
#include <list>
#include <zasm/zasm.hpp>

#include "../Instruction/Instruction.h"

class BaseOptimization
{
public:
	virtual bool run(std::list<Instruction>& instructions) = 0;
};