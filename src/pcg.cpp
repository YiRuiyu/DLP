//////////////////////////////////////////////////////////////////////////////////////////
// The main idea of PCG.cpp is to analyze loops to implement 
// Production and Consumption relationship.
//
// First, we do analysis on basic blocks finding producer for each instructions.
// Like, generating the producers list for instructions in the block
// 
// Then, for dealing with the instructions which has not producers in the block,
// backtracing the cfg(cfg.c & cfg.h) is a neccessay way to find out-block producers.
//
// Finally, still none-producers instructions find their producers from block following to
// it(previous loop producers).
//
// Generate PCG
// by @YRY 21/2/2023
// 
//////////////////////////////////////////////////////////////////////////////////////////
//TODO: about the branch which is easy to separated from the instructions with group

#include "../include/pcg.h"
#include "../include/BinaryCode.h" //only for testing

const int       *num_funcs = get_func_num();
const int       *num_loops = get_loop_num();
const LOOP      **loop_list = get_loop();
const Func      **func_list = get_func();

//-----------------------------------------------------------------------------
// Part 0: Global variables 
// Those invisible to other modules should be guarded with static keyword
//-----------------------------------------------------------------------------
static int          num_pcg_func;
static PCG_FUNC    *pcg_func_list;
//-----------------------------------------------------------------------------
// Part 1: Generate producer list for each block in each function
//
// Input:cs_insn --->instructions
// 
//-----------------------------------------------------------------------------
void gen_pcg(cs_insn *insns)
{
    PCG_FUNC        *pcg_func;


    num_pcg_func = get_looped_func(&pcg_func_list, loop_list);
    for (int i = 0; i < *num_funcs + 1; i++)
    {
        pcg_func_list[i].func = (Func*)&(*func_list)[i];
        pcg_func = &(pcg_func_list[i]);
        gen_func_pcg(pcg_func, insns);
    }
}

//Help function for this Part
//-----------------------------------------------------------------------------

//This function get through all PCG_FUNC and complete the PCG.
static void gen_func_pcg(PCG_FUNC *pcg_func, cs_insn *insns)
{
    PCG_BLOCK   *pcg_block;

    pcg_func->blocks = (PCG_BLOCK*)malloc(sizeof(PCG_BLOCK)*(pcg_func->func->cfg.num_nodes + 1));
    
    
    for (int i = 0; i < pcg_func->func->cfg.num_nodes + 1; i++) {
		pcg_func->blocks[i].node = &pcg_func->func->cfg.nodes[i];
        pcg_block = &(pcg_func->blocks[i]);

		in_block_link(pcg_block, insns);                                 //Generate the in-block PCG completing *producer and *non_consume
        //pre_block_link(pcg_func, pcg_block, insns);                      //Fill out-block data in PCG with the help of *non_consume and CFG
        //aft_block_link(pcg_func, pcg_block, insns);                                   //If the instruction is in a loop, the producer can be generated in last loop which is in the block after it
	}

    
}


