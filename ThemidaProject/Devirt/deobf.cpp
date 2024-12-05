#include <Windows.h>
#include <vector>

#include <zasm/formatter/formatter.hpp>

#include "deobf.h"
#include "../emulator/emu.h"
#include "../utils/Utils.h"
#include "../utils/Logger.h"

#include "../callbacks/callbacks.h"

void deobf::run(uintptr_t rva)
{
	UserData* userData = new UserData();
	
	m_cpu->addCallback(traceCallback, userData);

	m_cpu->run(rva);

	bool isContinue;
	do
	{
		isContinue = optimizer->run(userData->instructions);

	} while (isContinue);

	logger->log("After\n");

	for (auto& instruction : userData->instructions) {
		std::string toLog = std::format("Trying to emulate instruction at rva:0x{:x} count : {:d} | {} --\n",
			instruction.getAddress(),
			instruction.getCount(), formatInstruction(instruction.getZasmInstruction()));
		logger->log(toLog);
	}
}
