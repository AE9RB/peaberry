
//`#start header` -- edit after this line, do not edit this line
// ========================================
// Copyright 2013 David Turnbull AE9RB
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
// Generated on 10/21/2012 at 15:53
// Component: SyncSOF
module SyncSOF (
	input   clock,
	input   sod,
	input   sof
);

//`#start body` -- edit after this line, do not edit this line

    // This will sync the PSoC PLL to 38.864 MHz with a digital lock using 
    // the 1kHz from USB sof (start-of-frame). Counting begins at the start
    // of buffer 0 obtained from the DMA communicating with the DelSigs.

    // We want exactly exactly 36864 cycles per 1ms USB frame.
    wire [6:0] clockcount1;
    cy_psoc3_count7 #(.cy_period(7'b0111111))
    Counter0 (
        /* input        */ .clock(clock),
        /* input        */ .reset(sod),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(clockcount1),
        /* output       */ .tc(clocktc1)
    );
    wire [6:0] clockcount2;
    cy_psoc3_count7 #(.cy_period(7'b0111111))
    Counter1 (
        /* input        */ .clock(clocktc1),
        /* input        */ .reset(sod),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(clockcount2),
        /* output       */ .tc(clocktc2)
    );
    wire [6:0] clockcount3;
    cy_psoc3_count7 #(.cy_period(7'b0001000))
    Counter2 (
        /* input        */ .clock(clocktc2),
        /* input        */ .reset(sod),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(clockcount3),
        /* output       */ .tc()
    );

    // Report to the CPU where we are in the frame
    reg [7:0] frame_pos_hi;
	cy_psoc3_status #(.cy_md_select(8'h00), .cy_force_order(`TRUE))
	FRAME_POS_HI ( .status( frame_pos_hi ));

    reg frame_pos_ready;
    reg [6:0] frame_pos_lo;
	cy_psoc3_status #(.cy_md_select(8'h01), .cy_force_order(`TRUE))
	FRAME_POS_LO ( 
        .status( {frame_pos_lo, frame_pos_ready} ),
        .clock(clock)
    );

    // Which buffer to use for USB DMA.
    reg [1:0] buffer;
    cy_psoc3_status #(.cy_md_select(8'h00), .cy_force_order(`TRUE)) 
    BUFFER ( .status({6'b0, buffer}) );


    reg sof_sync;
    always @(posedge sof or posedge sod)
    begin
        if (sod) buffer <= 1;
        else
        begin
            sof_sync <= ~sof_sync;
            if (buffer == 2 ) buffer <= 0;
            else buffer <= buffer + 1;
        end
    end
    
    reg sof_prev;
    always @(posedge clock)
    begin
        
        if (sof_sync != sof_prev)
        begin
            sof_prev <= sof_sync;
            frame_pos_hi = {clockcount3[3:0], clockcount2[5:2]};
            frame_pos_lo = {clockcount2[1:0], clockcount1[5:1]};
            frame_pos_ready = 1;
        end
        else frame_pos_ready = 0;
    end


//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
