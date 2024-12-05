#pragma once
#include <list>
#include <zasm/zasm.hpp>

#include "../Instruction/Instruction.h"

class BaseOptimization;


class Optimizer
{
private:
	std::list<BaseOptimization*>passes;
public:
	Optimizer();
	bool run(std::list<Instruction>& instructions);
};