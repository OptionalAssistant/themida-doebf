#pragma once
#include <Windows.h>
#include <array>
#include <vector>
#include <functional>

#include <zasm/zasm.hpp>

class EmulatorCPU;


class EmulatorCPU
{
private:
	using callbackFunction = bool(*)(EmulatorCPU* cpu_, uintptr_t address, 
		zasm::InstructionDetail curInstr, void* userData);

	struct callbackStruct {
		void* callback_data;
		callbackFunction callback_fn;
	};
	struct MemoryRegion {
		uintptr_t address;
		std::vector<BYTE> memory;
		uintptr_t size;
	};

	std::vector<MemoryRegion>memory_regions;

	WORD rFlags{};
	std::array<uintptr_t, 17>regs{0};
	
	uintptr_t eip{};
	

	std::vector<BYTE>data{};

	zasm::Decoder* decoder;
	
	bool isStop;
	bool isStopBefore;
	bool isStopAfter;

	std::vector<callbackStruct>callbacks;

	zasm::InstructionDetail instruction;
	
	uintptr_t count;
	void PushValue(uintptr_t value);
	uintptr_t PopValue();

	void HandlePush();
	void HandleCall();
	void HandleMov();
	void HandlePop();
	void HandleLea();
	void HandleNeg();
	void HandleRol();
	void HandleJccJump();
	void HandleAdd();
	void HandleShl();
	void HandleAdc();
	void HandleSetge();
	void HandleMovzx();
	void HandleSub();
	void HandleRor();
	void HandleXadd();
	void HandleXor();
	void HandleSbb();
	void HandleMovsx();
	void HandleAnd();
	void HandleNot();
	void HandleBswap();
	void HandleDec();
	void HandleInc();
	void HandleRet();
	void HandlerOr();
	void HandleCmovb();
	void HandleSetz();
	void HandleExchange();
	void HandleSetnz();
	void HandleSetp();
	void HandleSetnle();
	void HandleSetnb();
	void HandleSetbe();
	void HandleCmp();
	void HandleSetle();
	void HandleSetnbe();
	void HandleCmovnz();
	void HandleSets();
	void HandleSetb();
	void HandleCmovl();
	void HandleSetno();
	void HandleCld();
	void HandleMovsb();
	void HandleSetns();
	void HandleCmovns();
	void HandleCmovnb();
	void HandleCmovp();
	void HandleSetl();
	void HandleCmovbe();
	void HandleCmpxchg();
	void HandleSetnp();
	void HandleSeto();
	void HandleCmovnp();
	void HandleCmovno();
	void HandleTest();
	void HandleCmovz();
	void HandleCmovs();
	void HandleCmovnl();
	void HandleCmovnbe();
	void HandleCmovle();
	void HandleCmovnle();
	void HandleCmovo();
	void HandleCdqe();
	void HandleCbw();
	void HandleCwde();
	void HandleCwd();
	void HandleBtc();
	void HandleBts();
	void HandleBtr();
	void HandleCqo();
	void HandleBt();
	void HandleCdq();
	void HandleIMul();
	void HandleIDiv();

	void SetCarryFlag();
	void SetParityFlag();
	void SetAuxiliaryCarryFlag();
	void SetZeroFlag();
	void SetSignFlag();
	void SetOverflowFlag();

	void ClearCarryFlag();
	void ClearParityFlag();
	void ClearAuxiliaryCarryFlag();
	void ClearZeroFlag();
	void ClearSignFlag();
	void ClearOverflowFlag();
	void ClearDirectionFlag();

	BYTE GetCarryFlag();
	BYTE GetParityFlag();
	BYTE GetAuxiliaryFlag();
	BYTE GetZeroFlag();
	BYTE GetSignFlag();
	BYTE GetOverflowFlag();

	
	bool CF;
	bool PF;
	bool AF;
	bool ZF;
	bool SF;
	bool OF;

	enum BinaryOp
	{
		ADD = 0,
		SUB,
		NONE
	};
	uintptr_t GetValue(const zasm::Operand& op);

	void WriteResult(const zasm::Operand& op, uintptr_t value);

	void PrintFlags();



	void update_eflags(uintptr_t dst,uintptr_t dst_old, uintptr_t src1, uintptr_t src2,
		zasm::BitSize bs, BinaryOp op);
	//void PrintRegisters();
public:
	enum Registers {
		RAX,
		RBX,
		RCX,
		RDX,
		RBP,
		RSP,
		RSI,
		RDI,
		R8,
		R9,
		R10,
		R11,
		R12,
		R13,
		R14,
		R15,

		GS,
		REGISTER_MAX_VALUE
	};
	inline static constexpr uintptr_t baseImage = 0x0000000140000000;
	void run(uintptr_t eip);

	EmulatorCPU();
	void reg_write( zasm::Reg reg,uintptr_t value);

	uintptr_t reg_read( zasm::Reg reg);

	void setEip(uintptr_t eip);
	uintptr_t getEip();

	std::array<uintptr_t, 17> getRegistersValues();

	void addCallback(callbackFunction callback, void* userData = nullptr);
	void removeCallback(callbackFunction callbackDelete);

	void stop_emu();
	void stop_emu_after();
	void stop_emu_before();

	uintptr_t CalcMemAddress(const zasm::Mem& mem);
	uintptr_t CalcEffectiveMemAddress(const zasm::Operand& op, uintptr_t i);

	bool mem_map(uintptr_t address, uintptr_t size);
	bool mem_unmap(uintptr_t address);
	uintptr_t from_virtual_to_real(uintptr_t rva);
	uintptr_t mem_read(uintptr_t address, void* bytes, uintptr_t size);
	void mem_write(uintptr_t address, const void* bytes, uintptr_t  size);

};



uintptr_t reg_read_(std::array<uintptr_t, 17>& regs,zasm::Reg reg, WORD rFlag);

void reg_write_(std::array<uintptr_t, 17>& regs,zasm::Reg reg, uintptr_t value, WORD& rFlags);