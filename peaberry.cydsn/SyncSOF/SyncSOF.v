
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
    reg [8:0] pwmstate;
    reg [8:0] pwmcounter;
    reg pwm;
    assign faster = pwm;
    assign slower = ~pwm;

    
    // Which buffer to use for USB DMA.
    reg buffer;
    cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000)) 
    STATUS ( 
        /* input [07:00] */ .status(buffer),
        /* input         */ .reset(),
        /* input         */ .clock(clock)
    );
    
    
    // These values have to be found by experimentation.
    // They will vary by slightly if the debugger is plugged in.
    localparam PWM_MIN = 491;
    localparam PWM_MAX = 502;
    

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

    
    // A delay on the sod is added to avoid collisions due to jitter.
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
    cy_psoc3_count7 #(.cy_period(7'b1111111))
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
        if (sodtriggered) buffer <= 0;
        else
        begin
            sof_sync = ~sof_sync;
            buffer <= buffer + 1;
        end
    end
    
    always @(negedge clock)
    begin
        pwm <= pwmcounter < pwmstate;
    end
    
    always @(posedge clock or posedge sod)
    begin
        if (sod) sodtriggered <= 1;
        else
        begin
            pwmcounter <= pwmcounter + 1;
            if (sof_sync != sof_prev)
            begin
                sof_prev <= sof_sync;
                if (quadrant[1])
                begin
                    if (pwmstate > PWM_MIN) pwmstate <= pwmstate - 1;
                    else pwmstate <= PWM_MIN; // initialize
                end
                else
                begin
                    if (pwmstate < PWM_MAX) pwmstate <= pwmstate + 1;
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


//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
