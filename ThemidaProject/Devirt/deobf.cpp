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



    optimizer->run(userData->instructions);

	logger->log("After\n");

	printOutInstructions(userData->instructions);
}
