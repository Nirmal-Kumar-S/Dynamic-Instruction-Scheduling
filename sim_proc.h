#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <vector>
#include <algorithm>

using namespace std;

// Global Variables

int PC, Cycle, width, rob_size, iq_size;
bool Pipeline_empty;

// Global Structures

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

typedef struct RMT_Entry
{
	bool valid;
	int tag;
}RMT_Entry;

typedef struct ROB_Entry
{
	int dest,pc;
	bool rdy;
	void clear()
	{
		dest = pc = 0;
		rdy = false;
	}
}ROB_Entry;

typedef struct Instruction
{
	int pc, type, dst, src1, src2, src1_original, src2_original, count;
	bool src1_rdy, src2_rdy, src1_ROB, src2_ROB;
	int FE_start, FE_end, DE_start, DE_end, RN_start, RN_end, RR_start, RR_end, DI_start, DI_end;
	int IS_start, IS_end, EX_start, EX_end, WB_start, WB_end, RT_start, RT_end;

	void clear()
	{
		pc = type = dst = src1 = src2 = count = src1_original = src2_original = 0;
		src1_rdy = src2_rdy = src1_ROB = src2_ROB = false;
		FE_start = FE_end = DE_start = DE_end = RN_start = RN_end = RR_start = RR_end = DI_start = DI_end = IS_start = IS_end = EX_start = EX_end = WB_start = WB_end = RT_start = RT_end = 0;

	}

	bool operator < (const Instruction &temp) const
	{
		return (pc < temp.pc);
	}
}Instruction;

// Global Classes

class RMT_Class
{
public:
	int RMT_Size;
	RMT_Entry rmt[67];
};

class ROB_Class
{
public:
	int head, tail;
	int ROB_Size;
	ROB_Entry *rob = NULL;
	void Create_ROB()
	{
		ROB_Size = rob_size;
		rob = new ROB_Entry[ROB_Size];
		for (int i = 0; i < ROB_Size; i++)
        {
            rob[i].clear();
        }  
	}
};

class Register_Fixed
{
public:
	int size;
	bool empty;
	Instruction *reg = NULL;
	void create_Register()
	{
		size = width;
		reg = new Instruction[size];
		for (int i = 0; i < size; i++)
        {
            reg[i].clear();
        }  
	}
};

class Register_Dynamic
{
public:
	vector <Instruction> reg;
	void dec_count()
	{
		for (unsigned int i = 0; i < reg.size(); i++)
				reg[i].count--;
	}
};

// Global Objects

RMT_Class RMT;
ROB_Class ROB;
Register_Fixed DE, RN, RR, DI;
Register_Dynamic IQ, Exec_List, WB, RT;

#endif
