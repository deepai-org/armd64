`timescale 1ns/1ps

module tb_poly_transition_stack;
  localparam int DEPTH = 8;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic clk;
  logic rst_n;
  logic push;
  logic [1:0] push_frontend;
  logic [63:0] push_pc;
  logic [63:0] push_sp;
  logic [31:0] push_flags;
  logic pop;

  logic pop_valid;
  logic [1:0] pop_frontend;
  logic [63:0] pop_pc;
  logic [63:0] pop_sp;
  logic [31:0] pop_flags;
  logic peek_valid;
  logic [1:0] peek_frontend;
  logic [63:0] peek_pc;
  logic [63:0] peek_sp;
  logic [31:0] peek_flags;
  logic empty;
  logic full;
  logic overflow;
  logic underflow;
  logic conflict;
  logic [3:0] depth;

  poly_transition_stack #(
    .DEPTH(DEPTH)
  ) dut (
    .clk_i(clk),
    .rst_ni(rst_n),
    .push_i(push),
    .push_frontend_i(push_frontend),
    .push_pc_i(push_pc),
    .push_sp_i(push_sp),
    .push_flags_i(push_flags),
    .pop_i(pop),
    .pop_valid_o(pop_valid),
    .pop_frontend_o(pop_frontend),
    .pop_pc_o(pop_pc),
    .pop_sp_o(pop_sp),
    .pop_flags_o(pop_flags),
    .peek_valid_o(peek_valid),
    .peek_frontend_o(peek_frontend),
    .peek_pc_o(peek_pc),
    .peek_sp_o(peek_sp),
    .peek_flags_o(peek_flags),
    .empty_o(empty),
    .full_o(full),
    .overflow_o(overflow),
    .underflow_o(underflow),
    .conflict_o(conflict),
    .depth_o(depth)
  );

  always #5 clk = ~clk;

  function automatic logic [1:0] frontend_for(input int index);
    begin
      case (index % 3)
        0: frontend_for = POLY_FRONTEND_X86;
        1: frontend_for = POLY_FRONTEND_AARCH64;
        default: frontend_for = POLY_FRONTEND_RISCV;
      endcase
    end
  endfunction

  task automatic clear_inputs;
    begin
      push = 1'b0;
      push_frontend = 2'd0;
      push_pc = 64'd0;
      push_sp = 64'd0;
      push_flags = 32'd0;
      pop = 1'b0;
    end
  endtask

  task automatic check(input logic condition, input [1023:0] message);
    begin
      if (!condition) begin
        $display("FAIL: %0s", message);
        $fatal;
      end
    end
  endtask

  task automatic push_frame(input int index);
    begin
      @(negedge clk);
      push = 1'b1;
      push_frontend = frontend_for(index);
      push_pc = 64'h1000 + 64'(index * 4);
      push_sp = 64'h8000 - 64'(index * 16);
      push_flags = 32'(index);
      @(posedge clk);
      #1;
      check(!overflow && !underflow && !conflict, "push has no error");
      check(depth == 4'(index + 1), "push increments depth");
      check(peek_valid, "push updates peek valid");
      check(peek_frontend == frontend_for(index), "push updates peek frontend");
      check(peek_pc == 64'h1000 + 64'(index * 4), "push updates peek pc");
      check(peek_sp == 64'h8000 - 64'(index * 16), "push updates peek sp");
      check(peek_flags == 32'(index), "push updates peek flags");
      clear_inputs();
    end
  endtask

  task automatic pop_frame(input int index);
    begin
      @(negedge clk);
      pop = 1'b1;
      @(posedge clk);
      #1;
      check(pop_valid, "pop valid");
      check(!overflow && !underflow && !conflict, "pop has no error");
      check(pop_frontend == frontend_for(index), "pop frontend");
      check(pop_pc == 64'h1000 + 64'(index * 4), "pop pc");
      check(pop_sp == 64'h8000 - 64'(index * 16), "pop sp");
      check(pop_flags == 32'(index), "pop flags");
      check(depth == 4'(index), "pop decrements depth");
      clear_inputs();
    end
  endtask

  initial begin
    clk = 1'b0;
    rst_n = 1'b1;
    clear_inputs();

    #1 rst_n = 1'b0;
    #2;
    check(empty, "reset empty");
    check(!full, "reset not full");
    check(!peek_valid, "reset no peek");
    check(depth == 4'd0, "reset depth");

    #12 rst_n = 1'b1;

    for (int i = 0; i < DEPTH; i++) begin
      push_frame(i);
    end

    check(full, "full after depth pushes");
    check(depth == 4'(DEPTH), "depth at capacity");

    @(negedge clk);
    push = 1'b1;
    push_frontend = POLY_FRONTEND_X86;
    push_pc = 64'h2000;
    push_sp = 64'h7000;
    push_flags = 32'hdeadbeef;
    @(posedge clk);
    #1;
    check(overflow, "overflow when full");
    check(!pop_valid && !underflow && !conflict, "overflow only");
    check(depth == 4'(DEPTH), "overflow preserves depth");
    clear_inputs();

    for (int i = DEPTH - 1; i >= 0; i--) begin
      pop_frame(i);
    end

    check(empty, "empty after pops");
    check(!peek_valid, "no peek after empty");

    @(negedge clk);
    pop = 1'b1;
    @(posedge clk);
    #1;
    check(underflow, "underflow when empty");
    check(!pop_valid && !overflow && !conflict, "underflow only");
    clear_inputs();

    push_frame(0);

    @(negedge clk);
    push = 1'b1;
    push_frontend = POLY_FRONTEND_AARCH64;
    push_pc = 64'h3000;
    push_sp = 64'h9000;
    push_flags = 32'h1234;
    pop = 1'b1;
    @(posedge clk);
    #1;
    check(conflict, "push pop conflict");
    check(!pop_valid && !overflow && !underflow, "conflict only");
    check(depth == 4'd1, "conflict preserves depth");
    check(peek_frontend == POLY_FRONTEND_X86, "conflict preserves top frontend");
    check(peek_pc == 64'h1000, "conflict preserves top pc");

    $display("POLY_RTL_TRANSITION_STACK_SIM_OK");
    $finish;
  end
endmodule
