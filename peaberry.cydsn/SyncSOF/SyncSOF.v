
//`#start header` -- edit after this line, do not edit this line
// ========================================
// Copyright 2012 David Turnbull AE9RB
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ========================================
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 09/20/2012 at 19:55
// Component: SyncSOF
module SyncSOF (
	output  faster,
	output  slower,
	input   clock,
	input   sod,
	input   sof
);

//`#start body` -- edit after this line, do not edit this line

    // This will sync the PSoC PLL to 38.864 MHz with a digital lock using 
    // the 1kHz from USB sof (start-of-frame). Counting begins at the start
    // of buffer 0 obtained from the DMA communicating with the DelSigs.

    reg [1:0] quadrant;
    reg sodtriggered;
    reg reloadtriggered;
    reg sof_sync;
    reg sof_prev;
    reg [2:0] pwmstate;
    reg [8:0] pwmcounter;
    reg pwm;
    reg [1:0] pwmup;
    reg [1:0] pwmdown;
    
    assign faster = pwm;
    assign slower = ~pwm;

    
    // Which buffer to use for USB DMA.
    reg [1:0] buffer;
    cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000)) 
    Buffer ( 
        /* input [07:00] */ .status({6'b0, buffer}),
        /* input         */ .reset(),
        /* input         */ .clock(clock)
    );
    
    
    // These values have to be found by experimentation.
    // They will vary by slightly if the debugger is plugged in.
    localparam PWM_MIN = 479;
    localparam PWM_MAX = 486;
    localparam PWM_PERIOD = 499;
    localparam PWM_HOLD = 3;
    

    // The PLL is set higher than the target of 36.864.
    // 36.932 is the closest match.  We will use DMA to
    // nudge this lower by adjusting FASTCLK_PLL_P.
    // This value is obtained from cyfitter_cfg.c...
    // CY_SET_XTND_REG16((void CYFAR *)(CYREG_FASTCLK_PLL_P), 0x0C14);
    localparam FASTCLK_PLL_P_LSB = 8'h14;
    
    
    // The PLL_P values are stored in status registers to keep
    // DMA from using SRAM.  Small but easy boost to CPU speed.
    wire [7:0] pll_hi = FASTCLK_PLL_P_LSB;
    cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000)) 
    PLL_HI ( 
        /* input [07:00] */ .status(pll_hi),
        /* input         */ .reset(),
        /* input         */ .clock()
    );
    wire [7:0] pll_lo = FASTCLK_PLL_P_LSB-1;
    cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000)) 
    PLL_LO ( 
        /* input [07:00] */ .status(pll_lo),
        /* input         */ .reset(),
        /* input         */ .clock()
    );

    
    // A 1/32 frame delay on the sod is added to avoid collisions due to jitter.
    wire [6:0] delaycount1;
    cy_psoc3_count7 #(.cy_period(7'b1111111))
    Counter0 (
        /* input        */ .clock(clock),
        /* input        */ .reset(sod),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(delaycount1),
        /* output       */ .tc(delaytc1)
    );
    wire [6:0] delaycount2;
    cy_psoc3_count7 #(.cy_period(7'b0001000))
    Counter1 (
        /* input        */ .clock(delaytc1),
        /* input        */ .reset(sod),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(delaycount2),
        /* output       */ .tc(delaytc2)
    );


    // We want exactly exactly 36864 cycles per 1ms USB frame.
    // Count to 9216 and increment the quadrant each time.
    // This makes efficient use of the 7-bit counters and gives us
    // us the knowledge of which half of the buffer we're at.
    wire [6:0] clockcount1;
    cy_psoc3_count7 #(.cy_period(7'b1111111))
    Counter2 (
        /* input        */ .clock(clock),
        /* input        */ .reset(reloadtriggered),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(clockcount1),
        /* output       */ .tc(clocktc1)
    );
    wire [6:0] clockcount2;
    cy_psoc3_count7 #(.cy_period(7'b1000111))
    Counter3 (
        /* input        */ .clock(clocktc1),
        /* input        */ .reset(reloadtriggered),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(clockcount2),
        /* output       */ .tc(clocktc2)
    );
    
    
    always @(posedge clocktc2 or posedge reloadtriggered)
    begin
        if (reloadtriggered) quadrant <= 0;
        else quadrant <= quadrant + 1;
    end
    
    always @(posedge sof or posedge sodtriggered)
    begin
        if (sodtriggered) buffer <= 1;
        else
        begin
            sof_sync <= ~sof_sync;
            if (buffer == 2) buffer <= 0;
            else buffer <= buffer + 1;
        end
    end
    
    always @(posedge clock or posedge sod)
    begin
        if (sod) sodtriggered <= 1;
        else
        begin
            pwm <= pwmcounter < pwmstate + PWM_MIN;
            if (!pwmcounter) pwmcounter <= PWM_PERIOD;
            else pwmcounter <= pwmcounter - 1;
            if (sof_sync != sof_prev)
            begin
                sof_prev <= sof_sync;
                if (quadrant[1])
                begin
                    if (!pwmdown)
                    begin
                        if (pwmstate > 0) pwmstate <= pwmstate - 1;
                        if (!pwmup) pwmdown <= PWM_HOLD;
                    end
                    else pwmdown <= pwmdown - 1;
                    pwmup <= pwmup/2;
                end
                else
                begin
                    if (!pwmup) 
                    begin
                        if (pwmstate < PWM_MAX - PWM_MIN) pwmstate <= pwmstate + 1;
                        if (!pwmdown) pwmup <= PWM_HOLD;
                    end
                    else pwmup <= pwmup - 1;
                    pwmdown <= pwmdown/2;
                end
            end
            if (delaytc2 && sodtriggered)
            begin
                sodtriggered <= 0;
                reloadtriggered <= 1;
            end
            else reloadtriggered <= 0;
        end
    end


    wire delaycountdownclock;
    wire delaycountdownzero;
    cy_psoc3_udb_clock_enable_v1_0 #(.sync_mode(`TRUE)) 
    ClockSyncBypass
    (
        /* input  */    .clock_in(buffer),
        /* input  */    .enable(1'b1),
        /* output */    .clock_out(delaycountdownclock)
    ); 

    cy_psoc3_dp8 #(.cy_dpconfig_a(
    {
        `CS_ALU_OP__DEC, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC__ALU, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM0: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM1: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM2: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM3: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM4: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM5: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM6: */
        `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
        `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
        `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
        `CS_CMP_SEL_CFGA, /*CFGRAM7: */
        8'hFF, 8'h00,  /*CFG9: */
        8'hFF, 8'hFF,  /*CFG11-10: */
        `SC_CMPB_A1_D1, `SC_CMPA_A1_D1, `SC_CI_B_ARITH,
        `SC_CI_A_ARITH, `SC_C1_MASK_DSBL, `SC_C0_MASK_DSBL,
        `SC_A_MASK_DSBL, `SC_DEF_SI_0, `SC_SI_B_DEFSI,
        `SC_SI_A_DEFSI, /*CFG13-12: */
        `SC_A0_SRC_ACC, `SC_SHIFT_SL, 1'h0,
        1'h0, `SC_FIFO1_BUS, `SC_FIFO0_BUS,
        `SC_MSB_DSBL, `SC_MSB_BIT0, `SC_MSB_NOCHN,
        `SC_FB_NOCHN, `SC_CMP1_NOCHN,
        `SC_CMP0_NOCHN, /*CFG15-14: */
        10'h00, `SC_FIFO_CLK__DP,`SC_FIFO_CAP_AX,
        `SC_FIFO_LEVEL,`SC_FIFO__SYNC,`SC_EXTCRC_DSBL,
        `SC_WRK16CAT_DSBL /*CFG17-16: */
    }
    )) DelayCountdown(
            /*  input                   */  .reset(1'b0),
            /*  input                   */  .clk(delaycountdownclock),
            /*  input   [02:00]         */  .cs_addr({2'b0, delaycountdownzero}),
            /*  input                   */  .route_si(1'b0),
            /*  input                   */  .route_ci(1'b0),
            /*  input                   */  .f0_load(1'b0),
            /*  input                   */  .f1_load(1'b0),
            /*  input                   */  .d0_load(1'b0),
            /*  input                   */  .d1_load(1'b0),
            /*  output                  */  .ce0(),
            /*  output                  */  .cl0(),
            /*  output                  */  .z0(delaycountdownzero),
            /*  output                  */  .ff0(),
            /*  output                  */  .ce1(),
            /*  output                  */  .cl1(),
            /*  output                  */  .z1(),
            /*  output                  */  .ff1(),
            /*  output                  */  .ov_msb(),
            /*  output                  */  .co_msb(),
            /*  output                  */  .cmsb(),
            /*  output                  */  .so(),
            /*  output                  */  .f0_bus_stat(),
            /*  output                  */  .f0_blk_stat(),
            /*  output                  */  .f1_bus_stat(),
            /*  output                  */  .f1_blk_stat()
    );

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line

