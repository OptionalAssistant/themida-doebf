#include "Optimizer.h"
#include "SimplifyStackOperation.h"
#include "SimplifyConstantOperation.h"
#include "SimplifyDCEPass.h"
#include "../utils/Utils.h"

Optimizer::Optimizer()
{
	passes.push_back(new SimplifyStackOperation());
	passes.push_back(new SimplifyConstantOperation());
	passes.push_back(new SimplifyDCEPass());
}

bool Optimizer::run(Instruction* instruction)
{
	bool isExtraPass = false;
	for (auto& pass : passes) {
		isExtraPass |= pass->run(instruction);
	}
	return isExtraPass;
}
