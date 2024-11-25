#pragma once
#include <vector>

class BaseOptimization;
class Instruction;

class Optimizer
{
private:
	std::vector<BaseOptimization*>passes;
public:
	Optimizer();
	bool run(Instruction* instruction);
};