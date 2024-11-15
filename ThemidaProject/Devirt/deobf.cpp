#include <Windows.h>
#include <vector>

#include <zasm/formatter/formatter.hpp>

#include "deobf.h"
#include "../emulator/emu.h"
#include "../Linker/Linker.h"
#include "../utils/Utils.h"
#include "../Instruction/Instruction.h"
#include "../utils/Logger.h"


void deobf::run(uintptr_t rva)
{
	UserData userData;
	
	m_cpu->addCallback(traceCallback, &userData);

	m_cpu->run(rva);

	for (Instruction* current = userData.head; current != nullptr; current = current->getNext()) {
		current->LinkInstruction();
	}

	formateLinkedInstructions(userData.head);
	
	bool isContinue;
	do
	{
	  isContinue = optimizer->run(userData.head);

	} while (isContinue);

	logger->log("After\n");

	formateLinkedInstructions(userData.head);

}
