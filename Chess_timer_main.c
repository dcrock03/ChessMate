module chess_timer (
    input wire clk,           // System clock
    input wire rst,           // Reset signal
    input wire switch_player, // Button to switch between players
    output reg [15:0] display_time, // Time to display (in seconds)
    output reg active_player  // 0 for player 1, 1 for player 2
);

    // Parameters for initial time (15 minutes = 900 seconds)
    parameter INITIAL_TIME = 16'd900;

    // 1 Hz clock divider (for a 50 MHz system clock)
    reg [25:0] clk_divider;
    wire clk_1hz;

    // Internal signals
    reg [15:0] time_player1, time_player2;
    reg timer_enable;
    reg [2:0] debounce_counter;
    reg switch_player_debounced;

    // Generate 1 Hz clock
    always @(posedge clk or posedge rst) begin
        if (rst)
            clk_divider <= 26'd0;
        else if (clk_divider == 26'd49999999)
            clk_divider <= 26'd0;
        else
            clk_divider <= clk_divider + 1'b1;
    end

    assign clk_1hz = (clk_divider == 26'd49999999); // creates a signal that turns on for one clock cycle every second by checking if a counter has reached a specific value that represents one second's worth of faster clock cycles
    //just to make it more efficient since having this 1 Hz clock signal allows you to accurately measure and update time in one-second intervals without needing to count every single high speed clock cycle
    
    // Debounce switch_player input
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            debounce_counter <= 3'd0;
            switch_player_debounced <= 1'b0;
        end else if (switch_player) begin
            if (debounce_counter == 3'd7)
                switch_player_debounced <= 1'b1;
            else
                debounce_counter <= debounce_counter + 1'b1;
        end else begin
            debounce_counter <= 3'd0;
            switch_player_debounced <= 1'b0;
        end
    end
// Main timer logic
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            time_player1 <= INITIAL_TIME;
            time_player2 <= INITIAL_TIME;
            active_player <= 1'b0;
            timer_enable <= 1'b0;
        end else if (switch_player_debounced) begin
            active_player <= ~active_player;
            timer_enable <= 1'b1;
        end else if (clk_1hz && timer_enable) begin
            if (active_player == 1'b0 && time_player1 > 16'd0)
                time_player1 <= time_player1 - 1'b1;
            else if (active_player == 1'b1 && time_player2 > 16'd0)
                time_player2 <= time_player2 - 1'b1;
        end
    end

    // Display logic
    always @* begin
        display_time = (active_player == 1'b0) ? time_player1 : time_player2;
    end

endmodule
