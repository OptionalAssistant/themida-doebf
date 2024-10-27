#pragma once

class EmulatorCPU;
class PE;

class deobf
{
private:
	EmulatorCPU* m_cpu;
	PE* m_pe;
public:
	deobf(EmulatorCPU* cpu_, PE* pe) : m_cpu(cpu_), m_pe(pe) {}

	void run(uintptr_t);

};

