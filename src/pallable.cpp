//////////////////////////////////////////////////////////////////////////////////////////
// The main idea of pallable.cpp is designed for tell if each instruction can be set as Parallelizable
// 
//
// First, determine all computing instruction if they are parallelizable.
// 
// 
//
//
// 
// by @YRY 24/4/2023
// 
//////////////////////////////////////////////////////////////////////////////////////////
//TODO:

#include "../include/pcg.h"
#include "../include/pallable.h"
#include "../include/BinaryCode.h" //only for testing


//-----------------------------------------------------------------------------
// Part 0: Global variables 
// Those invisible to other modules should be guarded with static keyword
//-----------------------------------------------------------------------------
static const Func    **func_list       =   get_func();
static int            *num_nested      =   get_nested_loops_num(); 
static LOOP         ***nested_loops    =   get_nested_loops();
static int            *num_pcg_func    =   get_pcg_func_num();
static PCG_FUNC      **pcg_func_list   =   get_pcg_func();



//-----------------------------------------------------------------------------
// Part 1: 
//          Get through pcg and find if instructions are para
// 
//-----------------------------------------------------------------------------
void de_parallelizable(cs_insn *insns)
{
    int flag = 0;
    PCG_FUNC    *pcg_func;
    PCG_BLOCK   *pcg_block;

    for (int p = 0; p < *num_pcg_func + 1; p++)                              //for each pcg_func
    {
        pcg_func = (PCG_FUNC*)&((*pcg_func_list)[p]);                                   
        for(int i = 0; i < *num_nested + 1; i++)                             //for each nested loop
        {
            if((*nested_loops)[i]->func != pcg_func->func && flag == 0)
                continue;
            else if((*nested_loops)[i]->func != pcg_func->func && flag == 1)
                break;

            flag = 1;
            for(int j = (*nested_loops)[i]->src; j <=(*nested_loops)[i]->dst; j++)//for each blocks in loop
            {
                //printf("The curr block is %d\n", j);
                pcg_func->blocks[j].node = &pcg_func->func->cfg.nodes[j];
                pcg_block = &(pcg_func->blocks[j]);
                if(!de_computing(pcg_func, pcg_block, insns))                      //computing insns is para                                                   
                {
                    del(i);                                             //delete this loop
                    i -= 1;
                    break;
                }
            }
            if(*num_nested >= 0 && !de_memory(pcg_func, (*nested_loops)[i], insns))
            {
                del(i);                                             //delete this loop
                i -= 1;
                break;
            }
        }
    }

    tag();
}


