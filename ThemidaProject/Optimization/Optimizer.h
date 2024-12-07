#pragma once
#include <list>
#include <zasm/zasm.hpp>

#include "../Instruction/Instruction.h"

class BaseOptimization;


class Optimizer
{
private:
	std::list<BaseOptimization*>passes;
	std::list<BaseOptimization*>mbaPasses;
	void runMbaPasses(std::list<Instruction>& instructions);
	bool runOtherPasses(std::list<Instruction>& instructions);
public:
	Optimizer();
	bool run(std::list<Instruction>& instructions);
};