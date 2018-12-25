//-----------------------------------------------------------------------------
//
// Jonathan Westhues, April 2006
//-----------------------------------------------------------------------------

module hi_read_rx_xcorr(
    pck0, ck_1356meg, ck_1356megb,
    pwr_lo, pwr_hi, pwr_oe1, pwr_oe2, pwr_oe3, pwr_oe4,
    adc_d, adc_clk,
    ssp_frame, ssp_din, ssp_dout, ssp_clk,
    cross_hi, cross_lo,
    dbg,
    xcorr_is_848, snoop, xcorr_quarter_freq
);
    input pck0, ck_1356meg, ck_1356megb;
    output pwr_lo, pwr_hi, pwr_oe1, pwr_oe2, pwr_oe3, pwr_oe4;
    input [7:0] adc_d;
    output adc_clk;
    input ssp_dout;
    output ssp_frame, ssp_din, ssp_clk;
    input cross_hi, cross_lo;
    output dbg;
    input xcorr_is_848, snoop, xcorr_quarter_freq;

// Carrier is steady on through this, unless we're snooping.
assign pwr_hi = ck_1356megb & (~snoop);
assign pwr_oe1 = 1'b0;
assign pwr_oe3 = 1'b0;
assign pwr_oe4 = 1'b0;
// Unused.
assign pwr_lo = 1'b0;
assign pwr_oe2 = 1'b0;

assign adc_clk = ck_1356megb;  // sample frequency is 13,56 MHz

