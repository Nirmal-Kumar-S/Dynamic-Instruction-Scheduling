#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <vector>
#include <algorithm>
#include "sim_proc.h"

using namespace std;

// Functions

bool Advance_Cycle(FILE *FP)
{
	if (Pipeline_empty == true && feof(FP)) 
		return false;
	else
		return true;
}

void Fetch(FILE *FP)
{
	int op_type, dest, src1, src2;   // Variables are read from trace file
    unsigned long int pc;            // Variable holds the pc read from input file
	int ctr = 0;
	int i = 0;

	if (feof(FP) || DE.empty == false)
		return;

	if (DE.empty)
	{
		while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
		{
			DE.reg[i].pc = PC;					// Reading the instructions and storing it in DE bundle
			DE.reg[i].type = op_type;
			DE.reg[i].dst = dest;
			DE.reg[i].src1 = src1;
			DE.reg[i].src1_original = src1;
			DE.reg[i].src2 = src2;
			DE.reg[i].src2_original = src2;
			DE.reg[i].src1_rdy = DE.reg[i].src2_rdy = DE.reg[i].src1_ROB = DE.reg[i].src2_ROB = false;
			DE.reg[i].FE_start = Cycle;
			DE.reg[i].FE_end = 1;
			DE.reg[i].DE_start = Cycle + 1;
			
			if (DE.reg[i].type == 0)
				DE.reg[i].count = 1;
			else if (DE.reg[i].type == 1)
				DE.reg[i].count = 2;
			else if (DE.reg[i].type == 2)
				DE.reg[i].count = 5;

			PC++;
			ctr++;
			i++;
			DE.size = i;
			if (i == ((unsigned)width))
				break;
		}
		if (ctr != 0)
			DE.empty = false;
	}
}

void Decode()
{
	if (!DE.empty)
	{
		if (RN.empty)
		{
			for (unsigned int i = 0; i < DE.size; i++)
			{
				DE.reg[i].RN_start = Cycle + 1;				// Updating the Cycles
				DE.reg[i].DE_end = DE.reg[i].RN_start - DE.reg[i].DE_start;
			}
			for (unsigned int i = 0; i < DE.size; i++)
			{
				RN.reg[i].pc=DE.reg[i].pc;					// Reading the instructions from DE and storing it in RN bundle
				RN.reg[i].type=DE.reg[i].type;
				RN.reg[i].dst=DE.reg[i].dst;
				RN.reg[i].src1=DE.reg[i].src1;
				RN.reg[i].src2=DE.reg[i].src2;
				RN.reg[i].src1_original=DE.reg[i].src1_original;
				RN.reg[i].src2_original=DE.reg[i].src2_original;
				RN.reg[i].count=DE.reg[i].count;
				RN.reg[i].src1_rdy=DE.reg[i].src1_rdy;
				RN.reg[i].src2_rdy=DE.reg[i].src2_rdy;
				RN.reg[i].src1_ROB=DE.reg[i].src1_ROB;
				RN.reg[i].src2_ROB=DE.reg[i].src2_ROB;
				RN.reg[i].FE_start=DE.reg[i].FE_start;
				RN.reg[i].FE_end=DE.reg[i].FE_end;
				RN.reg[i].DE_start=DE.reg[i].DE_start;
				RN.reg[i].DE_end=DE.reg[i].DE_end;
				RN.reg[i].RN_start=DE.reg[i].RN_start;
				RN.reg[i].RN_end=DE.reg[i].RN_end;
				RN.reg[i].RR_start=DE.reg[i].RR_start;
				RN.reg[i].RR_end=DE.reg[i].RR_end;
				RN.reg[i].DI_start=DE.reg[i].DI_start;
				RN.reg[i].DI_end=DE.reg[i].DI_end;
				RN.reg[i].IS_start=DE.reg[i].IS_start;
				RN.reg[i].IS_end=DE.reg[i].IS_end;
				RN.reg[i].EX_start=DE.reg[i].EX_start;
				RN.reg[i].EX_end=DE.reg[i].EX_end;
				RN.reg[i].WB_start=DE.reg[i].WB_start;
				RN.reg[i].WB_end=DE.reg[i].WB_end;
				RN.reg[i].RT_start=DE.reg[i].RT_start;
				RN.reg[i].RT_end=DE.reg[i].RT_end;
			}
			RN.size = DE.size;
			for (unsigned int i = 0; i < DE.size; i++)
			{
				DE.reg[i].clear();
			}
			DE.empty = true;
			RN.empty = false;
		}
			
	}
}

