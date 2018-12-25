//-----------------------------------------------------------------------------
// General-purpose miscellany.
//
// Jonathan Westhues, April 2006.
// Cex: Sized up to 16 inputs
//-----------------------------------------------------------------------------

module mux16(sel, y, x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15);
    input [3:0] sel;
    input x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    output y;
    reg y;

always @(x0 or x1 or x2 or x3 or x4 or x5 or x6 or x7 or x8 or x9 or x10 or x11 or x12 or x13 or x14 or x15 or sel)
begin
    case (sel)
        4'b0000: y = x0;
        4'b0001: y = x1;
        4'b0010: y = x2;
        4'b0011: y = x3;
        4'b0100: y = x4;
        4'b0101: y = x5;
        4'b0110: y = x6;
        4'b0111: y = x7;
        4'b1000: y = x8;
        4'b1001: y = x9;
        4'b1010: y = x10;
        4'b1011: y = x11;
        4'b1100: y = x12;
        4'b1101: y = x13;
        4'b1110: y = x14;
        4'b1111: y = x15;
    endcase
end

endmodule
