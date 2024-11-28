#include <Windows.h>
#include <vector>

#include <zasm/formatter/formatter.hpp>

#include "deobf.h"
#include "../emulator/emu.h"
#include "../utils/Utils.h"
#include "../Instruction/Instruction.h"
#include "../utils/Logger.h"

#include "../callbacks/callbacks.h"

void deobf::run(uintptr_t rva)
{
	UserData* userData = new UserData();
	
	m_cpu->addCallback(traceCallback, userData);

	m_cpu->run(0x1000);


	for (Instruction* currentInstruction = userData->head;
		currentInstruction != nullptr;
		currentInstruction = currentInstruction->getNext()) {
		currentInstruction->LinkInstruction();
	}


	printLinkedInstruction(userData->head);

	bool isContinue;
	do
	{
		isContinue = optimizer->run(userData->head);

	} while (isContinue);

	logger->log("After\n");
	printLinkedInstruction(userData->head);

}
