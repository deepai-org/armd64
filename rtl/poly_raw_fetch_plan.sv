// Raw foreign frontend fetch geometry for Poly FPGA/silicon bring-up.
//
// This block does not access memory or decode full instructions. It describes
// the fixed hardware contract after the frontend has selected raw AArch64 or
// RISC-V execution: legal alignment, fetch width, instruction bits, and next PC.
module poly_raw_fetch_plan (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic [31:0] fetch_word_i,

    output logic        raw_fetch_o,
    output logic        align_fault_o,
    output logic [63:0] fetch_addr_o,
    output logic [2:0]  fetch_bytes_o,
    output logic [31:0] insn_o,
    output logic [63:0] next_pc_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [2:0] POLY_AARCH64_FETCH_BYTES = 3'd4;
  localparam logic [2:0] POLY_RISCV_FETCH_BYTES_16 = 3'd2;
  localparam logic [2:0] POLY_RISCV_FETCH_BYTES_32 = 3'd4;

  logic riscv_is_32;

  always_comb begin
    raw_fetch_o = 1'b0;
    align_fault_o = 1'b0;
    fetch_addr_o = pc_i;
    fetch_bytes_o = 3'd0;
    insn_o = 32'd0;
    next_pc_o = pc_i;
    riscv_is_32 = fetch_word_i[1:0] == 2'b11;

    if (valid_i) begin
      unique case (frontend_i)
        POLY_FRONTEND_AARCH64: begin
          raw_fetch_o = 1'b1;
          align_fault_o = pc_i[1:0] != 2'b00;
          fetch_bytes_o = POLY_AARCH64_FETCH_BYTES;
          insn_o = fetch_word_i;
          next_pc_o = pc_i + 64'd4;
        end
        POLY_FRONTEND_RISCV: begin
          raw_fetch_o = 1'b1;
          align_fault_o = pc_i[0] != 1'b0;
          fetch_bytes_o = riscv_is_32 ?
            POLY_RISCV_FETCH_BYTES_32 : POLY_RISCV_FETCH_BYTES_16;
          insn_o = riscv_is_32 ? fetch_word_i : {16'd0, fetch_word_i[15:0]};
          next_pc_o = pc_i + (riscv_is_32 ? 64'd4 : 64'd2);
        end
        default: begin
          raw_fetch_o = 1'b0;
        end
      endcase
    end
  end
endmodule
