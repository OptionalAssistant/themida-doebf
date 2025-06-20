#pragma once

#include "../Optimization/Optimizer.h"

class EmulatorCPU;
class PE;


class deobf
{
private:
	EmulatorCPU* m_cpu;
	PE* m_pe;
	Optimizer* optimizer;

	void captureTrace(uintptr_t rva, BasicBlock* bb);
	void transverseBlock(uintptr_t rva, BasicBlock* bb);
	BasicBlock* handleBBIntersection(uintptr_t address);
public:
	deobf(EmulatorCPU* cpu_, PE* pe=0) : m_cpu(cpu_), m_pe(pe), optimizer(new Optimizer()) {}

	void run(uintptr_t);

};

