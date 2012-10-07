
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
// Generated on 09/05/2012 at 18:13
// Component: IQGen
module IQGen (
	output [1:0] qsd,
	output [1:0] qse,
	output  tx,
	input   clock
);

//`#start body` -- edit after this line, do not edit this line

    wire counter_clock;
    cy_psoc3_udb_clock_enable_v1_0 #(.sync_mode(`FALSE)) 
    ClockSyncBypass
    (
        /* input  */    .clock_in(clock),
        /* input  */    .enable(1'b1),
        /* output */    .clock_out(counter_clock)
    ); 

    wire [7:0] settings;
    cy_psoc3_control # (.cy_init_value(8'b0), .cy_force_order(1))
    Settings ( .control(settings) );

    wire [6:0] count;
    cy_psoc3_count7 #(.cy_period(7'b0011111))
    Counter (
        /* input        */ .clock(counter_clock),
        /* input        */ .reset(1'b0),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(count),
        /* output       */ .tc()
    );

    reg qsd0, qsd1, qsd2, qse0, qse1;
    assign qsd = {qsd1, qsd2};
    assign qse = {qse1, qse0};
    assign tx = settings[0];
    wire dividelower = settings[1];
    wire rxbit = dividelower ? count[4] : count[1];
    wire txbit = tx ? rxbit : 1'b0;

    always @(posedge counter_clock)
    begin
        if (count[2:0]==3'b0 || !dividelower )
        begin
            {qsd2, qsd1, qsd0} <= {qsd1, qsd0, rxbit};
            {qse1, qse0} <= {qse0, txbit};
        end
    end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
