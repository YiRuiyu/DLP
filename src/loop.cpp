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
void gen_loop()
{
    //Variables:
    int i, j;
    const int *num_funcs = get_func_num();

    uint64_t addr, Jmp_addr;

    num_loops = -1;
    LOOP *temp;
    loop = (LOOP*) malloc(sizeof(LOOP)*NUMLOOP);

    CFG_Edge 	*edge;
    Func *func;
    const Func **func_list = get_func();
    
    //Traverse cfg find edges pointing to forward blocks
    for (i = 0; i < *num_funcs + 1; i++) {              //every func
        func = (Func *)&((*func_list)[i]);
        for (j = 0; j < func->cfg.num_edges + 1; j++) {  //every edge
		    edge = &func->cfg.edges[j];
            if(edge->type == ctrltrans)
            {
                addr = edge->src->start_addr + (edge->src->len - 1) * 0x4;
                Jmp_addr = edge->dst->start_addr;
                temp = isLoop(Jmp_addr, addr, func);
                loop_append(temp);
            }
            
	    }
    }
    //Traverse loops and Check if there are nested loops
    count_nested();
    //dump out loop list
    //dump_loop();
}


// Helper functions for this part
//-----------------------------------------------------------------------------
static bool isBranch(cs_insn ins)
{
    return ins.detail->groups[0] == 1;
}



static LOOP* isLoop(uint64_t jump, uint64_t addr, Func *func)
{
    if(jump < addr)
    {
        LOOP *temp = (LOOP*) malloc(sizeof(LOOP));
        temp->id = ++num_loops;
        temp->start_addr = jump;
        temp->end_addr = addr;
        temp->num = -1;
        temp->func = func;
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
        gen_func_list(insn, count);
		gen_cfg(insn);
		gen_loop();
    }
}


const LOOP** get_loop(void)
{
    return (const LOOP**)&loop;
}


const int* get_loop_num(void)
{
	return &num_loops;
}