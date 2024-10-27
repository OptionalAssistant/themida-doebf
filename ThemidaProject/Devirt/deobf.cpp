#include <Windows.h>
#include <vector>

#include <zasm/formatter/formatter.hpp>

#include "deobf.h"
#include "../emulator/emu.h"
#include "../Linker/Linker.h"

void deobf::run(uintptr_t rva)
{
	UserData userData;
	
	m_cpu->addCallback(traceCallback, &userData);

	m_cpu->run(rva);

}
