module multiedge(
    input clk, rst, d,
    output q
);

always @(posedge clk, posedge rst)
begin
    if(rst)
        q <= 0;
    else
        q <= d;
end

endmodule