// When we're a reader, we just need to do the BPSK demod; but when we're an
// eavesdropper, we also need to pick out the commands sent by the reader,
// using AM. Do this the same way that we do it for the simulated tag.
reg after_hysteresis, after_hysteresis_prev, after_hysteresis_prev_prev;
reg [11:0] has_been_low_for;
always @(negedge adc_clk)
begin
    if(& adc_d[7:0]) after_hysteresis <= 1'b1;
    else if(~(| adc_d[7:0])) after_hysteresis <= 1'b0;

    if(after_hysteresis)
    begin
        has_been_low_for <= 7'b0;
    end
    else
    begin
        if(has_been_low_for == 12'd4095)
        begin
            has_been_low_for <= 12'd0;
            after_hysteresis <= 1'b1;
        end
        else
            has_been_low_for <= has_been_low_for + 1;
    end
end


// Let us report a correlation every 64 samples. I.e.
// one Q/I pair after 4 subcarrier cycles for the 848kHz subcarrier,
// one Q/I pair after 2 subcarrier cycles for the 424kHz subcarriers,
// one Q/I pair for each subcarrier cyle for the 212kHz subcarrier.
// We need a 6-bit counter for the timing.
reg [5:0] corr_i_cnt;
always @(negedge adc_clk)
begin
	corr_i_cnt <= corr_i_cnt + 1;
end		

// And a couple of registers in which to accumulate the correlations. From the 64 samples
// we would add at most 32 times the difference between unmodulated and modulated signal. It should
// be safe to assume that a tag will not be able to modulate the carrier signal by more than 25%.
// 32 * 255 * 0,25 = 2040, which can be held in 11 bits. Add 1 bit for sign.
// Temporary we might need more bits. For the 212kHz subcarrier we could possible add 32 times the
// maximum signal value before a first subtraction would occur. 32 * 255 = 8160 can be held in 13 bits. 
// Add one bit for sign -> need 14 bit registers but final result will fit into 12 bits.
reg signed [13:0] corr_i_accum;
reg signed [13:0] corr_q_accum;
// we will report maximum 8 significant bits
reg signed [7:0] corr_i_out;
reg signed [7:0] corr_q_out;
// clock and frame signal for communication to ARM
reg ssp_clk;
reg ssp_frame;


// The subcarrier reference signals
reg subcarrier_I;
reg subcarrier_Q;

always @(corr_i_cnt or xcorr_is_848 or xcorr_quarter_freq)
begin
	if (xcorr_is_848 & ~xcorr_quarter_freq)				// 848 kHz
		begin
			subcarrier_I = ~corr_i_cnt[3];
			subcarrier_Q = ~(corr_i_cnt[3] ^ corr_i_cnt[2]);
		end
	else if (xcorr_is_848 & xcorr_quarter_freq)			// 212 kHz	
		begin
			subcarrier_I = ~corr_i_cnt[5];
			subcarrier_Q = ~(corr_i_cnt[5] ^ corr_i_cnt[4]);
		end
	else
		begin											// 424 kHz
			subcarrier_I = ~corr_i_cnt[4];
			subcarrier_Q = ~(corr_i_cnt[4] ^ corr_i_cnt[3]);
		end
end
	
// ADC data appears on the rising edge, so sample it on the falling edge
always @(negedge adc_clk)
begin
    // These are the correlators: we correlate against in-phase and quadrature
    // versions of our reference signal, and keep the (signed) result to
    // send out later over the SSP.
    if(corr_i_cnt == 6'd0)
    begin
        if(snoop)
        begin
			// Send 7 most significant bits of tag signal (signed), plus 1 bit reader signal
			if (corr_i_accum[13:11] == 3'b000 || corr_i_accum[13:11] == 3'b111) 
				corr_i_out <= {corr_i_accum[11:5], after_hysteresis_prev_prev};
			else // truncate to maximum value
				if (corr_i_accum[13] == 1'b0)
					corr_i_out <= {7'b0111111, after_hysteresis_prev_prev};
				else
					corr_i_out <= {7'b1000000, after_hysteresis_prev_prev};
			if (corr_q_accum[13:11] == 3'b000 || corr_q_accum[13:11] == 3'b111) 
				corr_q_out <= {corr_q_accum[11:5], after_hysteresis_prev};
			else // truncate to maximum value
				if (corr_q_accum[13] == 1'b0)
					corr_q_out <= {7'b0111111, after_hysteresis_prev};
				else
					corr_q_out <= {7'b1000000, after_hysteresis_prev};
			after_hysteresis_prev_prev <= after_hysteresis;
        end
        else
        begin
            // Send 8 bits of tag signal
			if (corr_i_accum[13:11] == 3'b000 || corr_i_accum[13:11] == 3'b111) 
				corr_i_out <= corr_i_accum[11:4];
			else // truncate to maximum value
				if (corr_i_accum[13] == 1'b0)
					corr_i_out <= 8'b01111111;
				else
					corr_i_out <= 8'b10000000;
			if (corr_q_accum[13:11] == 3'b000 || corr_q_accum[13:11] == 3'b111) 
				corr_q_out <= corr_q_accum[11:4];
			else // truncate to maximum value
				if (corr_q_accum[13] == 1'b0)
					corr_q_out <= 8'b01111111;
				else
					corr_q_out <= 8'b10000000;
        end
		// Initialize next correlation. 
		// Both I and Q reference signals are high when corr_i_nct == 0. Therefore need to accumulate.
        corr_i_accum <= $signed({1'b0,adc_d});
        corr_q_accum <= $signed({1'b0,adc_d});
    end
    else
    begin
        if (subcarrier_I)
            corr_i_accum <= corr_i_accum + $signed({1'b0,adc_d});
        else
            corr_i_accum <= corr_i_accum - $signed({1'b0,adc_d});

        if (subcarrier_Q)
            corr_q_accum <= corr_q_accum + $signed({1'b0,adc_d});
        else
            corr_q_accum <= corr_q_accum - $signed({1'b0,adc_d});

    end

	// for each Q/I pair report two reader signal samples when sniffing
    if(corr_i_cnt == 6'd32)
        after_hysteresis_prev <= after_hysteresis;

    // Then the result from last time is serialized and send out to the ARM.
    // We get one report each cycle, and each report is 16 bits, so the
    // ssp_clk should be the adc_clk divided by 64/16 = 4.

    if(corr_i_cnt[1:0] == 2'b10)
        ssp_clk <= 1'b0;

    if(corr_i_cnt[1:0] == 2'b00)
    begin
        ssp_clk <= 1'b1;
        // Don't shift if we just loaded new data, obviously.
        if(corr_i_cnt != 6'd0)
        begin
            corr_i_out[7:0] <= {corr_i_out[6:0], corr_q_out[7]};
            corr_q_out[7:1] <= corr_q_out[6:0];
        end
    end

    // set ssp_frame signal for corr_i_cnt = 0..3
    // (send one frame with 16 Bits)
    if(corr_i_cnt[5:2] == 4'b0000)
        ssp_frame = 1'b1;
    else
        ssp_frame = 1'b0;

end

assign ssp_din = corr_i_out[7];

assign dbg = corr_i_cnt[3];

endmodule
