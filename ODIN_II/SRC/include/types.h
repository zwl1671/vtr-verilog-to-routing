/*

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include "string_cache.h"
#include "odin_util.h"
#include "read_xml_arch_file.h"
#include "simulate_blif.h"
#include "argparse_value.hpp"
#include <mutex>

#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TYPES_H
#define TYPES_H

#define ODIN_STD_BITWIDTH (sizeof(long long)*8)

#define TO_ENUM(op) op
#define TO_STRING(op) #op

typedef struct config_t_t config_t;
typedef struct global_args_t_t global_args_t;
/* new struct for the global arguments of verify_blif function */
typedef struct global_args_read_blif_t_t global_args_read_blif_t;

typedef struct ast_node_t_t ast_node_t;
typedef struct typ_t typ;

typedef struct sim_state_t_t sim_state_t;
typedef struct nnode_t_t nnode_t;
typedef struct ace_obj_info_t_t ace_obj_info_t;
typedef struct npin_t_t npin_t;
typedef struct nnet_t_t nnet_t;
typedef struct signal_list_t_t signal_list_t;
typedef struct char_list_t_t char_list_t;
typedef struct netlist_t_t netlist_t;
typedef struct netlist_stats_t_t netlist_stats_t;
typedef struct chain_information_t_t chain_information_t;

// to define type of adder in cmd line
typedef struct adder_def_t_t adder_def_t;

/* for parsing and AST creation errors */
#define PARSE_ERROR -3
/* for netlist creation oerrors */
#define NETLIST_ERROR -4
/* for blif read errors */
#define BLIF_ERROR -5
/* for errors in netlist (clustered after tvpack) errors */
#define NETLIST_FILE_ERROR -6
/* for errors in activation estimateion creation */
#define ACTIVATION_ERROR -7
/* for errors in the netlist simulation */
#define SIMULATION_ERROR -8

/* unique numbers to mark the nodes as we DFS traverse the netlist */
#define PARTIAL_MAP_TRAVERSE_VALUE 10
#define OUTPUT_TRAVERSE_VALUE 12
#define COUNT_NODES 14 /* NOTE that you can't call countnodes one after the other or the mark will be incorrect */
#define COMBO_LOOP 15
#define COMBO_LOOP_ERROR 16
#define GRAPH_CRUNCH 17
#define STATS 18
#define SEQUENTIAL_LEVELIZE 19

/* unique numbers for using void *data entries in some of the datastructures */
#define RESET -1
#define LEVELIZE 12
#define ACTIVATION 13

// causes an interrupt in GDB
#define verbose_assert(condition) std::cerr << "ASSERT FAILED: " << #condition << " \n\t@ " << __LINE__ << "::" << __FILE__ << std::endl;
#define oassert(condition) { if(!(condition)){ verbose_assert(condition); std::abort();} }
// bitvector library (PETER_LIB) defines it, so we don't

/* This is the data structure that holds config file details */
struct config_t_t
{
	std::vector<std::string> list_of_file_names;

	std::string output_type; // string name of the type of output file

	std::string debug_output_path; // path for where to output the debug outputs
	short output_ast_graphs; // switch that outputs ast graphs per node for use with GRaphViz tools
	short output_netlist_graphs; // switch that outputs netlist graphs per node for use with GraphViz tools
	short print_parse_tokens; // switch that controls whether or not each token is printed during parsing
	short output_preproc_source; // switch that outputs the pre-processed source
	int min_hard_multiplier; // threshold from hard to soft logic
	int mult_padding; // setting how multipliers are padded to fit fixed size
	// Flag for fixed or variable hard mult (1 or 0)
	int fixed_hard_multiplier;
	// Flag for splitting hard multipliers If fixed_hard_multiplier is set, this must be 1.
	int split_hard_multiplier;
	// 1 to split memory width down to a size of 1. 0 to split to arch width.
	char split_memory_width;
	// Set to a positive integer to split memory depth to that address width. 0 to split to arch width.
	int split_memory_depth;

	//add by Sen
	// Threshold from hard to soft logic(extra bits)
	int min_hard_adder;
	int add_padding; // setting how multipliers are padded to fit fixed size
	// Flag for fixed or variable hard mult (1 or 0)
	int fixed_hard_adder;
	// Flag for splitting hard multipliers If fixed_hard_multiplier is set, this must be 1.
	int split_hard_adder;
	//  Threshold from hard to soft logic
	int min_threshold_adder;

