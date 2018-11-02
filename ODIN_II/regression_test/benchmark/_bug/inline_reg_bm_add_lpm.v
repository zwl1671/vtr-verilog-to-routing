// DEFINES
`define BITS 2         // Bit width of the operands

module bm_add_lpm(clock,
		reset_n,
		a_in,
		b_in,
		out);

// SIGNAL DECLARATIONS
input	clock;
input 	reset_n;

input reg [`BITS-1:0] a_in;
input reg [`BITS-1:0] b_in;

output wire [`BITS-1:0] out;

// ASSIGN STATEMENTS
assign out = a_in + b_in;

endmodule