void Rename()
{
	if (!RN.empty)
	{
		if (RR.empty)
		{
			// Calculating free space in the ROB
			int ROB_space;
			if (ROB.tail < ROB.head)
				ROB_space = ROB.head - ROB.tail;
			else if (ROB.head < ROB.tail)
				ROB_space = ROB.ROB_Size - (ROB.tail - ROB.head);
			else
			{
				if (ROB.tail < (ROB.ROB_Size - 1))
				{
					if (ROB.rob[ROB.tail + 1].dest == 0 && ROB.rob[ROB.tail + 1].pc == 0 && ROB.rob[ROB.tail + 1].rdy == 0)
						ROB_space = ROB.ROB_Size;
					else
						ROB_space = 0;
				}
				else
				{
					if (ROB.rob[ROB.tail - 1].dest == 0 && ROB.rob[ROB.tail - 1].pc == 0 && ROB.rob[ROB.tail - 1].rdy == 0)
						ROB_space = ROB.ROB_Size;
					else
						ROB_space = 0;
				}
			}

			// Processing the bundle
			if ((unsigned)ROB_space < RN.size)
				return;
			else
			{
				for (unsigned int i = 0; i < RN.size; i++)
				{
					if (RN.reg[i].src1 != -1)
						if (RMT.rmt[RN.reg[i].src1].valid == true)							
						{
							RN.reg[i].src1 = RMT.rmt[RN.reg[i].src1].tag;					// Rename src1
							RN.reg[i].src1_ROB = true;									
						}


					if (RN.reg[i].src2 != -1)
						if (RMT.rmt[RN.reg[i].src2].valid == true)							
						{
							RN.reg[i].src2 = RMT.rmt[RN.reg[i].src2].tag;					// Rename src2
							RN.reg[i].src2_ROB = true;										
						}
                    
					ROB.rob[ROB.tail].dest = RN.reg[i].dst;									// Set the ROB tail dst to the instruction dst
					ROB.rob[ROB.tail].pc = RN.reg[i].pc;									// Set the ROB tail pc to the instruction pc
					ROB.rob[ROB.tail].rdy = false;											// Set the ROB tail ready bit to false

					if (RN.reg[i].dst != -1)												
					{
						RMT.rmt[RN.reg[i].dst].tag = ROB.tail;								// Assign the value of the current ROB tail to the RMT table
						RMT.rmt[RN.reg[i].dst].valid = true;								
					}

					RN.reg[i].dst = ROB.tail;												// Rename the destination

					if (ROB.tail != (ROB.ROB_Size - 1))
						ROB.tail++;
					else
						ROB.tail = 0;

					RN.reg[i].RR_start = Cycle + 1;											// Updating the Cycles
					RN.reg[i].RN_end = RN.reg[i].RR_start - RN.reg[i].RN_start;
				}
				for (unsigned int i = 0; i < RN.size; i++)
			    {
					RR.reg[i].pc=RN.reg[i].pc;												// Reading the instructions from RN and storing it in RR bundle
					RR.reg[i].type=RN.reg[i].type;
					RR.reg[i].dst=RN.reg[i].dst;
					RR.reg[i].src1=RN.reg[i].src1;
					RR.reg[i].src2=RN.reg[i].src2;
					RR.reg[i].src1_original=RN.reg[i].src1_original;
					RR.reg[i].src2_original=RN.reg[i].src2_original;
					RR.reg[i].count=RN.reg[i].count;
					RR.reg[i].src1_rdy=RN.reg[i].src1_rdy;
					RR.reg[i].src2_rdy=RN.reg[i].src2_rdy;
					RR.reg[i].src1_ROB=RN.reg[i].src1_ROB;
					RR.reg[i].src2_ROB=RN.reg[i].src2_ROB;
					RR.reg[i].FE_start=RN.reg[i].FE_start;
					RR.reg[i].FE_end=RN.reg[i].FE_end;
					RR.reg[i].DE_start=RN.reg[i].DE_start;
					RR.reg[i].DE_end=RN.reg[i].DE_end;
					RR.reg[i].RN_start=RN.reg[i].RN_start;
					RR.reg[i].RN_end=RN.reg[i].RN_end;
					RR.reg[i].RR_start=RN.reg[i].RR_start;
					RR.reg[i].RR_end=RN.reg[i].RR_end;
					RR.reg[i].DI_start=RN.reg[i].DI_start;
					RR.reg[i].DI_end=RN.reg[i].DI_end;
					RR.reg[i].IS_start=RN.reg[i].IS_start;
					RR.reg[i].IS_end=RN.reg[i].IS_end;
					RR.reg[i].EX_start=RN.reg[i].EX_start;
					RR.reg[i].EX_end=RN.reg[i].EX_end;
					RR.reg[i].WB_start=RN.reg[i].WB_start;
					RR.reg[i].WB_end=RN.reg[i].WB_end;
					RR.reg[i].RT_start=RN.reg[i].RT_start;
					RR.reg[i].RT_end=RN.reg[i].RT_end;
				}
				RR.size = RN.size;
				for (unsigned int i = 0; i < RN.size; i++)
				{
					RN.reg[i].clear();
				}
				RN.empty = true;
				RR.empty = false;
			}
		}

	}
}

