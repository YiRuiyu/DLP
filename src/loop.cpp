///////////////////////////////////////////////////////////////////////////////
// 
// Generate Loop Area for the code
// by @YRY
// 
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <new>
#include <cstdlib>
#include "../include/loop.h"
#include "../include/BinaryCode.h"// only for testing


//-----------------------------------------------------------------------------
// Part 0: Global variables 
// Those invisible to other modules should be guarded with static keyword
//-----------------------------------------------------------------------------
static LOOP             *loop;
static int              num_loops;
static int 				NUMLOOP = 50;
static int 				NUMNESTED = 10;

//-----------------------------------------------------------------------------
// Part 1: Generate Loop Area 
// Traverse the instcutions find the Loop Area
//-----------------------------------------------------------------------------
static void gen_loop(cs_insn *insn, int count)
{
    //Variables:
    int i;
    num_loops = -1;
    LOOP *temp;
    uint64_t addr, Jmp_addr, Next_addr, offset; 
    loop = (LOOP*) malloc(sizeof(LOOP)*NUMLOOP);
    //Traverse and Find start, end address
    for(i = 0; i < count; i++)
    {
        if(isBranch(insn[i]))
        {
            addr = insn[i].address;
            cs_riscv *riscv = &(insn[i].detail->riscv);
            offset = riscv->operands[2].imm;
		    Jmp_addr = addr + offset;
            //generate loop
		    temp = isLoop(Jmp_addr, addr);
            loop_append(temp);
        }
        else continue;
    }
    //Traverse loops and Check if there are nested loops
    count_nested();
    //dump out loop list
    dump_loop();
}


// Helper functions for this part
//-----------------------------------------------------------------------------
static bool isBranch(cs_insn ins)
{
    return ins.detail->groups[0] == 1;
}



static LOOP* isLoop(uint64_t jump, uint64_t addr)
{
    if(jump < addr)
    {
        LOOP *temp = (LOOP*) malloc(sizeof(LOOP));
        temp->id = ++num_loops;
        temp->start_addr = jump;
        temp->end_addr = addr;
        temp->num = -1;
        return temp;
    }
    else return NULL; 
}



static void loop_append(LOOP *temp)
{
    if(temp != NULL)
    {
        LoopisFull(num_loops, &NUMLOOP, &loop);
        loop[num_loops] = *temp;
    }
}



static void LoopisFull(int num, int *max, LOOP **list)
{
	if(num + 1 == *max)
	{
		*max *= 2;
		*list = (LOOP*)realloc(*list, *max*sizeof(LOOP));
	}
}



static void count_nested()
{
    int i, j;
	int temp_id;
	for(i = 0; i < num_loops + 1; i++)
	{
		for(j = i + 1; j < num_loops - i + 1; j++)
		{
            if(loop[i].start_addr >= loop[j].start_addr)
            {
                loop[j].inside[++loop[j].num] = &(loop[i]);
                break;
            }
        }        
	}
}



static void dump_loop()
{
    int i, j;
	for(i = 0; i < num_loops + 1; i++)
    {
        printf("The %d Loop start address is 0x%08lx, end address is 0x%08lx\n", i, loop[i].start_addr, loop[i].end_addr);
        printf("The %d Loop has %d nested loop\n", i, loop[i].num + 1);
        for(j = 0; j < loop[i].num + 1; j++)
        {
            printf("The nested loop id is %d, start addr is 0x%08lx, end addr is 0x%08lx\n", loop[i].inside[j]->id, loop[i].inside[j]->start_addr, loop[i].inside[j]->end_addr);
        }
        printf("\n\n");
    }
}



void testloop(void)
{
	
	size_t count;//used as iterator
    cs_insn *insn;//pointing at the instructions
	std::string Code;
    uint64_t address;
    if(GetCodeAndAddress("/home/rory/Desktop/32ia.riscv", Code, address))
    {
        unsigned char* Code_Hex = nullptr;

    	//Transform the Code into the Capstone form
    	Code_Hex = String_to_Hex(Code);

    	//Disassemblying the bit stream
    	insn = Disasm(Code_Hex, address, Code.length()/2, count);

		gen_loop(insn, count);
    }
}