//--------------------------------------------------------------------------------
//Part 2:Determine if computing instuctions can be parallel
//--------------------------------------------------------------------------------
bool de_computing(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns)
{
    int         base, in_recur;
    cs_insn    *this_ins;

    base = (pcg_block->node->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
    for(int offset = pcg_block->node->len - 1; offset >= 0; offset--)                   //From block's bottom traverse to top
    {
        this_ins = &insns[base + offset];
        if(this_ins->id == 206 && this_ins->detail->riscv.operands[0].reg != 1)//jal in loop cannot be para
            return false;
        if(this_ins->detail->riscv.op_count == 3)                              //is computing insn
        {
            in_recur = 0;                                                      // only under Prevself ins as producer situation, this_ins cannot be pallable. If this_ins is Prevself and no consumer, it can be set as treepara. 
            if(!is_com_pallable(pcg_func, pcg_block, offset, in_recur))
                return false;
        }
    }
    return true;
}


bool is_com_pallable(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, int offset, int flag)
{
    int lblock, rblock;
    int lid, rid;
    PCG_LINK *l_link;
    PCG_LINK *r_link;

    if(offset == -77)
        return true;
    if(pcg_block->producer[offset][0].type == PrevSelf && flag == 1)
        return false;
    else 
    {
        flag = 1;
        if(pcg_block->producer[offset][0].block == -77)         //which means in the curr block
            lblock = pcg_block->node->id;
        else 
            lblock = pcg_block->producer[offset][0].block;

        if(pcg_block->producer[offset][1].block == -77)         //which means in the curr block
            rblock = pcg_block->node->id;
        else 
            rblock = pcg_block->producer[offset][1].block;

        lid    = pcg_block->producer[offset][0].id;
        rid    = pcg_block->producer[offset][1].id;

        return is_com_pallable(pcg_func, &pcg_func->blocks[lblock], lid, flag) && is_com_pallable(pcg_func, &pcg_func->blocks[rblock], rid, flag);
    }
}


//--------------------------------------------------------------------------------
//Determine if memory instuctions can be parallel
//--------------------------------------------------------------------------------
bool de_memory(PCG_FUNC *pcg_func, LOOP *loop, cs_insn *insns)
{
    int swblock,lwblock;
    int start, end, temp_start, temp_end;
    cs_insn *this_ins;

    start = (loop->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
    end   = (loop->end_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;

    for(int i = start; i <= end; i++)
    {
        this_ins = &insns[i];
        if(this_ins->id == RISCV_INS_SW|| this_ins->id ==RISCV_INS_SB ||this_ins->id == RISCV_INS_SH)
        {
            for(int j = start; j <= end; j++)
            {
                if(insns[j].id == RISCV_INS_LB|| insns[j].id == RISCV_INS_LBU || insns[j].id == RISCV_INS_LH ||insns[j].id == RISCV_INS_LHU || insns[j].id == RISCV_INS_LW|| insns[j].id == RISCV_INS_LWU)
                {
                    if(this_ins->detail->riscv.operands[1].mem.base == insns[j].detail->riscv.operands[1].mem.base) //TODO:change it into a function
                        if(i >= j)
                            continue;
                        else
                        {
                            for(swblock = loop->src; swblock <= loop->dst; swblock++)
                            {
                                temp_start = (pcg_func->func->cfg.nodes[swblock].start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
                                temp_end   = (pcg_func->func->cfg.nodes[swblock].start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4 + pcg_func->func->cfg.nodes[swblock].len -1;
                                if(swblock >= temp_start && swblock <= temp_end)
                                    break;
                            }
                            for(lwblock = loop->src; lwblock <= loop->dst; lwblock++)
                            {
                                temp_start = (pcg_func->func->cfg.nodes[lwblock].start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
                                temp_end   = (pcg_func->func->cfg.nodes[lwblock].start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4 + pcg_func->func->cfg.nodes[lwblock].len -1;
                                if(lwblock >= temp_start && lwblock <= temp_end)
                                    break;
                            }
                            if(swblock == lwblock)
                            {
                                // if(is_on_path(pcg_func, PCG_BLOCK *pcg_block, int offset))                        //TODO:to be done
                                //     return false
                                // else
                                //     reorder();
                                printf("!");
                                return true;
                            }
                            else 
                                return false;
                        }
                    
                }
            }
        }
    }
    return true;
}


void tag()
{
    int flag = 0;
    PCG_FUNC *pcg_func;
    PCG_BLOCK *pcg_block;

    printf("\n\n\n");
    for (int p = 0; p < *num_pcg_func + 1; p++)                              //for each pcg_func
    {
        pcg_func = (PCG_FUNC*)&((*pcg_func_list)[p]);   
        if(*num_nested == -1)             
            printf("This program cannot be parallel\n");                 
        for(int i = 0; i < *num_nested + 1; i++)                             //for each nested loop
        {
            if((*nested_loops)[i]->func != pcg_func->func && flag == 0)
                continue;
            else if((*nested_loops)[i]->func != pcg_func->func && flag == 1)
                break;

            flag = 1;
            for(int j = (*nested_loops)[i]->src; j <=(*nested_loops)[i]->dst; j++)//for each blocks in loop
            {
                //printf("The curr block is %d\n", j);
                pcg_func->blocks[j].node = &pcg_func->func->cfg.nodes[j];
                pcg_block = &(pcg_func->blocks[j]);
                for(int offset = 0; offset <= pcg_block->node->len - 1; offset++)                   //From block's bottom traverse to top
                {
                    if(pcg_block->producer[offset][0].type == PrevSelf && (pcg_block->producer[offset][1].type == LoopConst || pcg_block->producer[offset][1].type == CurrIter || pcg_block->producer[offset][1].type == PrevIter))
                        printf("The %d Func %d block %d ins is TreePara\n", p, j, offset);
                    else if(pcg_block->producer[offset][0].type == AddrInduct && pcg_block->producer[offset][1].type == LoopConst)
                        printf("The %d Func %d block %d ins is AddrPara\n", p, j, offset);
                    else
                        printf("The %d Func %d block %d ins is FullPara\n", p, j, offset);
                }
            }
                
        }
        flag = 0;
    }
}





void del(int i)
{
    for(int offset = i; offset < *num_nested + 1; offset++)
        (*nested_loops)[i] = (*nested_loops)[i+1];
    *num_nested -= 1;
}


void testpall(void)
{
    size_t count;//used as iterator
    cs_insn *insn;//pointing at the instructions
	std::string Code;
    uint64_t address;
    if(GetCodeAndAddress("/home/rory/Desktop/test.riscv", Code, address))
    {
        unsigned char* Code_Hex = nullptr;

    	//Transform the Code into the Capstone form
    	Code_Hex = String_to_Hex(Code);

    	//Disassemblying the bit stream
    	insn = Disasm(Code_Hex, address, Code.length()/2, count);

		gen_func_list(insn, count);
		gen_cfg(insn);
        gen_loop();
        gen_pcg(insn);
        de_parallelizable(insn);
    }
}