///////////////////////////////////////////////////////////////////////////////
// 
// Generate Control Flow Graphs (CFGs) for functions in the code segement
// by @LXF, @YRY
// 
///////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
#include "../include/Disasm.h"

#define MAX_IN 16
struct CFG_Edge_T; struct CFG_Node_T; struct Func_T; struct CFG_T;
typedef enum edge_type{ functioncall = 1, ctrltrans, fallthrough } edgetype;
typedef struct CFG_Edge_T {
	CFG_Node_T	*src;
	CFG_Node_T	*dst;
	edgetype 	type;
	//type:
	//   0:function call
	//   1:branch
	//   2:sequnce
} CFG_Edge;


typedef struct CFG_Node_T {
	int 		id;	// node id within the function
	uint64_t 	start_addr;
	int 		len;
	int 		num_in;
	CFG_Edge_T	*in[MAX_IN];
	CFG_Edge_T	*out[2];
} CFG_Node;


typedef struct CFG_T {
	CFG_Node_T	*nodes;
	int 		num_nodes;
	CFG_Edge_T	*edges;
	int 		num_edges;
	Func_T 		*func;
} CFG;


typedef struct Func_T {
	uint64_t 	start_addr;
	int 		len;
	CFG_T		cfg;
} Func;


int gen_func_list(cs_insn *ins, int count);
static bool check_dup(uint64_t *func_start_list, uint64_t address);
static bool isJal(int id, int reg);
static bool isRet(int id);
static uint64_t* func_list_append(uint64_t address, cs_riscv *riscv, uint64_t *func_start_list);
static uint64_t* sort(uint64_t *list, int num);
static uint64_t get_entry();
static void dump_func_list();
void gen_cfg(cs_insn *insn);
static void gen_cfg_nodes(Func *func, cs_insn *insn);
static void gen_cfg_edges(Func *func, cs_insn *insn);
static CFG_Node* gen_node(Func *func, int i, uint64_t *node_entries, int num_node_entries, int *total_len);
static int get_node_entries(Func *func, const cs_insn *insn, uint64_t **node_entries);
static void new_edges(Func *func, CFG_Node *src, cs_insn *insn);
static CFG_Edge* gen_edge(Func *func, CFG_Node *src, uint64_t addr, edgetype type);
static CFG_Node* search_node(Func *func, uint64_t start_addr);
static CFG_Node* search_node_jal(uint64_t start_addr);
static int find_index(const cs_insn *insn, uint64_t addr);
static void dump_node_entries(Func *func, uint64_t *node_entries, int num_node_entries);
static void isFull(int num, int *max, uint64_t **list);
static void EdgeisFull(int num, int *max, CFG_Edge **list);
static uint64_t* bran_append(cs_insn ins, uint64_t *list, int *num);
static uint64_t* jal_append(cs_insn ins, uint64_t *list, int *num);
static void dump_cfg_nodes(Func *func);
static void dump_cfg_edges(Func *func);
void testcfg(void);