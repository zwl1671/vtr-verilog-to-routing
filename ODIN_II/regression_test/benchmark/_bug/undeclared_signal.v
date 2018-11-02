/* Odin will hot replace undeclared signal module instanciation with a dummy wire */
// this is broken for simulation as there is no generated i/o vectors, fix this bug in the simulator
// (PR is open for this related to multiclock_abc)

module top(
  clk
);
input clk;

testmod x (
  .clk(clk),
  .reset(1'b0),
  .a(DoesNotExist)
);

endmodule

module  testmod (
  clk,
  reset,
  a
);
input reset;
input clk;
input a;

reg q;

always @(posedge clk)
  q <= ~reset && a;

endmodule
