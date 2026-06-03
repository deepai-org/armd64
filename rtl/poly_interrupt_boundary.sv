// Poly raw-frontend interrupt entry and user-return restore boundary.
//
// Asynchronous interrupts from raw AArch64/RISC-V user mode enter the standard
// x86 interrupt path after recording the precise foreign PC. On return to user
// mode, hardware resumes the saved raw frontend only when the architectural
// return PC matches the interrupted foreign PC.
module poly_interrupt_boundary (
    input  logic        valid_i,
    input  logic        feature_enabled_i,
    input  logic        cpl3_i,
    input  logic        interrupt_i,
    input  logic        user_return_i,
    input  logic        state_dirty_i,
    input  logic [1:0]  current_frontend_i,
    input  logic [63:0] current_pc_i,
    input  logic        interrupted_valid_i,
    input  logic [1:0]  interrupted_frontend_i,
    input  logic [63:0] interrupted_pc_i,
    input  logic [63:0] user_return_pc_i,

    output logic        enter_x86_interrupt_o,
    output logic        save_interrupted_o,
    output logic        spill_full_state_o,
    output logic        spill_header_only_o,
    output logic        clear_state_dirty_o,
    output logic [1:0]  saved_frontend_o,
    output logic [63:0] saved_pc_o,
    output logic        restore_raw_o,
    output logic        clear_interrupted_o,
    output logic [1:0]  next_frontend_o,
    output logic [63:0] next_pc_o,

    output logic        error_o,
    output logic        invalid_current_frontend_o,
    output logic        invalid_current_pc_o,
    output logic        invalid_interrupted_frontend_o,
    output logic        invalid_interrupted_pc_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic current_raw;
  logic interrupted_raw;
  logic interrupt_candidate;
  logic return_candidate;

  function automatic logic canonical64(input logic [63:0] addr);
    begin
      canonical64 = addr[63:48] == {16{addr[47]}};
    end
  endfunction

  function automatic logic frontend_aligned(
      input logic [1:0] frontend,
      input logic [63:0] addr
  );
    begin
      unique case (frontend)
        POLY_FRONTEND_AARCH64: frontend_aligned = addr[1:0] == 2'b00;
        POLY_FRONTEND_RISCV: frontend_aligned = addr[0] == 1'b0;
        default: frontend_aligned = 1'b1;
      endcase
    end
  endfunction

  always_comb begin
    current_raw =
      current_frontend_i == POLY_FRONTEND_AARCH64 ||
      current_frontend_i == POLY_FRONTEND_RISCV;
    interrupted_raw =
      interrupted_frontend_i == POLY_FRONTEND_AARCH64 ||
      interrupted_frontend_i == POLY_FRONTEND_RISCV;

    interrupt_candidate =
      valid_i && feature_enabled_i && cpl3_i && interrupt_i && current_raw;
    return_candidate =
      valid_i && feature_enabled_i && cpl3_i && user_return_i &&
      interrupted_valid_i;

    invalid_current_frontend_o =
      valid_i && interrupt_i &&
      !(current_frontend_i == POLY_FRONTEND_X86 ||
        current_frontend_i == POLY_FRONTEND_AARCH64 ||
        current_frontend_i == POLY_FRONTEND_RISCV);
    invalid_current_pc_o =
      interrupt_candidate &&
      (!canonical64(current_pc_i) ||
        !frontend_aligned(current_frontend_i, current_pc_i));
    invalid_interrupted_frontend_o =
      return_candidate && !interrupted_raw;
    invalid_interrupted_pc_o =
      return_candidate &&
      (!canonical64(interrupted_pc_i) ||
        !frontend_aligned(interrupted_frontend_i, interrupted_pc_i));

    error_o =
      invalid_current_frontend_o ||
      invalid_current_pc_o ||
      invalid_interrupted_frontend_o ||
      invalid_interrupted_pc_o;

    enter_x86_interrupt_o = interrupt_candidate && !error_o;
    save_interrupted_o = enter_x86_interrupt_o;
    spill_full_state_o = enter_x86_interrupt_o && state_dirty_i;
    spill_header_only_o = enter_x86_interrupt_o && !state_dirty_i;
    clear_state_dirty_o = enter_x86_interrupt_o;
    saved_frontend_o = save_interrupted_o ? current_frontend_i : POLY_FRONTEND_X86;
    saved_pc_o = save_interrupted_o ? current_pc_i : 64'd0;

    restore_raw_o =
      return_candidate &&
      !error_o &&
      user_return_pc_i == interrupted_pc_i;
    clear_interrupted_o = restore_raw_o;

    next_frontend_o =
      enter_x86_interrupt_o ? POLY_FRONTEND_X86 :
      restore_raw_o ? interrupted_frontend_i :
      current_frontend_i;
    next_pc_o =
      enter_x86_interrupt_o ? current_pc_i :
      restore_raw_o ? interrupted_pc_i :
      current_pc_i;
  end
endmodule
