`timescale 1ns/1ps

module tb_poly_frontend_state;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [2:0] POLY_REDIRECT_INIT      = 3'd1;
  localparam logic [2:0] POLY_REDIRECT_COMMIT    = 3'd2;
  localparam logic [2:0] POLY_REDIRECT_INTERRUPT = 3'd3;
  localparam logic [2:0] POLY_REDIRECT_RETURN    = 3'd4;
  localparam logic [2:0] POLY_REDIRECT_TRAP      = 3'd5;
  localparam logic [2:0] POLY_REDIRECT_TRAP_RETURN = 3'd6;

  logic clk;
  logic rst_n;
  logic init;
  logic [1:0] init_frontend;
  logic [63:0] init_pc;
  logic commit;
  logic [1:0] commit_frontend;
  logic [63:0] commit_pc;
  logic interrupt_restore;
  logic [1:0] interrupt_frontend;
  logic [63:0] interrupt_pc;
  logic return_resume;
  logic [1:0] return_frontend;
  logic [63:0] return_pc;
  logic trap_vector;
  logic [1:0] trap_frontend;
  logic [63:0] trap_pc;
  logic trap_return;
  logic [1:0] trap_return_frontend;
  logic [63:0] trap_return_pc;
  logic fault;
  logic stall;

  logic [1:0] current_frontend;
  logic [63:0] current_pc;
  logic redirect_valid;
  logic [1:0] redirect_frontend;
  logic [63:0] redirect_pc;
  logic [2:0] redirect_reason;
  logic update;
  logic hold;
  logic conflict;
  logic invalid_frontend;
  logic invalid_pc;
  logic error;

  poly_frontend_state dut (
    .clk_i(clk),
    .rst_ni(rst_n),
    .init_i(init),
    .init_frontend_i(init_frontend),
    .init_pc_i(init_pc),
    .commit_i(commit),
    .commit_frontend_i(commit_frontend),
    .commit_pc_i(commit_pc),
    .interrupt_restore_i(interrupt_restore),
    .interrupt_frontend_i(interrupt_frontend),
    .interrupt_pc_i(interrupt_pc),
    .return_resume_i(return_resume),
    .return_frontend_i(return_frontend),
    .return_pc_i(return_pc),
    .trap_vector_i(trap_vector),
    .trap_frontend_i(trap_frontend),
    .trap_pc_i(trap_pc),
    .trap_return_i(trap_return),
    .trap_return_frontend_i(trap_return_frontend),
    .trap_return_pc_i(trap_return_pc),
    .fault_i(fault),
    .stall_i(stall),
    .current_frontend_o(current_frontend),
    .current_pc_o(current_pc),
    .redirect_valid_o(redirect_valid),
    .redirect_frontend_o(redirect_frontend),
    .redirect_pc_o(redirect_pc),
    .redirect_reason_o(redirect_reason),
    .update_o(update),
    .hold_o(hold),
    .conflict_o(conflict),
    .invalid_frontend_o(invalid_frontend),
    .invalid_pc_o(invalid_pc),
    .error_o(error)
  );

  always #5 clk = ~clk;

  task automatic clear_inputs;
    begin
      init = 1'b0;
      init_frontend = 2'd0;
      init_pc = 64'd0;
      commit = 1'b0;
      commit_frontend = 2'd0;
      commit_pc = 64'd0;
      interrupt_restore = 1'b0;
      interrupt_frontend = 2'd0;
      interrupt_pc = 64'd0;
      return_resume = 1'b0;
      return_frontend = 2'd0;
      return_pc = 64'd0;
      trap_vector = 1'b0;
      trap_frontend = 2'd0;
      trap_pc = 64'd0;
      trap_return = 1'b0;
      trap_return_frontend = 2'd0;
      trap_return_pc = 64'd0;
      fault = 1'b0;
      stall = 1'b0;
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
    clk = 1'b0;
    rst_n = 1'b1;
    clear_inputs();

    #1 rst_n = 1'b0;
    #2;
    check(current_frontend == POLY_FRONTEND_X86, "reset frontend");
    check(current_pc == 64'd0, "reset pc");

    #12 rst_n = 1'b1;
    @(negedge clk);

    init = 1'b1;
    init_frontend = POLY_FRONTEND_X86;
    init_pc = 64'h1000;
    #1;
    check(update && redirect_valid, "init redirects");
    check(redirect_frontend == POLY_FRONTEND_X86, "init redirect frontend");
    check(redirect_pc == 64'h1000, "init redirect pc");
    check(redirect_reason == POLY_REDIRECT_INIT, "init redirect reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_X86, "init committed frontend");
    check(current_pc == 64'h1000, "init committed pc");
    clear_inputs();

    @(negedge clk);
    commit = 1'b1;
    commit_frontend = POLY_FRONTEND_AARCH64;
    commit_pc = 64'h4000;
    #1;
    check(update && redirect_valid, "commit redirects");
    check(redirect_frontend == POLY_FRONTEND_AARCH64, "commit redirect frontend");
    check(redirect_pc == 64'h4000, "commit redirect pc");
    check(redirect_reason == POLY_REDIRECT_COMMIT, "commit redirect reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_AARCH64, "commit frontend state");
    check(current_pc == 64'h4000, "commit pc state");
    clear_inputs();

    @(negedge clk);
    commit = 1'b1;
    commit_frontend = POLY_FRONTEND_AARCH64;
    commit_pc = 64'h4002;
    #1;
    check(error && invalid_pc, "bad aarch64 alignment faults");
    check(!update && !redirect_valid, "bad alignment blocks redirect");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_AARCH64, "bad alignment holds frontend");
    check(current_pc == 64'h4000, "bad alignment holds pc");
    clear_inputs();

    @(negedge clk);
    commit = 1'b1;
    commit_frontend = POLY_FRONTEND_RISCV;
    commit_pc = 64'h8000;
    stall = 1'b1;
    #1;
    check(hold && !update && !redirect_valid, "stall blocks update");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_AARCH64, "stall holds frontend");
    check(current_pc == 64'h4000, "stall holds pc");
    clear_inputs();

    @(negedge clk);
    interrupt_restore = 1'b1;
    interrupt_frontend = POLY_FRONTEND_RISCV;
    interrupt_pc = 64'h8000;
    #1;
    check(update && redirect_valid, "interrupt restore redirects");
    check(redirect_frontend == POLY_FRONTEND_RISCV, "interrupt redirect frontend");
    check(redirect_pc == 64'h8000, "interrupt redirect pc");
    check(redirect_reason == POLY_REDIRECT_INTERRUPT, "interrupt redirect reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_RISCV, "interrupt frontend state");
    check(current_pc == 64'h8000, "interrupt pc state");
    clear_inputs();

    @(negedge clk);
    return_resume = 1'b1;
    return_frontend = POLY_FRONTEND_X86;
    return_pc = 64'h1200;
    #1;
    check(update && redirect_valid, "return resume redirects");
    check(redirect_frontend == POLY_FRONTEND_X86, "return redirect frontend");
    check(redirect_pc == 64'h1200, "return redirect pc");
    check(redirect_reason == POLY_REDIRECT_RETURN, "return redirect reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_X86, "return frontend state");
    check(current_pc == 64'h1200, "return pc state");
    clear_inputs();

    @(negedge clk);
    trap_vector = 1'b1;
    trap_frontend = POLY_FRONTEND_AARCH64;
    trap_pc = 64'h6000;
    #1;
    check(update && redirect_valid, "trap vector redirects");
    check(redirect_frontend == POLY_FRONTEND_AARCH64, "trap redirect frontend");
    check(redirect_pc == 64'h6000, "trap redirect pc");
    check(redirect_reason == POLY_REDIRECT_TRAP, "trap redirect reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_AARCH64, "trap frontend state");
    check(current_pc == 64'h6000, "trap pc state");
    clear_inputs();

    @(negedge clk);
    trap_return = 1'b1;
    trap_return_frontend = POLY_FRONTEND_X86;
    trap_return_pc = 64'h1800;
    #1;
    check(update && redirect_valid, "trap return redirects");
    check(redirect_frontend == POLY_FRONTEND_X86, "trap return redirect frontend");
    check(redirect_pc == 64'h1800, "trap return redirect pc");
    check(redirect_reason == POLY_REDIRECT_TRAP_RETURN,
      "trap return redirect reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_X86, "trap return frontend state");
    check(current_pc == 64'h1800, "trap return pc state");
    clear_inputs();

    @(negedge clk);
    commit = 1'b1;
    commit_frontend = POLY_FRONTEND_AARCH64;
    commit_pc = 64'h4000;
    return_resume = 1'b1;
    return_frontend = POLY_FRONTEND_X86;
    return_pc = 64'h1300;
    #1;
    check(conflict && error, "same-cycle update conflict");
    check(!update && !redirect_valid, "conflict blocks redirect");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_X86, "conflict holds frontend");
    check(current_pc == 64'h1800, "conflict holds pc");
    clear_inputs();

    @(negedge clk);
    init = 1'b1;
    init_frontend = POLY_FRONTEND_X86;
    init_pc = 64'h2000;
    commit = 1'b1;
    commit_frontend = POLY_FRONTEND_AARCH64;
    commit_pc = 64'h4000;
    return_resume = 1'b1;
    return_frontend = POLY_FRONTEND_RISCV;
    return_pc = 64'h8000;
    #1;
    check(!conflict && update && redirect_valid, "init overrides conflict");
    check(redirect_frontend == POLY_FRONTEND_X86, "init override frontend");
    check(redirect_pc == 64'h2000, "init override pc");
    check(redirect_reason == POLY_REDIRECT_INIT, "init override reason");
    @(posedge clk);
    #1;
    check(current_frontend == POLY_FRONTEND_X86, "init override state frontend");
    check(current_pc == 64'h2000, "init override state pc");

    $display("POLY_RTL_FRONTEND_STATE_SIM_OK");
    $finish;
  end
endmodule