	// If the memory is smaller than both of these, it will be converted to soft logic.
	int soft_logic_memory_depth_threshold;
	int soft_logic_memory_width_threshold;

	char *arch_file; // Name of the FPGA architecture file
};

typedef enum {
	NO_SIMULATION = 0,
	TEST_EXISTING_VECTORS,
	GENERATE_VECTORS,
}simulation_type;

/* the global arguments of the software */
struct global_args_t_t
{
    argparse::ArgValue<char*> config_file;
	argparse::ArgValue<std::vector<std::string>> verilog_files;
	argparse::ArgValue<char*> blif_file;
	argparse::ArgValue<char*> output_file;
	argparse::ArgValue<char*> arch_file; // Name of the FPGA architecture file

	argparse::ArgValue<char*> high_level_block; //Legacy option, no longer used

    argparse::ArgValue<bool> write_netlist_as_dot;
    argparse::ArgValue<bool> write_ast_as_dot;
    argparse::ArgValue<bool> all_warnings;
    argparse::ArgValue<bool> show_help;

    argparse::ArgValue<bool> black_box_latches; //Weather or not to treat and output latches as black boxes
	argparse::ArgValue<char*> adder_def; //carry skip adder skip size

	/////////////////////
	// For simulation.
	/////////////////////
	// Generate this number of random vectors.
	argparse::ArgValue<int> sim_num_test_vectors;
	// Input vectors to simulate instead of generating vectors.
	argparse::ArgValue<char*> sim_vector_input_file;
	// Existing output vectors to verify against.
	argparse::ArgValue<char*> sim_vector_output_file;
	// Simulation output Directory
	argparse::ArgValue<char*> sim_directory;
	// Tells the simulator whether or not to generate random vectors which include the unknown logic value.
	argparse::ArgValue<bool> sim_generate_three_valued_logic;
	// Output both falling and rising edges in the output_vectors file. (DEFAULT)
	argparse::ArgValue<bool> sim_output_both_edges;
	// Additional pins, nets, and nodes to output.
	argparse::ArgValue<std::vector<std::string>> sim_additional_pins;
	// Comma-separated list of primary input pins to hold high for all cycles but the first.
	argparse::ArgValue<std::vector<std::string>> sim_hold_high;
	// Comma-separated list of primary input pins to hold low for all cycles but the first.
	argparse::ArgValue<std::vector<std::string>> sim_hold_low;

	argparse::ArgValue<int> parralelized_simulation;
	//
	argparse::ArgValue<int> sim_initial_value;
	// The seed for creating random simulation vector
    argparse::ArgValue<int> sim_random_seed;

	argparse::ArgValue<bool> interactive_simulation;

};

#endif // TYPES_H

//-----------------------------------------------------------------------------------------------------

#ifndef AST_TYPES_H
#define AST_TYPES_H

typedef enum
{
	DEC = 10,
	HEX = 16,
	OCT = 8,
	BIN = 2,
	LONG_LONG = 0
} bases;

typedef enum
{
	SIGNED,
	UNSIGNED
} signedness;