void RegRead()
{
	if (!RR.empty)
	{
		if (DI.empty)
		{
			for (unsigned int i = 0; i < RR.size; i++)
			{
				if (RR.reg[i].src1_ROB)											//Checking if any source register is ready
				{
					if (ROB.rob[RR.reg[i].src1].rdy == 1)
						RR.reg[i].src1_rdy = true;
				}
				else
					RR.reg[i].src1_rdy = true;

				if (RR.reg[i].src2_ROB)
				{
					if (ROB.rob[RR.reg[i].src2].rdy == 1)
						RR.reg[i].src2_rdy = true;
				}
				else
					RR.reg[i].src2_rdy = true;

				RR.reg[i].DI_start = Cycle + 1;									// Updating the Cycles
				RR.reg[i].RR_end = RR.reg[i].DI_start - RR.reg[i].RR_start;
			}
			for (unsigned int i = 0; i < RR.size; i++)
			{
				DI.reg[i].pc=RR.reg[i].pc;										// Reading the instructions from RR and storing it in DI bundle
				DI.reg[i].type=RR.reg[i].type;
				DI.reg[i].dst=RR.reg[i].dst;
				DI.reg[i].src1=RR.reg[i].src1;
				DI.reg[i].src2=RR.reg[i].src2;
				DI.reg[i].src1_original=RR.reg[i].src1_original;
				DI.reg[i].src2_original=RR.reg[i].src2_original;
				DI.reg[i].count=RR.reg[i].count;
				DI.reg[i].src1_rdy=RR.reg[i].src1_rdy;
				DI.reg[i].src2_rdy=RR.reg[i].src2_rdy;
				DI.reg[i].src1_ROB=RR.reg[i].src1_ROB;
				DI.reg[i].src2_ROB=RR.reg[i].src2_ROB;
				DI.reg[i].FE_start=RR.reg[i].FE_start;
				DI.reg[i].FE_end=RR.reg[i].FE_end;
				DI.reg[i].DE_start=RR.reg[i].DE_start;
				DI.reg[i].DE_end=RR.reg[i].DE_end;
				DI.reg[i].RN_start=RR.reg[i].RN_start;
				DI.reg[i].RN_end=RR.reg[i].RN_end;
				DI.reg[i].RR_start=RR.reg[i].RR_start;
				DI.reg[i].RR_end=RR.reg[i].RR_end;
				DI.reg[i].DI_start=RR.reg[i].DI_start;
				DI.reg[i].DI_end=RR.reg[i].DI_end;
				DI.reg[i].IS_start=RR.reg[i].IS_start;
				DI.reg[i].IS_end=RR.reg[i].IS_end;
				DI.reg[i].EX_start=RR.reg[i].EX_start;
				DI.reg[i].EX_end=RR.reg[i].EX_end;
				DI.reg[i].WB_start=RR.reg[i].WB_start;
				DI.reg[i].WB_end=RR.reg[i].WB_end;
				DI.reg[i].RT_start=RR.reg[i].RT_start;
				DI.reg[i].RT_end=RR.reg[i].RT_end;
			}
			DI.size = RR.size;
			for (unsigned int i = 0; i < RR.size; i++)
			{
				RR.reg[i].clear();
			}
			RR.empty = true;
			DI.empty = false;
		}
	}
}

