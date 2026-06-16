`timescale 1ns/1ps

module tb_poly_cpuid_rom;
  localparam logic [31:0] POLY_CPUID_BASE = 32'h40000000;
  localparam logic [31:0] POLY_CPUID_MAX = 32'h4000000a;

  logic valid_i;
  logic [31:0] leaf;
  logic [31:0] subleaf;
  logic hit;
  logic [31:0] eax;
  logic [31:0] ebx;
  logic [31:0] ecx;
  logic [31:0] edx;

  poly_cpuid_rom dut (
    .valid_i(valid_i),
    .leaf_i(leaf),
    .subleaf_i(subleaf),
    .hit_o(hit),
    .eax_o(eax),
    .ebx_o(ebx),
    .ecx_o(ecx),
    .edx_o(edx)
  );

  task automatic set_query(input logic [31:0] query_leaf,
                           input logic [31:0] query_subleaf);
    begin
      valid_i = 1'b1;
      leaf = query_leaf;
      subleaf = query_subleaf;
      #1;
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

  task automatic check_regs(input logic [31:0] exp_eax,
                            input logic [31:0] exp_ebx,
                            input logic [31:0] exp_ecx,
                            input logic [31:0] exp_edx,
                            input [1023:0] message);
    begin
      check(hit, message);
      check(eax == exp_eax, "unexpected eax");
      check(ebx == exp_ebx, "unexpected ebx");
      check(ecx == exp_ecx, "unexpected ecx");
      check(edx == exp_edx, "unexpected edx");
    end
  endtask

  initial begin
    valid_i = 1'b0;
    leaf = 32'd0;
    subleaf = 32'd0;
    #1;
    check(!hit && eax == 32'd0 && ebx == 32'd0 && ecx == 32'd0 && edx == 32'd0,
      "idle query misses");

    set_query(POLY_CPUID_BASE, 32'd0);
    check_regs(POLY_CPUID_MAX, 32'h796c6f50, 32'h21555043, 32'h746f6c67,
      "vendor leaf");

    set_query(POLY_CPUID_BASE + 32'd1, 32'd0);
    check_regs(32'd1, 32'h00000007, 32'hbe3fffff, 32'd20,
      "feature leaf");

    set_query(POLY_CPUID_BASE + 32'd2, 32'd31);
    check_regs(32'h00000044, 32'h00000045, 32'h00000071, 32'h00000072,
      "x86 v2 event/spill control leaf");

    set_query(POLY_CPUID_BASE + 32'd2, 32'd32);
    check_regs(32'h00fc3a0f, 32'd3, 32'd4, 32'd3,
      "x86 opcode geometry leaf");

    set_query(POLY_CPUID_BASE + 32'd2, 32'd33);
    check_regs(32'd1, 32'h0000003f, 32'd1, 32'd0,
      "x86 opcode contract leaf");

    set_query(POLY_CPUID_BASE + 32'd2, 32'd34);
    check_regs(32'h00000046, 32'h00000047, 32'h00000048, 32'd0,
      "x86 auto-spill profiling leaf");

    set_query(POLY_CPUID_BASE + 32'd2, 32'd0);
    check(!hit && eax == 32'd0 && ebx == 32'd0 && ecx == 32'd0 && edx == 32'd0,
      "invalid escape subleaf misses");

    set_query(POLY_CPUID_BASE + 32'd3, 32'd0);
    check_regs(32'h00ffbf61, 32'd0, 32'd20, 32'h00002000,
      "state leaf");

    set_query(POLY_CPUID_BASE + 32'd4, 32'd0);
    check_regs(32'd20, 32'h00002000, 32'h0040000c, 32'h0001ff7c,
      "arch xsave leaf");

    set_query(POLY_CPUID_BASE + 32'd4, 32'd1);
    check_regs(32'h00000000, 32'h00000040, 32'h31594c50, 32'd12,
      "xsave header leaf");

    set_query(POLY_CPUID_BASE + 32'd4, 32'd4);
    check_regs(32'h00000200, 32'h00000200, 32'd32, 32'd16,
      "aarch64 fp bank leaf");

    set_query(POLY_CPUID_BASE + 32'd4, 32'd7);
    check_regs(32'h00000580, 32'h00000200, 32'd32, 32'd16,
      "riscv fp bank leaf");

    set_query(POLY_CPUID_BASE + 32'd4, 32'd14);
    check_regs(32'h00001800, 32'h00000280, 32'd8, 32'h00000040,
      "native return xsave leaf");

    set_query(POLY_CPUID_BASE + 32'd4, 32'd99);
    check(!hit && eax == 32'd0 && ebx == 32'd0 && ecx == 32'd0 && edx == 32'd0,
      "invalid xsave subleaf misses");

    set_query(POLY_CPUID_BASE + 32'd5, 32'd0);
    check_regs(32'd2, 32'h00000040, 32'd8, 32'h0000007f,
      "trap packet leaf");

    set_query(POLY_CPUID_BASE + 32'd6, 32'd0);
    check_regs(32'd1, 32'h0000001f, 32'h0000000f, 32'h00000006,
      "interrupt leaf");

    set_query(POLY_CPUID_BASE + 32'd7, 32'd0);
    check_regs(32'd1, 32'd1, 32'h0000001f, 32'h00000006,
      "memory model leaf");

    set_query(POLY_CPUID_BASE + 32'd8, 32'd0);
    check_regs(32'd1, 32'h00000fff, 32'h00020004, 32'h00000007,
      "transition contract leaf");

    set_query(POLY_CPUID_BASE + 32'd8, 32'd1);
    check_regs(32'd0, 32'd1, 32'd2, 32'h00000007,
      "frontend id leaf");

    set_query(POLY_CPUID_BASE + 32'd8, 32'd5);
    check_regs(32'h00001800, 32'h00000280, 32'd8, 32'h00000040,
      "transition native return leaf");

    set_query(POLY_CPUID_BASE + 32'd8, 32'd99);
    check(!hit, "invalid transition subleaf misses");

    set_query(POLY_CPUID_BASE + 32'd9, 32'd0);
    check_regs(32'd1, 32'h00007e9f, 32'h00100808, 32'd0,
      "abi bridge leaf");

    set_query(POLY_CPUID_BASE + 32'd10, 32'd0);
    check_regs(32'd2, 32'h000001ff, 32'h00000023, 32'h01000200,
      "v2 abi leaf");

    set_query(POLY_CPUID_BASE + 32'd11, 32'd0);
    check(!hit && eax == 32'd0 && ebx == 32'd0 && ecx == 32'd0 && edx == 32'd0,
      "unsupported leaf misses");

    $display("POLY_RTL_CPUID_ROM_SIM_OK");
    $finish;
  end
endmodule
