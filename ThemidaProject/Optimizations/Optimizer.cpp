#include "Optimizer.h"
#include "DeadCodeElimination.h"

Optimizer::Optimizer()
{
	passes.push_back(new DeadCodeElimination());
}

bool Optimizer::run(Instruction* instruction)
{
	bool isExtraPass = false;
	for (auto& pass : passes) {
		isExtraPass |= pass->run(instruction);
	}
	return isExtraPass;
}