static void in_block_link(PCG_BLOCK *pcg_block, cs_insn *insns)
{
    //Work: 1.Go through all instructions in this block
    //      2.For each instructions: find its producer and its consumer;
    //      3.Store them in PCG_LINK *producer and *non_consume;
    //Hint: instructions can be divided into two parts:
    //      Calculating instructions:       find producers
    //      Memory related instructions:    it is constructed by base and offset.                                       (To be done fisrt)
    //                                      There are three parts to find if a consumer is related to this instruction. (To be finished)
    int          base;
    cs_insn     *this_ins;

    pcg_block->producer=(PCG_LINK**)calloc((pcg_block->node->len)*2,sizeof(PCG_LINK));      //2d array for producers
    for(int i = 0; i <= pcg_block->node->len; i++)
        pcg_block->producer[i] = (PCG_LINK *)calloc(2, sizeof(PCG_LINK));
        
    pcg_block->non_consume=(PCG_LINK*)calloc(pcg_block->node->len,sizeof(PCG_LINK));
    //we divide in block analysis into two parts,computing insn and mem insn
    //analyze computing insn part 

    base = (pcg_block->node->start_addr - (*func_list)[0].cfg.nodes[0].start_addr)/0x4;
    for(int offset = pcg_block->node->len - 1; offset >= 0; offset--)                   //From block's bottom traverse to top
    {
        //printf("Base = %d\n", base);
        //printf("Offset = %d\n", offset);
        this_ins = &insns[base + offset];
        //printf("0x%" PRIx64 "\n", this_ins->address);
        if(is_auipc(this_ins->detail->riscv.op_count,this_ins->id))                   //TODO: To be considered
            continue;
        else if(is_compute(this_ins->detail->riscv.op_count))                        //Computing instructions(if its operands num is 3)
        {
            for(int k=1; k<=2; k++)                                                 //for rs1 and rs2 
                find_in_produce(pcg_block, insns, base, offset, k);
        }
        else
        {
            //TODO: memory access
        }
        
    }

    //gen_non_consume();
}
/*for(i=(block->node->start_addr+block->node->len)-1;i>block->node->start_addr-1;i--)//From block's bottom traverse to top
{
    if(is_auipc(insns[i].detail->riscv.op_count,insns[i].id))//if current insn is auipc there is no consumer so skip this turn
    {
        continue;
    }
    else if(is_compute(insns[i].detail->riscv.op_count)) //if current insn is computing insn then do next step
    {
        for(size_t k=1;k<=2;k++)//each computing insn has two comsumers
        {
            for(j=i;j>(block->node->start_addr)-1;j--)//producer of current data must in the previous insn
            {
                if(is_compute(insns[j].detail->riscv.op_count) || is_auipc(insns[j].detail->riscv.op_count,insns[j].id) )//both computing insn and auipc can produce data in rd 
                {
                    if (insns[i].detail->riscv.operands[k].type==RISCV_OP_REG)//computing insn may have operand IMM that doesn't consume anything
                    {
                        if(insns[i].detail->riscv.operands[k].reg==insns[j].detail->riscv.operands[0].reg)
                        {
                            block->producer[producer_index]->id=j;
                            block->producer[producer_index]->type=inblock;
                            block->producer[producer_index]->op->type=RISCV_OP_REG;
                            block->producer[producer_index]->op->reg=insns[j].detail->riscv.operands[0].reg;
                            producer_index++;
                            break;
                        }   
                    }
                }
            }
        }
    }
}*/


static void pre_block_link(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns)
{
    //Work: For each instruction which has missing producers
    //      Search its producers in *non_consume of previous block on the CFG
    int         *pre_block_list;
    int          num_list;
    int          base;
    int          index;
    PCG_BLOCK   *pre_block;
    cs_insn     *this_ins;                                                                  //current instruciton

    num_list = find_pre_block(&pre_block_list, pcg_block);
    //TODO: find the start index of this block
    base = pcg_block->node->start_addr/0x4;
    for(int offset = 0; offset < pcg_block->node->len; offset++)                                               //for each instruction
    {
        index = base + offset;
        this_ins = &insns[index];

        switch(isMissing(pcg_block->producer, offset))                                               //have not found producer in in-block checking
        {
            case 1:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, this_ins, offset, 1);
                break;
            case 2:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, this_ins, offset, 2);
                break;
            case 12:
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, this_ins, offset, 1);
                find_out_produce(pcg_func, pcg_block, pre_block_list, num_list, this_ins, offset, 2);
                break;
            default: continue;
        }
    }

}


static void aft_block_link(PCG_FUNC *pcg_func, PCG_BLOCK *pcg_block, cs_insn *insns)
{
    //Work: For each instruction whcih has missing producers,                                    //TODO: TO fix more
    //      if it is in a loop, find its producers in block after it of this loop.
    int         *aft_block_list;
    int          num_list;
    int         base;
    cs_insn     *this_ins;

    num_list = find_post_block(&aft_block_list, pcg_func, pcg_block);
    base = pcg_block->node->start_addr/0x4;
    for(int offset = 0; offset < pcg_block->node->len; offset++)                                     //for each instruction
    {
        this_ins = &insns[base + offset];                                                                                     
        
        switch(isMissing(pcg_block->producer, offset))                                               //have not found producer in in-block checking
        {
            case 1:
                find_out_produce(pcg_func, pcg_block, aft_block_list, num_list, this_ins, offset, 1);
                break;
            case 2:
                find_out_produce(pcg_func, pcg_block, aft_block_list, num_list, this_ins, offset, 2);
                break;
            case 12:
                find_out_produce(pcg_func, pcg_block, aft_block_list, num_list, this_ins, offset, 1);
                find_out_produce(pcg_func, pcg_block, aft_block_list, num_list, this_ins, offset, 2);
                break;
            default: continue;
        }
    }
}