#define OP_LIST(_TYP) {\
	_TYP(NO_OP),\
	_TYP(MULTI_PORT_MUX),	/* port 1 = control port 2+ = mux options */\
	_TYP(FF_NODE),\
	_TYP(BUF_NODE),\
	_TYP(INPUT_NODE),\
	_TYP(OUTPUT_NODE),\
	_TYP(GND_NODE),\
	_TYP(VCC_NODE),\
	_TYP(CLOCK_NODE),\
	_TYP(ADD),	/* + */\
	_TYP(MINUS),	/* - */\
	_TYP(BITWISE_NOT), 	/* ~ */\
	_TYP(BITWISE_AND),	/* & */\
	_TYP(BITWISE_OR),	/* | */\
	_TYP(BITWISE_NAND),	/* ~& */\
	_TYP(BITWISE_NOR),	/* ~| */\
	_TYP(BITWISE_XNOR),	/* ~^ */\
	_TYP(BITWISE_XOR),	/* ^ */\
	_TYP(LOGICAL_NOT),	/* ! */\
	_TYP(LOGICAL_OR),	/* || */\
	_TYP(LOGICAL_AND),	/* && */\
	_TYP(LOGICAL_NAND),	/* No Symbol */\
	_TYP(LOGICAL_NOR),	/* No Symbol */\
	_TYP(LOGICAL_XNOR), /* No symbol */\
	_TYP(LOGICAL_XOR),	/* No Symbol */\
	_TYP(MULTIPLY),	/* * */\
	_TYP(DIVIDE),	/* / */\
	_TYP(MODULO),	/* % */\
	_TYP(OP_POW),	/* ** */\
	_TYP(LT), 			/* < */\
	_TYP(GT), 			/* > */\
	_TYP(LOGICAL_EQUAL),	/*  == */\
	_TYP(NOT_EQUAL),	/* != */\
	_TYP(LTE),	/* <= */\
	_TYP(GTE),	/* >= */\
	_TYP(SR),	/* >> */\
	_TYP(SL),	/* << */\
	_TYP(CASE_EQUAL),	/* === */\
	_TYP(CASE_NOT_EQUAL),	/* !== */\
	_TYP(ADDER_FUNC), \
	_TYP(CARRY_FUNC), \
	_TYP(MUX_2),\
	_TYP(BLIF_FUNCTION),\
	_TYP(NETLIST_FUNCTION),\
	_TYP(MEMORY),\
	_TYP(PAD_NODE),\
	_TYP(HARD_IP),\
	_TYP(GENERIC),	/* added for the unknown node type */\
	_TYP(FULLADDER),\
	_TYP(CMOS),\
	_TYP(NMOS),\
	_TYP(PMOS),\
	_TYP(NOTIF0),\
	_TYP(NOTIF1),\
	_TYP(BUF),\
	_TYP(LAST_OP)\
}
typedef enum OP_LIST(TO_ENUM) operation_list;

#define IDS_LIST(_TYP) {\
	_TYP(NO_ID),\
	/* top level things */\
	_TYP(FILE_ITEMS),\
	_TYP(MODULE),\
	/* VARIABLES */\
	_TYP(INPUT),\
	_TYP(OUTPUT),\
	_TYP(INOUT),\
	_TYP(WIRE),\
	_TYP(REG),\
	_TYP(INTEGER),\
	_TYP(PARAMETER),\
	_TYP(INITIALS),\
	_TYP(PORT),\
	/* OTHER MODULE ITEMS */\
	_TYP(MODULE_ITEMS),\
	_TYP(VAR_DECLARE),\
	_TYP(VAR_DECLARE_LIST),\
	_TYP(ASSIGN),\
   	/* OTHER MODULE AND FUNCTION ITEMS */\
	_TYP(FUNCTION),\
   	/* OTHER FUNCTION ITEMS */\
	_TYP(FUNCTION_ITEMS),\
	/* primitives */\
	_TYP(GATE),\
	_TYP(GATE_INSTANCE),\
	_TYP(ONE_GATE_INSTANCE),\
	/* Module instances */\
	_TYP(MODULE_CONNECT_LIST),\
	_TYP(MODULE_CONNECT),\
	_TYP(MODULE_PARAMETER_LIST),\
	_TYP(MODULE_PARAMETER),\
	_TYP(MODULE_NAMED_INSTANCE),\
	_TYP(MODULE_INSTANCE),\
	_TYP(MODULE_MASTER_INSTANCE),\
	_TYP(ONE_MODULE_INSTANCE),\
	/* Function instances*/\
	_TYP(FUNCTION_NAMED_INSTANCE),\
	_TYP(FUNCTION_INSTANCE),\
	_TYP(SPECIFY_PAL_CONNECTION_STATEMENT),\
	_TYP(SPECIFY_PAL_CONNECT_LIST),\
	/* statements */\
	_TYP(BLOCK),\
	_TYP(NON_BLOCKING_STATEMENT),\
	_TYP(BLOCKING_STATEMENT),\
	_TYP(ASSIGNING_LIST),\
	_TYP(CASE),\
	_TYP(CASE_LIST),\
	_TYP(CASE_ITEM),\
	_TYP(CASE_DEFAULT),\
	_TYP(ALWAYS),\
	_TYP(IF),\
	_TYP(IF_Q),\
	_TYP(FOR),\
	_TYP(WHILE),\
	/* Delay Control */\
	_TYP(DELAY_CONTROL),\
	_TYP(POSEDGE),\
	_TYP(NEGEDGE),\
	/* expressions */\
	_TYP(BINARY_OPERATION),\
	_TYP(UNARY_OPERATION),\
	/* basic primitives */\
	_TYP(ARRAY_REF),\
	_TYP(RANGE_REF),\
	_TYP(CONCATENATE),\
	/* basic identifiers */\
	_TYP(IDENTIFIERS),\
	_TYP(NUMBERS),\
	/* Hard Blocks */\
	_TYP(HARD_BLOCK),\
	_TYP(HARD_BLOCK_NAMED_INSTANCE),\
	_TYP(HARD_BLOCK_CONNECT_LIST),\
	_TYP(HARD_BLOCK_CONNECT),\
	/* EDDIE: new enum value for ids to replace MEMORY from operation_t */\
	_TYP(RAM),\
	_TYP(LAST_ID)\
}
typedef enum IDS_LIST(TO_ENUM) ids;


