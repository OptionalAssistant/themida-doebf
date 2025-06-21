#pragma once

#include "../Instruction/Instruction.h"

class BaseOptimization;
struct BasicBlock;

class Optimizer
{
private:
	std::list<BaseOptimization*>passes;
	std::list<BaseOptimization*>mbaPasses;
	void runMbaPasses(BasicBlock* bb);
	bool runOtherPasses(BasicBlock* bb);
	void removeRedudantJumps(BasicBlock* bb);
public:
	Optimizer();
	void run(BasicBlock* bb);
	void runHelper(BasicBlock* bb, std::unordered_set<BasicBlock*>& visited);
};