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
public:
	deobf(EmulatorCPU* cpu_, PE* pe) : m_cpu(cpu_), m_pe(pe), optimizer(new Optimizer()) {}

	void run(uintptr_t);

};

