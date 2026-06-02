`timescale 1ns/1ps

module tb_poly_ctrl_decode;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] X86_TRAP_RETURN     = 7'h04;
  localparam logic [6:0] X86_PCALL_SIG_BASE  = 7'h30;
  localparam logic [6:0] A64_TRAP_RETURN     = 7'h04;
  localparam logic [6:0] A64_PCALL_SIG_BASE  = 7'h50;
  localparam logic [6:0] RV_TRAP_RETURN      = 7'h04;
  localparam logic [6:0] RV_PCALL_SIG_BASE   = 7'h20;

  logic valid;
  logic [1:0] frontend;
  logic [31:0] insn;
  logic poly_ctrl;
  logic [6:0] subop;
  logic call_sig_imm;
  logic [6:0] signature_slot;

  poly_ctrl_decode dut (
    .valid_i(valid),
    .frontend_i(frontend),
    .insn_i(insn),
    .poly_ctrl_o(poly_ctrl),
    .subop_o(subop),
    .call_sig_imm_o(call_sig_imm),
    .signature_slot_o(signature_slot)
  );

  function automatic logic [31:0] x86_word(input logic [6:0] op);
    begin
      x86_word = {1'b0, op, 8'hfc, 8'h3a, 8'h0f};
    end
  endfunction

  function automatic logic [31:0] aarch64_word(input logic [6:0] op);
    begin
      aarch64_word = 32'hd503201f | ({25'd0, op} << 5);
    end
  endfunction

  function automatic logic [31:0] riscv_word(input logic [6:0] op);
    begin
      riscv_word = 32'h0000700b | ({25'd0, op} << 25);
    end
  endfunction

  task automatic check(input logic condition, input [1023:0] message);
    begin
      if (!condition) begin
        $display("FAIL: %0s", message);
        $fatal;
      end
    end
  endtask

  task automatic expect_decode(
      input logic [1:0] expected_frontend,
      input logic [31:0] word,
      input logic [6:0] expected_subop,
      input logic expected_call_sig,
      input logic [6:0] expected_slot
  );
    begin
      valid = 1'b1;
      frontend = expected_frontend;
      insn = word;
      #1;
      check(poly_ctrl, "expected control instruction");
      check(subop == expected_subop, "decoded subop");
      check(call_sig_imm == expected_call_sig, "call signature immediate flag");
      check(signature_slot == expected_slot, "signature slot");
    end
  endtask

  task automatic expect_miss(
      input logic expected_valid,
      input logic [1:0] expected_frontend,
      input logic [31:0] word
  );
    begin
      valid = expected_valid;
      frontend = expected_frontend;
      insn = word;
      #1;
      check(!poly_ctrl, "expected decoder miss");
      check(subop == 7'd0, "miss clears subop");
      check(!call_sig_imm, "miss clears signature flag");
      check(signature_slot == 7'd0, "miss clears signature slot");
    end
  endtask

  initial begin
    expect_decode(
      POLY_FRONTEND_X86, x86_word(X86_TRAP_RETURN),
      X86_TRAP_RETURN, 1'b0, 7'd0
    );
    expect_decode(
      POLY_FRONTEND_X86, x86_word(X86_PCALL_SIG_BASE + 7'd7),
      X86_PCALL_SIG_BASE + 7'd7, 1'b1, 7'd7
    );
    expect_decode(
      POLY_FRONTEND_X86, x86_word(X86_PCALL_SIG_BASE + 7'd12),
      X86_PCALL_SIG_BASE + 7'd12, 1'b1, 7'd12
    );
    expect_decode(
      POLY_FRONTEND_X86, x86_word(X86_PCALL_SIG_BASE + 7'd13),
      X86_PCALL_SIG_BASE + 7'd13, 1'b0, 7'd0
    );

    expect_decode(
      POLY_FRONTEND_AARCH64, aarch64_word(A64_TRAP_RETURN),
      A64_TRAP_RETURN, 1'b0, 7'd0
    );
    expect_decode(
      POLY_FRONTEND_AARCH64, aarch64_word(A64_PCALL_SIG_BASE + 7'd3),
      A64_PCALL_SIG_BASE + 7'd3, 1'b1, 7'd3
    );
    expect_decode(
      POLY_FRONTEND_AARCH64, aarch64_word(A64_PCALL_SIG_BASE + 7'd12),
      A64_PCALL_SIG_BASE + 7'd12, 1'b1, 7'd12
    );
    expect_decode(
      POLY_FRONTEND_AARCH64, aarch64_word(A64_PCALL_SIG_BASE + 7'd13),
      A64_PCALL_SIG_BASE + 7'd13, 1'b0, 7'd0
    );

    expect_decode(
      POLY_FRONTEND_RISCV, riscv_word(RV_TRAP_RETURN),
      RV_TRAP_RETURN, 1'b0, 7'd0
    );
    expect_decode(
      POLY_FRONTEND_RISCV, riscv_word(RV_PCALL_SIG_BASE + 7'd5),
      RV_PCALL_SIG_BASE + 7'd5, 1'b1, 7'd5
    );
    expect_decode(
      POLY_FRONTEND_RISCV, riscv_word(RV_PCALL_SIG_BASE + 7'd12),
      RV_PCALL_SIG_BASE + 7'd12, 1'b1, 7'd12
    );
    expect_decode(
      POLY_FRONTEND_RISCV, riscv_word(RV_PCALL_SIG_BASE + 7'd13),
      RV_PCALL_SIG_BASE + 7'd13, 1'b0, 7'd0
    );

    expect_miss(1'b0, POLY_FRONTEND_X86, x86_word(X86_TRAP_RETURN));
    expect_miss(1'b1, POLY_FRONTEND_X86, x86_word(X86_TRAP_RETURN) ^ 32'h00000100);
    expect_miss(1'b1, POLY_FRONTEND_X86, x86_word(7'h7f) | 32'h80000000);
    expect_miss(1'b1, POLY_FRONTEND_AARCH64, aarch64_word(A64_TRAP_RETURN) ^ 32'h1);
    expect_miss(1'b1, POLY_FRONTEND_RISCV, riscv_word(RV_TRAP_RETURN) ^ 32'h10);
    expect_miss(1'b1, 2'd3, x86_word(X86_TRAP_RETURN));

    $display("POLY_RTL_CTRL_DECODE_SIM_OK");
    $finish;
  end
endmodule