void Dispatch()
{
	if (!DI.empty)
	{
		if ((iq_size - IQ.reg.size()) >= DI.size)
		{
			for (unsigned int i = 0; i < DI.size; i++)
			{
				DI.reg[i].IS_start = Cycle + 1;										// Updating the Cycles
				DI.reg[i].DI_end = DI.reg[i].IS_start - DI.reg[i].DI_start;
				IQ.reg.push_back(DI.reg[i]);										// Reading the instructions from DI and storing it in IQ bundle
			}
			for (unsigned int i = 0; i < DI.size; i++)
			{
				DI.reg[i].clear();
			}
			DI.empty = true;
		}
	}
}

void Issue()
{
	if (IQ.reg.size() != 0)
	{
		sort(IQ.reg.begin(), IQ.reg.end());

		int i = 0;

		int ctr = 1;
		while (ctr != 0)
		{
			ctr = 0;
			for (unsigned int j = 0; j < IQ.reg.size(); j++)
			{
				if (IQ.reg[j].src1_rdy && IQ.reg[j].src2_rdy)
				{
					IQ.reg[j].EX_start = Cycle + 1;									// Updating the Cycles
					IQ.reg[j].IS_end = IQ.reg[j].EX_start - IQ.reg[j].IS_start;
					Exec_List.reg.push_back(IQ.reg[j]);								// Reading the instructions from IQ and storing it in Exec_List bundle
					IQ.reg.erase(IQ.reg.begin() + j);								 
					i++;
					ctr++;
					break;
				}
			}
			if (i == width)
				break;
		}
		
	}
	
}

void Execute()
{
	if (Exec_List.reg.size() != 0)
	{
		Exec_List.dec_count();

		int ctr = 1;
		
		while (ctr != 0)
		{
			ctr = 0;

			for (unsigned int i = 0; i < Exec_List.reg.size(); i++)
			{
				if (Exec_List.reg[i].count == 0)
				{
					Exec_List.reg[i].WB_start = Cycle + 1;										// Updating the Cycles
					Exec_List.reg[i].EX_end = Exec_List.reg[i].WB_start - Exec_List.reg[i].EX_start;

					WB.reg.push_back(Exec_List.reg[i]);											// Reading the instructions from Exec_List and storing it in WB bundle

					for (unsigned int x = 0; x < IQ.reg.size(); x++)							// Wake up dependent instructions in the IQ
					{
						if (IQ.reg[x].src1 == Exec_List.reg[i].dst)
							IQ.reg[x].src1_rdy = true;

						if (IQ.reg[x].src2 == Exec_List.reg[i].dst)
							IQ.reg[x].src2_rdy = true;
					}

					for (unsigned int y = 0; y < DI.size; y++)									// Wake up dependent instructions in the DI
					{
						if (DI.reg[y].src1 == Exec_List.reg[i].dst)
							DI.reg[y].src1_rdy = true;

						if (DI.reg[y].src2 == Exec_List.reg[i].dst)
							DI.reg[y].src2_rdy = true;
					}

					for (unsigned int z = 0; z < RR.size; z++)									// Wake up dependent instructions in the RR
					{
						if (RR.reg[z].src1 == Exec_List.reg[i].dst)
							RR.reg[z].src1_rdy = true;

						if (RR.reg[z].src2 == Exec_List.reg[i].dst)
							RR.reg[z].src2_rdy = true;
					}

					Exec_List.reg.erase(Exec_List.reg.begin() + i);
					ctr++;
					break;
				}
			}
		}
	}
}

void Writeback()
{
	if (WB.reg.size() != 0)
	{
		for (unsigned int i = 0; i < WB.reg.size(); i++)
		{
			ROB.rob[WB.reg[i].dst].rdy = true;
			WB.reg[i].RT_start = Cycle + 1;										// Updating the Cycles
			WB.reg[i].WB_end = WB.reg[i].RT_start - WB.reg[i].WB_start; 
			RT.reg.push_back(WB.reg[i]);										// Reading the instructions from WB and storing it in RT bundle
		}
		WB.reg.clear();
	}
}

