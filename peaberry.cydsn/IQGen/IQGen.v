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

//`#start body` -- edit after this line, do not edit this line

module IQGen (clock, qsd, qse);
    input clock;
    output [1:0] qsd;
    output [1:0] qse;

    reg [1:0] qsdi;
    reg [1:0] qsd;
    reg [1:0] qse;
    reg [2:0] qsd_counter;
    reg [2:0] qse_counter;
    wire [7:0] settings;

    cy_psoc3_control # (.cy_init_value (8'b1001), .cy_force_order(1))
    Settings ( .control(settings) );
    
    always @(posedge clock)
    begin
        if (qsd_counter == settings[2:0])
        begin
            qsd_counter <= 3'b0;
            qsdi[0] <= ~qsdi[0];
        end
        else
        begin
            if (qsd_counter == {1'b0, settings[2:1]})
                qsdi[1] <= qsdi[0];
            qsd_counter <= qsd_counter + 1;
        end

        if (1'b1 == settings[3])
        begin
            qsd[1] <= qsdi[0];
            qsd[0] <= qsdi[1];
        end
        else
        begin
            qsd[1:0] <= qsdi[1:0];
        end
        
        if (1'b1 == settings[4])
        begin
            if (qse_counter == settings[2:0])
            begin
                qse_counter <= 3'b0;
                qse[1] <= ~qse[1];
            end
            else
            begin
                if (qse_counter == {1'b0, settings[2:1]})
                    qse[0] <= qse[1];
                qse_counter <= qse_counter + 1;
            end
        end
        else
        begin
            qse[1:0] <= 2'b00;
        end

    end

endmodule

//`#end` -- edit above this line, do not edit this line

//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line


//[] END OF FILE
