// Minimal Poly control decoder for FPGA/silicon bring-up.
//
// This module only recognizes fixed-latency Poly control instructions. It does
// not read memory, parse descriptors, translate syscalls, or repack ABI state.
module poly_ctrl_decode (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [31:0] insn_i,
    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
    output logic        call_sig_imm_o,
    output logic [6:0]  signature_slot_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;
  localparam logic [6:0] POLY_ABI_SIGNATURE_SLOT_COUNT = 7'd13;

  localparam logic [7:0] POLY_X86_CTRL_PREFIX_0 = 8'h0f;
  localparam logic [7:0] POLY_X86_CTRL_PREFIX_1 = 8'h3a;
  localparam logic [7:0] POLY_X86_CTRL_PREFIX_2 = 8'hfc;
  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_IMM_BASE = 7'h30;

  localparam logic [31:0] POLY_AARCH64_CTRL_BASE = 32'hd503201f;
  localparam logic [31:0] POLY_AARCH64_CTRL_MASK = 32'hfffff01f;
  localparam logic [6:0]  POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE = 7'h50;

  localparam logic [31:0] POLY_RISCV_CTRL_BASE = 32'h0000700b;
  localparam logic [31:0] POLY_RISCV_CTRL_MASK = 32'h01ffffff;
  localparam logic [6:0]  POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE = 7'h20;

  logic is_x86_ctrl;
  logic is_aarch64_ctrl;
  logic is_riscv_ctrl;
  logic [6:0] decoded_subop;

  always_comb begin
    is_x86_ctrl =
      frontend_i == POLY_FRONTEND_X86 &&
      insn_i[7:0] == POLY_X86_CTRL_PREFIX_0 &&
      insn_i[15:8] == POLY_X86_CTRL_PREFIX_1 &&
      insn_i[23:16] == POLY_X86_CTRL_PREFIX_2 &&
      insn_i[31] == 1'b0;
    is_aarch64_ctrl =
      frontend_i == POLY_FRONTEND_AARCH64 &&
      (insn_i & POLY_AARCH64_CTRL_MASK) == POLY_AARCH64_CTRL_BASE;
    is_riscv_ctrl =
      frontend_i == POLY_FRONTEND_RISCV &&
      (insn_i & POLY_RISCV_CTRL_MASK) == POLY_RISCV_CTRL_BASE;

    decoded_subop = 7'd0;
    if (is_x86_ctrl)
      decoded_subop = insn_i[30:24];
    else if (is_aarch64_ctrl)
      decoded_subop = insn_i[11:5];
    else if (is_riscv_ctrl)
      decoded_subop = insn_i[31:25];

    poly_ctrl_o = valid_i && (is_x86_ctrl || is_aarch64_ctrl || is_riscv_ctrl);
    subop_o = poly_ctrl_o ? decoded_subop : 7'd0;
    call_sig_imm_o =
      poly_ctrl_o &&
      ((is_x86_ctrl &&
         decoded_subop >= POLY_X86_CTRL_PCALL_SIG_IMM_BASE &&
         decoded_subop < POLY_X86_CTRL_PCALL_SIG_IMM_BASE +
           POLY_ABI_SIGNATURE_SLOT_COUNT) ||
       (is_aarch64_ctrl &&
         decoded_subop >= POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE &&
         decoded_subop < POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE +
           POLY_ABI_SIGNATURE_SLOT_COUNT) ||
       (is_riscv_ctrl &&
         decoded_subop >= POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE &&
         decoded_subop < POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE +
           POLY_ABI_SIGNATURE_SLOT_COUNT));
    signature_slot_o = call_sig_imm_o ?
      (is_x86_ctrl ? decoded_subop - POLY_X86_CTRL_PCALL_SIG_IMM_BASE :
       is_aarch64_ctrl ?
         decoded_subop - POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE :
         decoded_subop - POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE) :
      7'd0;
  end
endmodule
