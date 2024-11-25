#include "Optimizer.h"
#include "BaseOptimization.h"

#include "SimplifyConstantOperation.h"
#include "SimplifyStackOperation.h"

Optimizer::Optimizer()
{
	passes.push_back(new SimplifyConstantOperation());
	passes.push_back(new SimplifyStackOperation());
}

bool Optimizer::run(Instruction* instruction)
{
	bool isExtraPass = false;
	for (auto& pass : passes) {
		isExtraPass |= pass->run(instruction);
	}
	return isExtraPass;
}