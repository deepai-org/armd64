`timescale 1ns/1ps

module tb_poly_interrupt_boundary;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic valid;
  logic feature_enabled;
  logic cpl3;
  logic interrupt;
  logic user_return;
  logic [1:0] current_frontend;
  logic [63:0] current_pc;
  logic interrupted_valid;
  logic [1:0] interrupted_frontend;
  logic [63:0] interrupted_pc;
  logic [63:0] user_return_pc;

  logic enter_x86_interrupt;
  logic save_interrupted;
  logic [1:0] saved_frontend;
  logic [63:0] saved_pc;
  logic restore_raw;
  logic clear_interrupted;
  logic [1:0] next_frontend;
  logic [63:0] next_pc;
  logic error;
  logic invalid_current_frontend;
  logic invalid_current_pc;
  logic invalid_interrupted_frontend;
  logic invalid_interrupted_pc;

  poly_interrupt_boundary dut (
    .valid_i(valid),
    .feature_enabled_i(feature_enabled),
    .cpl3_i(cpl3),
    .interrupt_i(interrupt),
    .user_return_i(user_return),
    .current_frontend_i(current_frontend),
    .current_pc_i(current_pc),
    .interrupted_valid_i(interrupted_valid),
    .interrupted_frontend_i(interrupted_frontend),
    .interrupted_pc_i(interrupted_pc),
    .user_return_pc_i(user_return_pc),
    .enter_x86_interrupt_o(enter_x86_interrupt),
    .save_interrupted_o(save_interrupted),
    .saved_frontend_o(saved_frontend),
    .saved_pc_o(saved_pc),
    .restore_raw_o(restore_raw),
    .clear_interrupted_o(clear_interrupted),
    .next_frontend_o(next_frontend),
    .next_pc_o(next_pc),
    .error_o(error),
    .invalid_current_frontend_o(invalid_current_frontend),
    .invalid_current_pc_o(invalid_current_pc),
    .invalid_interrupted_frontend_o(invalid_interrupted_frontend),
    .invalid_interrupted_pc_o(invalid_interrupted_pc)
  );

  task automatic clear_inputs;
    begin
      valid = 1'b1;
      feature_enabled = 1'b1;
      cpl3 = 1'b1;
      interrupt = 1'b0;
      user_return = 1'b0;
      current_frontend = POLY_FRONTEND_X86;
      current_pc = 64'd0;
      interrupted_valid = 1'b0;
      interrupted_frontend = POLY_FRONTEND_X86;
      interrupted_pc = 64'd0;
      user_return_pc = 64'd0;
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

  initial begin
    clear_inputs();
    #1;
    check(!enter_x86_interrupt && !restore_raw && !error, "idle has no action");
    check(next_frontend == POLY_FRONTEND_X86, "idle next frontend");
    check(next_pc == 64'd0, "idle next pc");

    clear_inputs();
    interrupt = 1'b1;
    current_frontend = POLY_FRONTEND_AARCH64;
    current_pc = 64'h4000;
    #1;
    check(enter_x86_interrupt && save_interrupted, "raw interrupt enters x86");
    check(saved_frontend == POLY_FRONTEND_AARCH64, "raw interrupt saved frontend");
    check(saved_pc == 64'h4000, "raw interrupt saved pc");
    check(next_frontend == POLY_FRONTEND_X86, "raw interrupt next frontend");
    check(next_pc == 64'h4000, "raw interrupt next pc");
    check(!error, "raw interrupt no error");

    clear_inputs();
    interrupt = 1'b1;
    current_frontend = POLY_FRONTEND_X86;
    current_pc = 64'h1000;
    #1;
    check(!enter_x86_interrupt && !save_interrupted, "x86 interrupt not captured");
    check(!error, "x86 interrupt no error");
    check(next_frontend == POLY_FRONTEND_X86, "x86 interrupt frontend unchanged");
    check(next_pc == 64'h1000, "x86 interrupt pc unchanged");

    clear_inputs();
    cpl3 = 1'b0;
    interrupt = 1'b1;
    current_frontend = POLY_FRONTEND_RISCV;
    current_pc = 64'h8000;
    #1;
    check(!enter_x86_interrupt && !save_interrupted, "cpl0 raw interrupt ignored");
    check(!error, "cpl0 raw interrupt no error");

    clear_inputs();
    feature_enabled = 1'b0;
    interrupt = 1'b1;
    current_frontend = POLY_FRONTEND_RISCV;
    current_pc = 64'h8000;
    #1;
    check(!enter_x86_interrupt && !save_interrupted, "disabled raw interrupt ignored");
    check(!error, "disabled raw interrupt no error");

    clear_inputs();
    interrupt = 1'b1;
    current_frontend = POLY_FRONTEND_AARCH64;
    current_pc = 64'h4002;
    #1;
    check(error && invalid_current_pc, "bad current pc rejected");
    check(!enter_x86_interrupt && !save_interrupted, "bad current pc blocks entry");

    clear_inputs();
    interrupt = 1'b1;
    current_frontend = 2'd3;
    current_pc = 64'h1000;
    #1;
    check(error && invalid_current_frontend, "bad current frontend rejected");
    check(!enter_x86_interrupt && !save_interrupted, "bad current frontend blocks entry");

    clear_inputs();
    user_return = 1'b1;
    current_frontend = POLY_FRONTEND_X86;
    current_pc = 64'h4000;
    interrupted_valid = 1'b1;
    interrupted_frontend = POLY_FRONTEND_RISCV;
    interrupted_pc = 64'h8000;
    user_return_pc = 64'h8000;
    #1;
    check(restore_raw && clear_interrupted, "matching return restores raw");
    check(next_frontend == POLY_FRONTEND_RISCV, "return restore frontend");
    check(next_pc == 64'h8000, "return restore pc");
    check(!error, "return restore no error");

    clear_inputs();
    user_return = 1'b1;
    current_frontend = POLY_FRONTEND_X86;
    current_pc = 64'h4000;
    interrupted_valid = 1'b1;
    interrupted_frontend = POLY_FRONTEND_RISCV;
    interrupted_pc = 64'h8000;
    user_return_pc = 64'h8002;
    #1;
    check(!restore_raw && !clear_interrupted, "mismatched return does not restore");
    check(!error, "mismatched return no error");
    check(next_frontend == POLY_FRONTEND_X86, "mismatched return frontend unchanged");
    check(next_pc == 64'h4000, "mismatched return pc unchanged");

    clear_inputs();
    user_return = 1'b1;
    current_frontend = POLY_FRONTEND_X86;
    current_pc = 64'h4000;
    interrupted_valid = 1'b1;
    interrupted_frontend = POLY_FRONTEND_X86;
    interrupted_pc = 64'h4000;
    user_return_pc = 64'h4000;
    #1;
    check(error && invalid_interrupted_frontend, "interrupted x86 rejected");
    check(!restore_raw && !clear_interrupted, "invalid interrupted frontend blocks restore");

    clear_inputs();
    user_return = 1'b1;
    current_frontend = POLY_FRONTEND_X86;
    current_pc = 64'h4000;
    interrupted_valid = 1'b1;
    interrupted_frontend = POLY_FRONTEND_RISCV;
    interrupted_pc = 64'h0000800000000000;
    user_return_pc = 64'h0000800000000000;
    #1;
    check(error && invalid_interrupted_pc, "bad interrupted pc rejected");
    check(!restore_raw && !clear_interrupted, "bad interrupted pc blocks restore");

    clear_inputs();
    user_return = 1'b1;
    current_frontend = POLY_FRONTEND_X86;
    current_pc = 64'h4000;
    interrupted_valid = 1'b0;
    interrupted_frontend = POLY_FRONTEND_RISCV;
    interrupted_pc = 64'h8000;
    user_return_pc = 64'h8000;
    #1;
    check(!restore_raw && !clear_interrupted && !error, "return without saved raw ignored");

    $display("POLY_RTL_INTERRUPT_BOUNDARY_SIM_OK");
    $finish;
  end
endmodule
