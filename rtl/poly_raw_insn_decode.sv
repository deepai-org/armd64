// Raw foreign instruction class decoder for FPGA/silicon bring-up.
//
// This is not a full executor. It classifies already-fetched AArch64/RISC-V
// instructions into hardware-relevant sidebands so the frontend can apply
// ordering, branch/return, and trap policy without emulator-only decoding.
module poly_raw_insn_decode (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic [31:0] insn_i,

    output logic        raw_insn_valid_o,
    output logic        memory_order_valid_o,
    output logic        memory_load_o,
    output logic        memory_store_o,
    output logic        memory_atomic_o,
    output logic        memory_barrier_o,
    output logic        branch_o,
    output logic        call_o,
    output logic        return_o,
    output logic        trap_o,
    output logic        branch_target_valid_o,
    output logic [63:0] branch_target_o
);
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic aarch64_valid;
  logic riscv_valid;
  logic a64_load_store_group;
  logic a64_atomic_group;
  logic a64_barrier;
  logic a64_bl;
  logic a64_b;
  logic a64_cond_b;
  logic a64_br;
  logic a64_blr;
  logic a64_ret;
  logic a64_trap;
  logic riscv_32;
  logic [6:0] rv_opcode;
  logic [4:0] rv_rd;
  logic [15:0] rv16;
  logic [2:0] rv16_funct3;
  logic [1:0] rv16_quad;
  logic rv_load;
  logic rv_store;
  logic rv_atomic;
  logic rv_barrier;
  logic rv_branch;
  logic rv_call;
  logic rv_return;
  logic rv_trap;
  logic rv32_uncond_direct;
  logic rv16_uncond_direct;
  logic [63:0] a64_b_offset;
  logic [63:0] rv_jal_offset;
  logic [63:0] rv16_j_offset;

  always_comb begin
    aarch64_valid = valid_i && frontend_i == POLY_FRONTEND_AARCH64;
    riscv_valid = valid_i && frontend_i == POLY_FRONTEND_RISCV;

    a64_load_store_group = insn_i[27:25] == 3'b100;
    a64_atomic_group = a64_load_store_group && insn_i[29:24] == 6'b001000;
    a64_barrier = (insn_i & 32'hfffff01f) == 32'hd503301f;
    a64_bl = (insn_i & 32'hfc000000) == 32'h94000000;
    a64_b = (insn_i & 32'h7c000000) == 32'h14000000;
    a64_cond_b = (insn_i & 32'hff000010) == 32'h54000000;
    a64_br = (insn_i & 32'hfffffc1f) == 32'hd61f0000;
    a64_blr = (insn_i & 32'hfffffc1f) == 32'hd63f0000;
    a64_ret = (insn_i & 32'hfffffc1f) == 32'hd65f0000;
    a64_trap = (insn_i & 32'hff000000) == 32'hd4000000;
    a64_b_offset = {{36{insn_i[25]}}, insn_i[25:0], 2'b00};

    riscv_32 = insn_i[1:0] == 2'b11;
    rv_opcode = insn_i[6:0];
    rv_rd = insn_i[11:7];
    rv16 = insn_i[15:0];
    rv16_funct3 = rv16[15:13];
    rv16_quad = rv16[1:0];

    rv_load =
      (riscv_32 && (rv_opcode == 7'h03 || rv_opcode == 7'h07)) ||
      (!riscv_32 &&
        ((rv16_quad == 2'b00 &&
           (rv16_funct3 == 3'b001 || rv16_funct3 == 3'b010 ||
            rv16_funct3 == 3'b011)) ||
         (rv16_quad == 2'b10 &&
           (rv16_funct3 == 3'b001 || rv16_funct3 == 3'b010 ||
            rv16_funct3 == 3'b011))));
    rv_store =
      (riscv_32 && (rv_opcode == 7'h23 || rv_opcode == 7'h27)) ||
      (!riscv_32 &&
        ((rv16_quad == 2'b00 &&
           (rv16_funct3 == 3'b101 || rv16_funct3 == 3'b110 ||
            rv16_funct3 == 3'b111)) ||
         (rv16_quad == 2'b10 &&
           (rv16_funct3 == 3'b101 || rv16_funct3 == 3'b110 ||
            rv16_funct3 == 3'b111))));
    rv_atomic = riscv_32 && rv_opcode == 7'h2f;
    rv_barrier = riscv_32 && rv_opcode == 7'h0f;
    rv_branch =
      (riscv_32 &&
        (rv_opcode == 7'h63 || rv_opcode == 7'h67 || rv_opcode == 7'h6f)) ||
      (!riscv_32 &&
        ((rv16_quad == 2'b01 &&
           (rv16_funct3 == 3'b001 || rv16_funct3 == 3'b101 ||
            rv16_funct3 == 3'b110 || rv16_funct3 == 3'b111)) ||
         (rv16_quad == 2'b10 && rv16[15:12] == 4'b1000 &&
          rv16[6:2] == 5'd0)));
    rv_call =
      (riscv_32 &&
        ((rv_opcode == 7'h6f && (rv_rd == 5'd1 || rv_rd == 5'd5)) ||
         (rv_opcode == 7'h67 && (rv_rd == 5'd1 || rv_rd == 5'd5)))) ||
      (!riscv_32 && rv16_quad == 2'b10 && rv16[15:12] == 4'b1001 &&
        rv16[11:7] != 5'd0 && rv16[6:2] == 5'd0);
    rv_return = riscv_32 && insn_i == 32'h00008067;
    rv_trap =
      (riscv_32 && rv_opcode == 7'h73) ||
      (!riscv_32 && rv16 == 16'h9002);
    rv32_uncond_direct = riscv_32 && rv_opcode == 7'h6f;
    rv16_uncond_direct =
      !riscv_32 && rv16_quad == 2'b01 &&
      (rv16_funct3 == 3'b001 || rv16_funct3 == 3'b101);
    rv_jal_offset = {
      {43{insn_i[31]}}, insn_i[31], insn_i[19:12], insn_i[20],
      insn_i[30:21], 1'b0
    };
    rv16_j_offset = {
      {52{rv16[12]}}, rv16[12], rv16[8], rv16[10:9], rv16[6],
      rv16[7], rv16[2], rv16[11], rv16[5:3], 1'b0
    };

    raw_insn_valid_o = aarch64_valid || riscv_valid;

    memory_load_o =
      (aarch64_valid && a64_load_store_group && !a64_atomic_group &&
        insn_i[22]) ||
      (riscv_valid && rv_load);
    memory_store_o =
      (aarch64_valid && a64_load_store_group && !a64_atomic_group &&
        !insn_i[22]) ||
      (riscv_valid && rv_store);
    memory_atomic_o =
      (aarch64_valid && a64_atomic_group) ||
      (riscv_valid && rv_atomic);
    memory_barrier_o =
      (aarch64_valid && a64_barrier) ||
      (riscv_valid && rv_barrier);
    memory_order_valid_o =
      raw_insn_valid_o &&
      (memory_load_o || memory_store_o || memory_atomic_o || memory_barrier_o);

    call_o = (aarch64_valid && (a64_bl || a64_blr)) || (riscv_valid && rv_call);
    return_o = (aarch64_valid && a64_ret) || (riscv_valid && rv_return);
    branch_o =
      (aarch64_valid &&
        (a64_b || a64_cond_b || a64_br || a64_bl || a64_blr || a64_ret)) ||
      (riscv_valid && rv_branch);
    trap_o = (aarch64_valid && a64_trap) || (riscv_valid && rv_trap);

    branch_target_valid_o =
      (aarch64_valid && (a64_b || a64_bl)) ||
      (riscv_valid && (rv32_uncond_direct || rv16_uncond_direct));
    branch_target_o = 64'd0;
    if (branch_target_valid_o) begin
      if (aarch64_valid && (a64_b || a64_bl))
        branch_target_o = pc_i + a64_b_offset;
      else if (riscv_valid && riscv_32 && rv_opcode == 7'h6f)
        branch_target_o = pc_i + rv_jal_offset;
      else
        branch_target_o = pc_i + rv16_j_offset;
    end
  end
endmodule