void Retire()
{
	int i = 0;

	while ((i < width))
	{
		if (ROB.head == ROB.tail && ROB.head != ROB.ROB_Size - 1)					
		{
			if (ROB.rob[ROB.head + 1].pc == 0)
				return;
		}

		if (ROB.rob[ROB.head].rdy)												// Check if instruction can be retired
		{
			for (unsigned int j = 0; j < RR.size; j++)
			{
				if (RR.reg[j].src1 == ROB.head)
					RR.reg[j].src1_rdy = true;

				if (RR.reg[j].src2 == ROB.head)
					RR.reg[j].src2_rdy = true;
			}

			for (unsigned int x = 0; x < RT.reg.size(); x++)
			{
				if (RT.reg[x].pc == ROB.rob[ROB.head].pc)
				{
					RT.reg[x].RT_end = (Cycle + 1) - RT.reg[x].RT_start;		// Updating the Cycles

					cout << RT.reg[x].pc << " fu{" << RT.reg[x].type << "} src{" << RT.reg[x].src1_original << "," << RT.reg[x].src2_original << "} ";
					cout << "dst{" << ROB.rob[ROB.head].dest << "} FE{" << RT.reg[x].FE_start << "," << RT.reg[x].FE_end << "} ";
					cout << "DE{" << RT.reg[x].DE_start << "," << RT.reg[x].DE_end << "} RN{" << RT.reg[x].RN_start << "," << RT.reg[x].RN_end << "} ";
					cout << "RR{" << RT.reg[x].RR_start << "," << RT.reg[x].RR_end << "} DI{" << RT.reg[x].DI_start << "," << RT.reg[x].DI_end << "} ";
					cout << "IS{" << RT.reg[x].IS_start << "," << RT.reg[x].IS_end << "} EX{" << RT.reg[x].EX_start << "," << RT.reg[x].EX_end << "} ";
					cout << "WB{" << RT.reg[x].WB_start << "," << RT.reg[x].WB_end << "} RT{" << RT.reg[x].RT_start << "," << RT.reg[x].RT_end << "} " << endl;

					RT.reg.erase(RT.reg.begin() + x);
					break;
				}

			}

			for (int z = 0; z < RMT.RMT_Size; z++)							// Update RMT contents after instruction is retired
			{
				if (RMT.rmt[z].tag == ROB.head)
				{
					RMT.rmt[z].tag = 0;
					RMT.rmt[z].valid = false;
				}
			}
			ROB.rob[ROB.head].clear();										// Update ROB contents after instruction is retired

			if (ROB.head != (ROB.ROB_Size - 1))
				ROB.head++;
			else
				ROB.head = 0;

			i++;
		}
		else
			break;
	}
}


int main(int argc, char* argv[])
{
	FILE *FP;               // File handler
	char *trace_file;       // Variable that holds trace file name;
    proc_params params;     
    
	if (argc != 5)
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];
	
	//Initializing the Global Variables
	Cycle = 0;
	PC = 0;
	Pipeline_empty = false;
	rob_size = params.rob_size;
	iq_size = params.iq_size;
	width = params.width;

	//Initializing the RMT
	RMT.RMT_Size = 67;								
	for (int i = 0; i < 67; i++)
	{
		RMT.rmt[i].valid = false;
		RMT.rmt[i].tag = -1;
	}

	//Initializing the ROB
	ROB.Create_ROB();								
	ROB.head = ROB.tail = 3;						
	
	//Initializing the Fixed Registers
	DE.create_Register();							
	RN.create_Register();
	RR.create_Register();
	DI.create_Register();
	DE.empty = RN.empty = RR.empty = DI.empty = true;

	FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

	while (Advance_Cycle(FP))
	{
		Retire();
		Writeback();
		Execute();
		Issue();
		Dispatch();
		RegRead();
		Rename();
		Decode();
		Fetch(FP);
		
		if (DE.empty && RN.empty && RR.empty && DI.empty && IQ.reg.size() == 0 && Exec_List.reg.size() == 0 && WB.reg.size() == 0)
			if (ROB.head == ROB.tail)
				if (ROB.rob[ROB.tail].pc == 0)
					Pipeline_empty = true;

		Cycle++;
	}

	cout << "# === Simulator Command =========" << endl;
	cout << "# ./sim " << params.rob_size << " " << params.iq_size<< " " << params.width << " " << trace_file << endl;
	cout << "# === Processor Configuration ===" << endl;
	cout << "# ROB_SIZE = " << params.rob_size << endl;
	cout << "# IQ_SIZE  = " << params.iq_size << endl;
	cout << "# WIDTH    = " << params.width << endl;
	cout << "# === Simulation Results ========" << endl;
	cout << "# Dynamic Instruction Count    = " << PC << endl;
	cout << "# Cycles                       = " << Cycle << endl;
	cout << "# Instructions Per Cycle (IPC) = " << fixed << setprecision(2) << ((float)PC / (float)Cycle) << endl;
}