static int get_looped_func(PCG_FUNC **pcg_list, const LOOP **loops)
{
    int     num_nested;
    const LOOP     *nested_loops[20] = {nullptr};

    for(int i = 0; i < *num_loops; i++)
    {
        num_nested = find_nested_loop(nested_loops, &(*loops)[i]);//nested_loops need to be freshed
}
    }
        


static int find_nested_loop(const LOOP **nested_loops, const LOOP *loop)
{        
    const LOOP     *temp_loop;
    const LOOP     *buffer[20] = {nullptr};
    int             nested_id[20];
    int             num_nested;
    int             num_buf;

    num_nested = -1;
    num_buf = 0;
    buffer[0] = loop;
    if(loop->num < 0)
        return -1;


    while(num_buf >= 0)
    {
        temp_loop = buffer[0];
        for(int i = 0; i < num_buf + 1; i++)
                buffer[i] = buffer[i + 1];
        num_buf = num_buf -1;

        if(temp_loop->num >= 0)
        {

            for(int j = 0; j <= temp_loop->num; j++)
            {
                num_buf = num_buf + 1;
                buffer[num_buf] = temp_loop->inside[j];
            }  
        }
        else    
        {
            num_nested = num_nested + 1;
            nested_loops[num_nested] = temp_loop;
        }
        
    }
    for(int i = 0; i < num_nested + 1; i++)
    {
        printf("The %d Loop start address is 0x%08lx, end address is 0x%08lx\n", i, nested_loops[i]->start_addr, nested_loops[i]->end_addr);
        printf("The %d Loop has %d nested loop\n", i, nested_loops[i]->num + 1);
    }        
    return num_nested;
}


static int isMissing(PCG_LINK **producers, int index)                       //TODO: Change it into 2-dimention array, and return 1 if op1 producer is missing, same with 2. all find is 0
                                                                             // 1: op1 producer is missing 2: op2 producer is missing 3: mem op producer is missuing 12: all missing
{
    if(producers[index][1].op == nullptr && producers[index][2].op == nullptr)
        return 12;
    else if(producers[index][1].op == nullptr)
        return 1;
    else if(producers[index][2].op == nullptr)
        return 2;
    else return 0;

}


static int find_pre_block(int **pre_block_list, PCG_BLOCK *block)
{
    CFG_Node    *node;
    int         num;

    num = -1;
    node = block->node;
    for(int i = 0; i < node->num_in + 1; i++)
        if(!(node->in[i]->type == fallthrough && node[i].id == 0))           //avoiding fallthrough crossing blocks
        {   
            num++;
            *pre_block_list[num] = node->in[i]->src->id;
        }        

    return num;
}


static int find_post_block(int **post_block_list,PCG_FUNC *pcg_func, PCG_BLOCK *block)
{
    CFG_Node    *node;
    Func        *func;
    int          num;

    num = -1;
    func = pcg_func->func;
    node = block->node;
    for(int i = 0; i < *num_loops + 1; i++)                                                                              //find which loop this 
    {
        if(node->start_addr >= (*loop_list)[i].start_addr && node->start_addr <= (*loop_list)[i].end_addr)
        {
            num++;
            *post_block_list[num] = node->id;
        }
        else break;
            
        for(int j = node->id; j < func->cfg.num_nodes; j++)
        {
            if(func->cfg.nodes[j].start_addr >= (*loop_list)[i].start_addr && func->cfg.nodes[j].start_addr + func->cfg.nodes[j].len*0x4 <= (*loop_list)[i].end_addr)
            {
                num++;
                *post_block_list[num] = j;
            }
            else break;   
        }
    }

    return num;
}


