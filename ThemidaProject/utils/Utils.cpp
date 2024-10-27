#include "Utils.h"


#include <fstream>
#include <zasm/formatter/formatter.hpp>


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

void printInstruction(const zasm::InstructionDetail& instruction) {
	printf("%s\n", formatInstruction(instruction).c_str());
}

std::string actionToString(const zasm::detail::OperandAccess& actionType) {
	
	switch (actionType) {
	case zasm::detail::OperandAccess::Read:
		return "Read";
	case  zasm::detail::OperandAccess::Write:
		return "Write";
	case  zasm::detail::OperandAccess::CondRead:
		return "CondRead";
	case  zasm::detail::OperandAccess::CondWrite:
		return "CondWrite";
	case  zasm::detail::OperandAccess::ReadWrite:
		return "ReadWrite";
	case  zasm::detail::OperandAccess::CondReadCondWrite:
		return "CondReadCondWrite";
	case  zasm::detail::OperandAccess::ReadCondWrite:
		return "ReadCondWrite";
	case  zasm::detail::OperandAccess::CondReadWrite:
		return "CondReadWrite";
	default:
		throw std::runtime_error("Error during access to string function");
		break;
	}
}

std::string formatBytes(const zasm::InstructionDetail& instruction,uintptr_t address) {
	std::string pr;
	for (int i = 0; i < instruction.getLength(); i++)
	{
		pr += std::format("0x{:x} ", *(BYTE*)(address + i));
		//printf("%s", pr.c_str());
	}
	return pr;
}

void EmulatorCPU::formatMemoryOperand(const zasm::Operand& op,uintptr_t i) {

		uintptr_t address = CalcEffectiveMemAddress(op,i);

		std::string action = actionToString(instruction.getOperandAccess(i));

		std::string actionType = std::format("{} :   ", action);
		logger->log(actionType);

		std::string memInfo = std::format("[ 0x{:x} ] =  ", address);
		logger->log(memInfo);
		//printf("%s", memInfo.c_str());

		if (instruction.getMnemonic() != zasm::x86::Mnemonic::Lea)
		{
			uintptr_t value;
			mem_read(address, &value,
				zasmBitsToNumericSize(instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)));
			std::string memContent = std::format("0x{:x}\n",
				value);
			logger->log(memContent);
			//printf("%s", memContent.c_str());

		}
}
void EmulatorCPU::LogInstruction(const zasm::InstructionDetail& instruction, uintptr_t address)
{


	auto string1 = std::format("{:x} ! ",count);
	logger->log(string1);

	logger->log(formatBytes(instruction,address));
		
	std::string string = std::format("|  {}\n",formatInstruction(instruction).c_str());
	logger->log(string);
//	printf("%s", string.c_str());

	for (int i = 0; i < instruction.getOperandCount(); i++)
	{
		const auto& op = instruction.getOperand(i);

		if (op.holds<zasm::Mem>())
		{
			formatMemoryOperand(op,i);
		}
	}
	

	PrintRegisters();
	
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



