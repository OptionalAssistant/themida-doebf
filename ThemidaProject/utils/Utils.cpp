#include "Utils.h"


#include <fstream>
#include <zasm/formatter/formatter.hpp>


#include "../emulator/emu.h"
#include "Logger.h"
#include "../Operands/BaseOperand.h"
#include "../Operands/MemoryOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/ConstantOperand.h"

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

std::string myActionToString(const OperandAction action) {

	switch (action)
	{
	case OperandAction::READ:
		return "READ";
	case OperandAction::WRITE:
		return "WRITE";
	case OperandAction::READWRITE:
		return "READWRITE";
	default:
		break;
	}
}

OperandAction zasmActionToOwn(const zasm::detail::OperandAccess& actionType)
{
	switch (actionType)
	{
	case zasm::detail::OperandAccess::Read:
		return OperandAction::READ;
	case zasm::detail::OperandAccess::Write:
		return OperandAction::WRITE;
	case zasm::detail::OperandAccess::ReadWrite:
		return OperandAction::WRITE;
	default:
		throw std::runtime_error("Failed to zasmActionToOwn.Unknown type");
		break;
	}
}

std::string formatBytes(const zasm::InstructionDetail& instruction,uintptr_t address) {
	std::string pr;
	for (int i = 0; i < instruction.getLength(); i++)
	{
		pr += std::format("{:x} ", *(BYTE*)(address + i));
	}
	return pr;
}

std::string FormatOperandUses(BaseOperand* op) {
	const auto& useList = op->getUseList();

	std::string result = std::format("USES: ");

	if (useList.empty())
		return result + "NONE ";

	for (auto& use : useList) {
		result += std::format("{:d} ", use->getParent()->getCount());
	}
	return result;
}

std::string formateOperand(BaseOperand* op) {
	
	MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(op);

	if (memoryOperand) {

		const auto& zasmInstruction = memoryOperand->getParent()->getZasmInstruction();
		std::string action = myActionToString(memoryOperand->getOperandAccess());
		std::string logMessage = std::format("{} :   [ 0x{:x} ]   ", action, memoryOperand->getMemoryAddress());

		if (memoryOperand->getOperandAccess() == OperandAction::WRITE) {
			logMessage += FormatOperandUses(op);
			if (!memoryOperand->getNext())
				logMessage += "NEXT WRITE: NONE ";
			else {
				logMessage += std::format(" NEXT WRITE: {:d} ",
					memoryOperand->getNext()->getParent()->getCount());
			}

			if (!memoryOperand->getPrev())
				logMessage += "PREV WRITE: NONE ";
			else {
				logMessage += std::format(" PREV WRITE: {:d} ",
					memoryOperand->getPrev()->getParent()->getCount());
			}

		}
		else if (memoryOperand->getOperandAccess() == OperandAction::READ) {
			if (!memoryOperand->getPrev())
				return logMessage += "ORIGIN: NONE ";
			else {
				logMessage += std::format("ORIGIN : {:d} ",
					memoryOperand->getPrev()->getParent()->getCount());
			}
		}
		return logMessage;
	}

	RegisterOperand* registerOpernad = dynamic_cast<RegisterOperand*>(op);

	if (registerOpernad) {
		std::string logMessage = formatZasmRegisterOperand(registerOpernad->getZasmOperand().get<zasm::Reg>());
		logMessage += std::format("  {} :", myActionToString(registerOpernad->getOperandAccess()));

		if (registerOpernad->getOperandAccess() == OperandAction::WRITE) {
			logMessage += FormatOperandUses(op);
			if (!registerOpernad->getNext())
				logMessage += "NEXT WRITE: NONE ";
			else {
				logMessage += std::format(" NEXT WRITE: {:d} ",
					registerOpernad->getNext()->getParent()->getCount());
			}

			if (!registerOpernad->getPrev())
				logMessage += "PREV WRITE: NONE ";
			else {
				logMessage += std::format(" PREV WRITE: {:d} ",
					registerOpernad->getPrev()->getParent()->getCount());
			}
		}
		 else if (registerOpernad->getOperandAccess() == OperandAction::READ) {
			if (!registerOpernad->getPrev())
				 logMessage +=  "ORIGIN: NONE ";
			else {
				logMessage += std::format("ORIGIN : {:d} ",
					registerOpernad->getPrev()->getParent()->getCount());
			}
		}
		return logMessage;
	}

	ConstantOperand* constantOperand = dynamic_cast<ConstantOperand*>(op);

	if (constantOperand) {
		std::string logMessage = std::format("CONST ");
		return logMessage;
	}
}

void formateLinkedInstructions(Instruction* instruction)
{
	for (Instruction* currentInstruction = instruction;
		currentInstruction != nullptr; currentInstruction = currentInstruction->getNext()) {

		std::string res =std::format("{:d} | ",currentInstruction->getCount())
			+ formatInstruction(currentInstruction->getZasmInstruction()) + "\n";
		
		const auto& Operands = currentInstruction->getOperands();

		for (int i = 0; i < Operands.size(); i++) {
			res += std::format("OP:{:d} ",i) + formateOperand(Operands[i]) + "\n";

		}
		printf("%s\n\n", res.c_str());
		logger->log(res + "\n");
	}

}

std::string EmulatorCPU::formatMemoryOperand(const zasm::Operand& op,uintptr_t i) {

	uintptr_t address = CalcEffectiveMemAddress(op, i);

	std::string action = actionToString(instruction.getOperandAccess(i));
	std::string logMessage = std::format("{} :   [ 0x{:x} ] =  ", action, address);

	if (instruction.getMnemonic() != zasm::x86::Mnemonic::Lea) {
		uintptr_t value;
		mem_read(address, &value,
			zasmBitsToNumericSize(instruction.getOperand(0).getBitSize(zasm::MachineMode::AMD64)));
		logMessage += std::format("0x{:x}\n", value);
	}

	return logMessage;
}
void EmulatorCPU::LogInstruction(const zasm::InstructionDetail& instruction, uintptr_t address)
{


	auto string1 = std::format("{:x} ! ",count);
	logger->log(string1);

	logger->log(formatBytes(instruction,address));
		
	std::string string = std::format("|  {}\n",formatInstruction(instruction).c_str());
	logger->log(string);

	for (int i = 0; i < instruction.getOperandCount(); i++)
	{
		const auto& op = instruction.getOperand(i);

		if (op.holds<zasm::Mem>())
		{
			logger->log(formatMemoryOperand(op,i));
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



