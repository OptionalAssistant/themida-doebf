#include "Utils.h"


#include <fstream>
#include <zasm/formatter/formatter.hpp>

#include "../Instruction/Instruction.h"
#include "../emulator/emu.h"
#include "Logger.h"


bool ReadFile(const std::string& path, std::vector<BYTE>& bin)
{
	if (!GetFileAttributesA(path.c_str()))
	{
		printf("File does not exist\n");
		return false;
	}

	std::ifstream File(path.c_str(), std::ios::binary | std::ios::ate);

	if (File.fail())
	{
		printf("Opening the file failed: 0x%X\n", (DWORD)File.rdstate());
		return false;
	}

	auto  FileSize = File.tellg();

	if (FileSize < 0x1000)
	{
		printf("Filesize is invalid.\n");
		File.close();
		return false;
	}
	bin.resize(FileSize);


	File.seekg(0, std::ios::beg);
	File.read((char*)bin.data(), FileSize);
	File.close();
	return true;
}


std::string formatInstruction(const zasm::InstructionDetail& instruction) {
	const auto& instr = instruction.getInstruction();

	return  zasm::formatter::toString(&instr, zasm::formatter::Options::HexImmediates |
		zasm::formatter::Options::HexOffsets);
}

std::string formatZasmRegisterOperand(const zasm::Reg& op) {

	return  zasm::formatter::toString(op, zasm::formatter::Options::HexImmediates |
		zasm::formatter::Options::HexOffsets);
}

void printInstruction(const zasm::InstructionDetail& instruction) {
	printf("%s\n", formatInstruction(instruction).c_str());
}


std::string formatBytes(const zasm::InstructionDetail& instruction,uintptr_t address) {
	std::string pr;
	for (int i = 0; i < instruction.getLength(); i++)
	{
		pr += std::format("{:x} ", *(BYTE*)(address + i));
	}
	return pr;
}

Instruction zasmToInstruction(zasm::InstructionDetail& instruction)
{
	Instruction newInstruction;
	for (int i = 0; i < instruction.getOperandCount(); i++) {
		auto& op = instruction.getOperand(i);

		if (op.holds<zasm::Mem>()) {
			newInstruction.addOperand(MemoryOperand(op));
		}
		else {
			newInstruction.addOperand(Operand(op));
		}
	}

	newInstruction.setZasmInstruction(instruction);

	return newInstruction;
}

EmulatorCPU::Registers zasmToEmulatorRegister(const ZydisRegister_& reg)
{
	switch (reg)
	{
	case ZYDIS_REGISTER_RAX:
		return EmulatorCPU::Registers::RAX;
	case ZYDIS_REGISTER_RBX:
		return EmulatorCPU::Registers::RBX;
	case ZYDIS_REGISTER_RCX:
		return EmulatorCPU::Registers::RCX;
	case ZYDIS_REGISTER_RDX:
		return EmulatorCPU::Registers::RDX;
	case ZYDIS_REGISTER_RBP:
		return EmulatorCPU::Registers::RBP;
	case ZYDIS_REGISTER_RSP:
		return EmulatorCPU::Registers::RSP;
	case ZYDIS_REGISTER_RSI:
		return EmulatorCPU::Registers::RSI;
	case ZYDIS_REGISTER_RDI:
		return EmulatorCPU::Registers::RDI;
	case ZYDIS_REGISTER_R8:
		return EmulatorCPU::Registers::R8;
	case ZYDIS_REGISTER_R9:
		return EmulatorCPU::Registers::R9;
	case ZYDIS_REGISTER_R10:
		return EmulatorCPU::Registers::R10;
	case ZYDIS_REGISTER_R11:
		return EmulatorCPU::Registers::R11;
	case ZYDIS_REGISTER_R12:
		return EmulatorCPU::Registers::R12;
	case ZYDIS_REGISTER_R13:
		return EmulatorCPU::Registers::R13;
	case ZYDIS_REGISTER_R14:
		return EmulatorCPU::Registers::R14;
	case ZYDIS_REGISTER_R15:
		return EmulatorCPU::Registers::R15;
	default:
		printf("unknown register conversion\n");
		exit(0);
		break;
	}
}

BYTE GetSignBit(uintptr_t value, zasm::BitSize mm)
{
	switch (mm)
	{
	case zasm::BitSize::_8:
		return (value & 0x80) >> 7;
	case zasm::BitSize::_16:
		return (value & 0x8000) >> 15;
	case zasm::BitSize::_32:
		return (value & 0x80000000) >> 31;
	case zasm::BitSize::_64:
		return (value & 0x8000000000000000) >> 63;
	default:
		printf("wtf?");
		break;
	}
}
void SetSignBit(uintptr_t value, zasm::BitSize mm)
{
	switch (mm)
	{
	case zasm::BitSize::_8:
		 value |= 0x80;
		 break;
	case zasm::BitSize::_16:
		value |= 0x8000;
		 break;
	case zasm::BitSize::_32:
		value |= 0x80000000;
		 break;
	case zasm::BitSize::_64:
		value |= 0x8000000000000000;
		 break;
	default:
		printf("wtf?");
		break;
	}
}
void ClearSignBit(uintptr_t value, zasm::BitSize mm)
{
	switch (mm)
	{
	case zasm::BitSize::_8:
		value |= 0x7F;
		break;
	case zasm::BitSize::_16:
		value |= 0x7FFF;
		break;
	case zasm::BitSize::_32:
		value |= 0x7FFFFFFF;
		break;
	case zasm::BitSize::_64:
		value |= 0x7FFFFFFFFFFFFFFF;
		break;
	default:
		printf("wtf?");
		break;
	}
}
BYTE GetSecondMSB(uintptr_t value, zasm::BitSize mm)
{
	switch (mm)
	{
	case zasm::BitSize::_8:
		return (value & 0x40) >> 6;
	case zasm::BitSize::_16:
		return (value & 0x4000) >> 14;
	case zasm::BitSize::_32:
		return (value & 0x40000000) >> 30;
	case zasm::BitSize::_64:
		return (value & 0x4000000000000000) >> 62;
	default:
		printf("wtf?");
		break;
	}
}

BYTE LSB(uintptr_t value)
{
	return value & 1;
}

uintptr_t zasmBitsToNumericSize(zasm::BitSize bs)
{
	switch (bs)
	{
	case zasm::BitSize::_8:
		return 1;
	case zasm::BitSize::_16:
		return 2;
	case zasm::BitSize::_32:
		return 4;
	case zasm::BitSize::_64:
		return 8;
	default:
		throw std::runtime_error("Invalid bs.Unable to convert zasmBitsToNumericSize");
	}
}