struct typ_t
{
	char *identifier;

	struct
	{
		short sign;
		short base;
		int size;
		int binary_size;
		char *binary_string;
		char *number;
		long long value;
		short is_full; //'bx means all of the wire get 'x'(dont care)
	} number;
	struct
	{
		operation_list op;
	} operation;
	struct
	{
		short is_parameter;
		short is_port;
		short is_input;
		short is_output;
		short is_inout;
		short is_wire;
		short is_reg;
		short is_integer;
		short is_initialized; // should the variable be initialized with some value?
		long long initial_value;
	} variable;
	struct
	{
		short is_instantiated;
		ast_node_t **module_instantiations_instance;
		int size_module_instantiations;
		int index;
	} module;
	struct
	{
		short is_instantiated;
		ast_node_t **function_instantiations_instance;
		int size_function_instantiations;
		int index;
	} function;
	struct
	{
		int num_bit_strings;
		char **bit_strings;
	} concat;

};


struct ast_node_t_t
{
	size_t unique_count;
	int far_tag;
	int high_number;
	ids type;
	typ types;

	ast_node_t **children;
	size_t num_children;

	int line_number;
	int file_number;

	short shared_node;
	void *hb_port;
	void *net_node;
	short is_read_write;

};
#endif // AST_TYPES_H

//-----------------------------------------------------------------------------------------------------
#ifndef NETLIST_UTILS_H
#define NETLIST_UTILS_H
/* DEFINTIONS for carry chain*/
struct chain_information_t_t
{
	char *name;//unique name of the chain
	int count;//the number of hard blocks in this chain
	int num_bits;
};

/* DEFINTIONS for all the different types of nodes there are.  This is also used cross-referenced in utils.c so that I can get a string version
 * of these names, so if you add new tpyes in here, be sure to add those same types in utils.c */
struct nnode_t_t
{
	long unique_id;
	char *name; // unique name of a node
	operation_list type; // the type of node
	int bit_width; // Size of the operation (e.g. for adders/subtractors)

	ast_node_t *related_ast_node; // the abstract syntax node that made this node

	short traverse_visited; // a way to mark if we've visited yet

	npin_t **input_pins; // the input pins
	int num_input_pins;
	int *input_port_sizes; // info about the input ports
	int num_input_port_sizes;

	npin_t **output_pins; // the output pins
	int num_output_pins;
	int *output_port_sizes; // info if there is ports
	int num_output_port_sizes;

	short unique_node_data_id;
	void *node_data; // this is a point where you can add additional data for your optimization or technique

	int forward_level; // this is your logic level relative to PIs and FFs .. i.e farthest PI
	int backward_level; // this is your reverse logic level relative to POs and FFs .. i.e. farthest PO
	int sequential_level; // the associated sequential network that the node is in
	short sequential_terminator; // if this combinational node is a terminator for the sequential level (connects to flip-flop or Output pin

	netlist_t* internal_netlist; // this is a point of having a subgraph in a node

	signed char *memory_data;
	//(int cycle, int num_input_pins, npin_t *inputs, int num_output_pins, npin_t *outputs);
	void (*simulate_block_cycle)(int, int, int*, int, int*);

	short *associated_function;

	char** bit_map; /*storing the bit map */
	int bit_map_line_count;

	// For simulation
	int in_queue; // Flag used by the simulator to avoid double queueing.
	npin_t **undriven_pins; // These pins have been found by the simulator to have no driver.
	int num_undriven_pins;
	int ratio; //clock ratio for clock nodes
	signed char has_initial_value; // initial value assigned?
	signed char initial_value; // initial net value

	//Generic gate output
	unsigned char generic_output; //describes the output (1 or 0) of generic blocks
};


