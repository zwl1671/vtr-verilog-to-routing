// DEFINES
`define BITS 2         // Bit width of the operands
`define WORD_SIZE 4

module 	bm_simple_memory(
	clock, 
	reset, 
	addr_in,
	addr_out,
	value_out,
	value_in
);

// SIGNAL DECLARATIONS
input	clock;
input 	reset;
input 	[`WORD_SIZE-1:0]	value_in;
input 	[`BITS-1:0] 		addr_in;
input 	[`BITS-1:0] 		addr_out;

output	[`WORD_SIZE-1:0]	value_out;

reg 	[`WORD_SIZE-1:0] memory [`BITS-1:0];

always @(posedge clock)
begin
	memory[addr_in] = value_in;
	value_out = memory[addr_out];
end


endmodule
