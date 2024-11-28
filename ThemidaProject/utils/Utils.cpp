#include "Utils.h"


#include <fstream>
#include <zasm/formatter/formatter.hpp>

#include "../Operands/BaseOperand.h"
#include "../Operands/MemoryOperand.h"
#include "../Operands/RegisterOperand.h"
#include "../Operands/FlagsOperand.h"
#include "../Operands/ConstantOperand.h"

#include "../ByteOperand/OperandUnit.h"
#include "../ByteOperand/MemoryByte.h"
#include "../ByteOperand/RegisterByte.h"
#include "../ByteOperand/FlagBit.h"

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

OperandAction zasmActionToOwn(const zasm::detail::OperandAccess& actionType)
{
	switch (actionType)
	{
	case zasm::detail::OperandAccess::Read:
	case zasm::detail::OperandAccess::None:
		return OperandAction::READ;
	case zasm::detail::OperandAccess::Write:
		return OperandAction::WRITE;
	case zasm::detail::OperandAccess::ReadWrite:
		return OperandAction::READWRITE;
	default:
		throw std::runtime_error("Failed to zasmActionToOwn.Unknown type");
		break;
	}
}

std::string FormatOperandUnitUses(OperandUnit* operandUnit) {
	const auto& useList = operandUnit->getUseList();

	std::string result = std::format("USES: ");

	if (useList.empty())
		return result + "NONE ";

	for (auto& use : useList) {
		result += std::format("{:d} ", use->getParent()->getParent()->getCount());
	}
	return result;
}

std::string formatRelations(OperandUnit* operandUnit) {
	std::string logMessage;
	
	BaseOperand* parentOperand = operandUnit->getParent();

	if (parentOperand->getOperandAccess() == OperandAction::READWRITE ||
		parentOperand->getOperandAccess() == OperandAction::WRITE) {
		
		if (operandUnit->getNext()) {
			logMessage += std::format("NEXT WRITE: {:d} ",
				operandUnit->getNext()->getParent()->getParent()->getCount());
		}
		else {
			logMessage += "NEXT WRITE: NONE ";
		}

		if (operandUnit->getPrev()) {
			logMessage += std::format("PREV WRITE: {:d} ",
				operandUnit->getPrev()->getParent()->getParent()->getCount());
		}
		else {
			logMessage += "PREV WRITE: NONE ";
		}

		logMessage += FormatOperandUnitUses(operandUnit);
	}
	else {
		if (operandUnit->getPrev()) {
			logMessage += std::format("ORIGIN: {:d} ",
				operandUnit->getPrev()->getParent()->getParent()->getCount());
		}
		else {
			logMessage += "ORIGIN : NONE ";
		}
	}

	return logMessage;
}
std::string formatRegisterOperand(RegisterOperand* registerOpernad) {
	std::string logMessage = formatZasmRegisterOperand(registerOpernad->getZasmOperand().get<zasm::Reg>());
	logMessage += std::format("  {} :\n", myActionToString(registerOpernad->getOperandAccess()));

	for (auto& operandUnit : registerOpernad->getOperandUnits()) {
		RegisterByte* registerByte = dynamic_cast<RegisterByte*>(operandUnit);
		
		logMessage += std::format("[{:d}] : {} \n", registerByte->getIndex(),
			formatRelations(registerByte));
	}

	return logMessage;
}

std::string formatMemoryOp(MemoryOperand* memoryOp) {
	std::string logMessage;

	if (memoryOp->getBase()->getZasmOperand().get<zasm::Reg>().getId() != zasm::Reg::Id::None)
		logMessage += "BASE: " + formatRegisterOperand(memoryOp->getBase());
	if(memoryOp->getIndex()->getZasmOperand().get<zasm::Reg>().getId() != zasm::Reg::Id::None)
		logMessage += "INDEX: " + formatRegisterOperand(memoryOp->getIndex());

	const auto& zasmInstruction = memoryOp->getParent()->getZasmInstruction();
	std::string action = myActionToString(memoryOp->getOperandAccess());
	logMessage += std::format("{} \n", action, memoryOp->getMemoryAddress());

	const int32_t opSize = memoryOp->getZasmOperand().get<zasm::Mem>().getByteSize();
	
	for (auto& operandUnit : memoryOp->getOperandUnits()) {
		MemoryByte* memoryByte = dynamic_cast<MemoryByte*>(operandUnit);

		logMessage += std::format("[ 0x{:x} ] : {} \n",memoryByte->getMemoryAddress(), 
			formatRelations(memoryByte));
	}

	return logMessage;
}

std::string formatFlagsOperand(FlagsOperand* flagOperand) {
	
	std::string res;
	
	for (auto& operandUnit : flagOperand->getOperandUnits()) {
		FlagBit* flag = dynamic_cast<FlagBit*>(operandUnit);
		res += std::format("{} : {} \n", formatFlagMask(flag->getFlagMask()) ,
			formatRelations(flag));
	}

	return res;
}

std::string formateOperand(BaseOperand* op) {


	ConstantOperand* constantOperand = dynamic_cast<ConstantOperand*>(op);
	std::string logMessage;
	if (constantOperand) {
		logMessage = std::format("CONST ");
		return logMessage;
	}

	MemoryOperand* memoryOperand = dynamic_cast<MemoryOperand*>(op);

	if (memoryOperand) {
		return formatMemoryOp(memoryOperand);
	}

	RegisterOperand* registerOpernad = dynamic_cast<RegisterOperand*>(op);

	if (registerOpernad) {
		return formatRegisterOperand(registerOpernad);
	}

	FlagsOperand* flagOperand = dynamic_cast<FlagsOperand*>(op);

	if (flagOperand) {
		return formatFlagsOperand(flagOperand);
	}
}

std::string formatInstruction(Instruction* instruction) {
	std::string res = std::format("{:d} | ", instruction->getCount())
		+ formatInstruction(instruction->getZasmInstruction()) + "\n";


	const auto& Operands = instruction->getOperands();

	for (int i = 0; i < Operands.size(); i++) {
		res += std::format("OP:{:d} ", i) + formateOperand(Operands[i]) + "\n";

	}

	return res;
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

std::string formatFlagMask(WORD mask)
{
	switch (mask)
	{
	case carryFlagMask:
		return "CF";
	case parityFlagMask:
		return "PF";
	case auxiliaryCarryFlagMask:
		return "AF";
	case zeroFlagMask:
		return "ZF";
	case signFlagMask:
		return "SF";
	case overflowFlagMask:
		return "OF";
	default:
		throw std::runtime_error("Error during formatting flag mask");
		break;
	}
}

void printLinkedInstruction(Instruction* instruction){
	for (Instruction* currentInstruction = instruction;
		currentInstruction != nullptr;
		currentInstruction = currentInstruction->getNext()) {
		logger->log(formatInstruction(currentInstruction) + "\n");
	}

}