// Ace_Obj_Info_t; /* Activity info for each node */
struct ace_obj_info_t_t
{
	int value;
	int num_ones;
	int num_toggles;
	double static_prob;
	double switch_prob;
	double switch_act;
	double prob0to1;
	double prob1to0;
	int depth;
};

struct npin_t_t
{
	long unique_id;
	ids type;         // INPUT or OUTPUT
	char *name;
	nnet_t *net;      // related net
	int pin_net_idx;
	nnode_t *node;    // related node
	int pin_node_idx; // pin on the node where we're located
	char *mapping;    // name of mapped port from hard block

	////////////////////
	// For simulation
	std::mutex pin_lock;
	bool is_being_written;
	int nb_of_reader;

	signed char *values; // The values for the current wave.
	int *cycle;          // The last cycle the pin was computed for.
	unsigned long coverage;
	bool is_default; // The pin is feeding a mux from logic representing an else or default.
	bool is_implied; // This signal is implied.



	////////////////////

        // For Activity Estimation
        ace_obj_info_t *ace_info;

};

struct nnet_t_t
{
	long unique_id;
	char *name; // name for the net
	short combined;

	npin_t *driver_pin; // the pin that drives the net

	npin_t **fanout_pins; // the pins pointed to by the net
	int num_fanout_pins; // the list size of pins

	short unique_net_data_id;
	void *net_data;

	/////////////////////
	// For simulation
	signed char values[SIM_WAVE_LENGTH];  // Stores the values of all connected pins.
	int cycle;                            // Stores the cycle of all connected pins.
	signed char has_initial_value; // initial value assigned?
	signed char initial_value; // initial net value
	//////////////////////
};

struct signal_list_t_t
{
	npin_t **pins;
	int count;

	char is_memory;
	char is_adder;
};

struct char_list_t_t
{
	char **strings;
	int num_strings;
};

struct netlist_t_t
{
	nnode_t *gnd_node;
	nnode_t *vcc_node;
	nnode_t *pad_node;
	nnet_t *zero_net;
	nnet_t *one_net;
	nnet_t *pad_net;
	nnode_t** top_input_nodes;
	int num_top_input_nodes;
	nnode_t** top_output_nodes;
	int num_top_output_nodes;
	nnode_t** ff_nodes;
	int num_ff_nodes;
	nnode_t** internal_nodes;
	int num_internal_nodes;
	nnode_t** clocks;
	int num_clocks;


	/* netlist levelized structures */
	nnode_t ***forward_levels;
	int num_forward_levels;
	int* num_at_forward_level;
	nnode_t ***backward_levels; // NOTE backward levels isn't neccessarily perfect.  Because of multiple output pins, the node can be put closer to POs than should be.  To fix, run a rebuild of the list afterwards since the marked "node->backward_level" is correct */
	int num_backward_levels;
	int* num_at_backward_level;

	nnode_t ***sequential_level_nodes;
	int num_sequential_levels;
	int* num_at_sequential_level;
	/* these structures store the last combinational node in a level before a flip-flop or output pin */
	nnode_t ***sequential_level_combinational_termination_node;
	int num_sequential_level_combinational_termination_nodes;
	int* num_at_sequential_level_combinational_termination_node;

	STRING_CACHE *nets_sc;
	STRING_CACHE *out_pins_sc;
	STRING_CACHE *nodes_sc;

	netlist_stats_t *stats;

	t_type_ptr type;
};

struct netlist_stats_t_t
{
	int num_inputs;
	int num_outputs;
	int num_ff_nodes;
	int num_logic_nodes;
	int num_nodes;

	float average_fanin; /* = to the fanin of all nodes: basic outs, combo and ffs */
	int *fanin_distribution;
	int num_fanin_distribution;

	long num_output_pins;
	float average_output_pins_per_node;
	float average_fanout; /* = to the fanout of all nodes: basic IOs, combo and ffs...no vcc, clocks, gnd */
	int *fanout_distribution;
	int num_fanout_distribution;

	/* expect these should be close but doubt they'll be equal */
	long num_edges_fo; /* without control signals like clocks and gnd and vcc...= to number of fanouts from output pins */
	long num_edges_fi; /* without control signals like clocks and gnd and vcc...= to number of fanins from output pins */

	int **combinational_shape;
	int *num_combinational_shape_for_sequential_level;
};

#endif // NET_TYPES_H