static void find_in_produce(PCG_BLOCK *pcg_block, cs_insn *insns, int base, int offset, int pos)
{
    cs_insn *this_ins;

    //printf("Base = %d\n", base);
    //printf("Offset = %d\n", offset);
    this_ins = &insns[base + offset];
    
    for(int i = base + offset; i > base; i--)
    {
        //printf("This iteration is %d\n", i);
        //printf("The k value is %d\n", pos);
        //for this_ins, this first half statement is aiming avoiding IMMs and the rest is for avoiding sw/sb/sh which cannot serve as producer
        if(this_ins->detail->riscv.operands[pos].type == RISCV_OP_REG && insns[i].id != (RISCV_INS_SW||RISCV_INS_SB||RISCV_INS_SH))             
            if(this_ins->detail->riscv.operands[pos].reg == insns[i].detail->riscv.operands[0].reg)
            {
                pcg_block->producer[offset][pos-1].id       = i - base;
                pcg_block->producer[offset][pos-1].type     = inblock;
                pcg_block->producer[offset][pos-1].op       = &insns[i].detail->riscv.operands[0];
                break;
            }
    }
}


static void find_out_produce(PCG_FUNC *pcg_func, PCG_BLOCK *block, int *pre_block_list, int num_list, cs_insn *ins, int offset, int pos)
{
    int          j;
    int          index;
    PCG_BLOCK   *pre_block;

    for(int i = 0; i < num_list + 1; i++)                                   //for each pre block
    {
        index = pre_block_list[i];
        pre_block = &pcg_func->blocks[index];
        for(j = 0; j < pre_block->num_non + 1; j++);                    //for each non_consume
            if(cmp_op(&pre_block->non_consume[j], ins, pos))
            {
                block->producer[offset][pos - 1].op = pre_block->non_consume[j].op;
                pre_block->non_consume[j].op = nullptr;
            }
                
    }
}


static bool cmp_op(PCG_LINK *link, cs_insn *ins,int position)
{
    if(link->op->type == ins->detail->riscv.operands[position].type && link->op->reg == ins->detail->riscv.operands[position].reg)           // TODO: change it to adapt to mem not only considering about reg also offset
            return true;
    else return false;
}


void dump_pcg()
{
    //for each func
        //for each block show the producer list
}


//determine whether insn is RV32I computing instruction
/*
static bool iscompute(int id){
    switch (id){
        case RISCV_INS_ADD:
        case RISCV_INS_ADDI:
        case RISCV_INS_ADDIW:
        case RISCV_INS_ADDW:
        case RISCV_INS_AND:
        case RISCV_INS_ANDI:
        //case RISCV_INS_AUIPC:
        case RISCV_INS_OR:
        case RISCV_INS_ORI:
        case RISCV_INS_SLL:
        case RISCV_INS_SLLI:
        case RISCV_INS_SLLIW:
        case RISCV_INS_SLLW:
        case RISCV_INS_SLT:
        case RISCV_INS_SLTI:
        case RISCV_INS_SLTU:
        case RISCV_INS_SLTIU:
        case RISCV_INS_SRA:
  	    case RISCV_INS_SRAI:
  	    case RISCV_INS_SRAIW:
  	    case RISCV_INS_SRAW:
  	    case RISCV_INS_SRET:
  	    case RISCV_INS_SRL:
  	    case RISCV_INS_SRLI:
  	    case RISCV_INS_SRLIW:
  	    case RISCV_INS_SRLW:
  	    case RISCV_INS_SUB:
  	    case RISCV_INS_SUBW:
        case RISCV_INS_XOR:
  	    case RISCV_INS_XORI: return true;
        default: return false;	
    }
}
*/

static bool is_compute(int opcount){
    return opcount==3;
}


static bool is_auipc(int opcount,int id){
    return (opcount==2 && id==RISCV_INS_AUIPC);
}


static bool is_mem(int opcount,int id){
    return opcount==2 && id!=RISCV_INS_AUIPC;
}

void testpcg(void)
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
        //gen_pcg(insn);
        get_looped_func(&pcg_func_list, loop_list);
        //find_nested_loop(&pcg_func_list, &(*loop_list)[21]);
    }
}
