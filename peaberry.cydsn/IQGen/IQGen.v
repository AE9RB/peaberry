
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

    wire [7:0] settings;
    cy_psoc3_control # (.cy_init_value(8'b0), .cy_force_order(1))
    Settings ( .control(settings) );

    wire [6:0] count;
    cy_psoc3_count7 #(.cy_period(7'b0011111))
    Counter (
        /* input        */ .clock(clock),
        /* input        */ .reset(1'b0),
        /* input        */ .load(1'b0),
        /* input        */ .enable(1'b1),
        /* output [6:0] */ .count(count),
        /* output       */ .tc()
    );

    // This unusual construct is my attempt to reserve a whole byte
    // in order to keep anyone else from routing signals nearby.
    reg [7:0] data;
    localparam qsd1 = 1;
    localparam qsd0 = 2;
    localparam qse1 = 6;
    localparam qse0 = 5;

    assign qsd = {data[qsd1], data[qsd0]};
    assign qse = {data[qse1], data[qse0]};
    assign tx = settings[1] == 1'b1;

    wire dividelower = settings[0] == 1'b1;
    wire runlower = count[2:0]==3'b0;
    wire execute = !dividelower || runlower;
    wire rxbit = dividelower ? count[4] : count[1];
    wire txbit = tx ? nextrxbit : 1'b0;
    
    always @(posedge clock)
    begin
        if (execute)
        begin
            {data[qsd1], data[qsd0]} <= {rxbit, data[qsd1]};
            {data[qse1], data[qse0]} <= {txbit, data[qse1]};
        end
    end

//`#end` -- edit above this line, do not edit this line
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line
