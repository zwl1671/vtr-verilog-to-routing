module bm_cmos(
		a,
		ctl1,
		ctl2,
		out
);

input a;
input ctl1;
input ctl2;
output out;

nmos(out, a, ctl1);

endmodule
