`timescale 1ns/1ps

module tb_poly_return_cookie_recover;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;
  localparam logic [63:0] POLY_RETURN_COOKIE = 64'hfffffffffffff000;

  logic valid;
  logic [1:0] current_frontend;
  logic [63:0] return_target_pc;
  logic transition_empty;
  logic [1:0] pop_frontend;
  logic [63:0] pop_pc;
  logic [63:0] pop_sp;
  logic [31:0] pop_flags;

  logic cookie_hit;
  logic pop_transition;
  logic resume;
  logic [1:0] resume_frontend;
  logic [63:0] resume_pc;
  logic [63:0] resume_sp;
  logic [31:0] resume_flags;
  logic error;
  logic invalid_frontend;
  logic missing_transition;

  poly_return_cookie_recover dut (
    .valid_i(valid),
    .current_frontend_i(current_frontend),
    .return_target_pc_i(return_target_pc),
    .transition_empty_i(transition_empty),
    .pop_frontend_i(pop_frontend),
    .pop_pc_i(pop_pc),
    .pop_sp_i(pop_sp),
    .pop_flags_i(pop_flags),
    .cookie_hit_o(cookie_hit),
    .pop_transition_o(pop_transition),
    .resume_o(resume),
    .resume_frontend_o(resume_frontend),
    .resume_pc_o(resume_pc),
    .resume_sp_o(resume_sp),
    .resume_flags_o(resume_flags),
    .error_o(error),
    .invalid_frontend_o(invalid_frontend),
    .missing_transition_o(missing_transition)
  );

  task automatic clear_inputs;
    begin
      valid = 1'b1;
      current_frontend = POLY_FRONTEND_X86;
      return_target_pc = 64'd0;
      transition_empty = 1'b0;
      pop_frontend = POLY_FRONTEND_X86;
      pop_pc = 64'd0;
      pop_sp = 64'd0;
      pop_flags = 32'd0;
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
    current_frontend = POLY_FRONTEND_AARCH64;
    return_target_pc = 64'h4004;
    pop_frontend = POLY_FRONTEND_X86;
    pop_pc = 64'h1000;
    pop_sp = 64'h8000;
    pop_flags = 32'h55;
    #1;
    check(!cookie_hit && !pop_transition && !resume, "normal return no recovery");
    check(!error && !invalid_frontend && !missing_transition, "normal return no error");
    check(resume_frontend == POLY_FRONTEND_AARCH64, "normal return frontend unchanged");
    check(resume_pc == 64'h4004, "normal return pc unchanged");
    check(resume_sp == 64'd0, "normal return sp zero");
    check(resume_flags == 32'd0, "normal return flags zero");

    clear_inputs();
    current_frontend = POLY_FRONTEND_RISCV;
    return_target_pc = POLY_RETURN_COOKIE;
    transition_empty = 1'b0;
    pop_frontend = POLY_FRONTEND_X86;
    pop_pc = 64'h12345678;
    pop_sp = 64'h7fff0000;
    pop_flags = 32'h21;
    #1;
    check(cookie_hit && pop_transition && resume, "cookie hit resumes");
    check(!error && !invalid_frontend && !missing_transition, "cookie hit no error");
    check(resume_frontend == POLY_FRONTEND_X86, "cookie resume frontend");
    check(resume_pc == 64'h12345678, "cookie resume pc");
    check(resume_sp == 64'h7fff0000, "cookie resume sp");
    check(resume_flags == 32'h21, "cookie resume flags");

    clear_inputs();
    current_frontend = POLY_FRONTEND_AARCH64;
    return_target_pc = POLY_RETURN_COOKIE;
    transition_empty = 1'b1;
    pop_frontend = POLY_FRONTEND_X86;
    pop_pc = 64'h1000;
    pop_sp = 64'h8000;
    #1;
    check(cookie_hit && error && missing_transition, "missing transition error");
    check(!pop_transition && !resume, "missing transition blocks resume");

    clear_inputs();
    current_frontend = 2'd3;
    return_target_pc = POLY_RETURN_COOKIE;
    transition_empty = 1'b0;
    pop_frontend = POLY_FRONTEND_X86;
    #1;
    check(cookie_hit && error && invalid_frontend, "invalid current frontend error");
    check(!pop_transition && !resume, "invalid current blocks resume");

    clear_inputs();
    current_frontend = POLY_FRONTEND_RISCV;
    return_target_pc = POLY_RETURN_COOKIE;
    transition_empty = 1'b0;
    pop_frontend = 2'd3;
    #1;
    check(cookie_hit && error && invalid_frontend, "invalid pop frontend error");
    check(!pop_transition && !resume, "invalid pop blocks resume");

    clear_inputs();
    valid = 1'b0;
    current_frontend = POLY_FRONTEND_RISCV;
    return_target_pc = POLY_RETURN_COOKIE;
    transition_empty = 1'b0;
    pop_frontend = POLY_FRONTEND_X86;
    pop_pc = 64'h1000;
    pop_sp = 64'h8000;
    pop_flags = 32'h44;
    #1;
    check(!cookie_hit && !pop_transition && !resume, "invalid cycle gated");
    check(!error && !invalid_frontend && !missing_transition, "invalid cycle no error");
    check(resume_frontend == POLY_FRONTEND_RISCV, "invalid cycle frontend unchanged");
    check(resume_pc == POLY_RETURN_COOKIE, "invalid cycle pc unchanged");

    $display("POLY_RTL_RETURN_COOKIE_RECOVER_SIM_OK");
    $finish;
  end
endmodule
