// Poly frontend handoff and transition fault ordering.
//
// This fixed-latency control block sits after poly_ctrl_decode. It validates
// the frontend target, canonical target PC, raw-frontend alignment, ABI
// signature slot availability, and transition-stack capacity before allowing a
// frontend/PC mutation. It does not read memory or translate ABI state.
module poly_frontend_handoff (
    input  logic        valid_i,
    input  logic [1:0]  current_frontend_i,
    input  logic        poly_ctrl_i,
    input  logic [6:0]  subop_i,
    input  logic        call_sig_imm_i,
    input  logic [6:0]  signature_slot_i,
    input  logic        signature_slot_valid_i,
    input  logic [1:0]  target_frontend_i,
    input  logic [63:0] target_pc_i,
    input  logic [63:0] fallthrough_pc_i,
    input  logic        transition_stack_full_i,

    output logic        transition_o,
    output logic        call_o,
    output logic        switch_o,
    output logic        trap_return_o,
    output logic        landing_o,
    output logic        push_transition_o,
    output logic [1:0]  next_frontend_o,
    output logic [63:0] next_pc_o,
    output logic [6:0]  selected_signature_slot_o,

    output logic        error_o,
    output logic        invalid_subop_o,
    output logic        invalid_frontend_o,
    output logic        noncanonical_target_o,
    output logic        target_align_fault_o,
    output logic        invalid_signature_slot_o,
    output logic        transition_stack_full_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PENTER_MODE = 7'h03;
  localparam logic [6:0] POLY_X86_CTRL_PSWITCH_MODE = 7'h04;
  localparam logic [6:0] POLY_X86_CTRL_LANDING = 7'h05;
  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_MODE = 7'h2d;
  localparam logic [6:0] POLY_X86_CTRL_TRAP_RETURN = 7'h62;

  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE = 7'h70;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN = 7'h76;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE = 7'h78;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_CALL_MODE = 7'h79;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE = 7'h7a;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_LANDING = 7'h7b;

  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_X86_ESCAPE = 7'd0;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_TRAP_RETURN = 7'd6;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_SWITCH_MODE = 7'd8;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_CALL_MODE = 7'd9;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE = 7'd10;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_LANDING = 7'd11;

  logic frontend_valid;
  logic target_frontend_valid;
  logic target_is_x86_escape;
  logic op_recognized;
  logic signature_required;
  logic target_checked;
  logic [1:0] effective_target_frontend;

  function automatic logic canonical64(input logic [63:0] addr);
    return addr[63:48] == 16'h0000 || addr[63:48] == 16'hffff;
  endfunction

  function automatic logic aligned_target(
      input logic [1:0] frontend,
      input logic [63:0] addr
  );
    unique case (frontend)
      POLY_FRONTEND_AARCH64: return addr[1:0] == 2'b00;
      POLY_FRONTEND_RISCV: return addr[0] == 1'b0;
      default: return 1'b1;
    endcase
  endfunction

  always_comb begin
    frontend_valid =
      current_frontend_i == POLY_FRONTEND_X86 ||
      current_frontend_i == POLY_FRONTEND_AARCH64 ||
      current_frontend_i == POLY_FRONTEND_RISCV;
    target_frontend_valid =
      target_frontend_i == POLY_FRONTEND_X86 ||
      target_frontend_i == POLY_FRONTEND_AARCH64 ||
      target_frontend_i == POLY_FRONTEND_RISCV;

    target_is_x86_escape =
      (current_frontend_i == POLY_FRONTEND_AARCH64 &&
        subop_i == POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE) ||
      (current_frontend_i == POLY_FRONTEND_RISCV &&
        subop_i == POLY_RISCV_CTRL_SUBOP_X86_ESCAPE);
    effective_target_frontend =
      target_is_x86_escape ? POLY_FRONTEND_X86 : target_frontend_i;

    switch_o = 1'b0;
    call_o = 1'b0;
    trap_return_o = 1'b0;
    landing_o = 1'b0;
    signature_required = 1'b0;

    if (valid_i && poly_ctrl_i && frontend_valid) begin
      unique case (current_frontend_i)
        POLY_FRONTEND_X86: begin
          switch_o =
            subop_i == POLY_X86_CTRL_PENTER_MODE ||
            subop_i == POLY_X86_CTRL_PSWITCH_MODE;
          call_o =
            subop_i == POLY_X86_CTRL_PCALL_SIG_MODE ||
            call_sig_imm_i;
          signature_required = call_o;
          trap_return_o = subop_i == POLY_X86_CTRL_TRAP_RETURN;
          landing_o = subop_i == POLY_X86_CTRL_LANDING;
        end
        POLY_FRONTEND_AARCH64: begin
          switch_o =
            subop_i == POLY_AARCH64_CTRL_SUBOP_X86_ESCAPE ||
            subop_i == POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE;
          call_o =
            subop_i == POLY_AARCH64_CTRL_SUBOP_CALL_MODE ||
            subop_i == POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE ||
            call_sig_imm_i;
          signature_required =
            subop_i == POLY_AARCH64_CTRL_SUBOP_CALL_SIG_MODE ||
            call_sig_imm_i;
          trap_return_o = subop_i == POLY_AARCH64_CTRL_SUBOP_TRAP_RETURN;
          landing_o = subop_i == POLY_AARCH64_CTRL_SUBOP_LANDING;
        end
        POLY_FRONTEND_RISCV: begin
          switch_o =
            subop_i == POLY_RISCV_CTRL_SUBOP_X86_ESCAPE ||
            subop_i == POLY_RISCV_CTRL_SUBOP_SWITCH_MODE;
          call_o =
            subop_i == POLY_RISCV_CTRL_SUBOP_CALL_MODE ||
            subop_i == POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE ||
            call_sig_imm_i;
          signature_required =
            subop_i == POLY_RISCV_CTRL_SUBOP_CALL_SIG_MODE ||
            call_sig_imm_i;
          trap_return_o = subop_i == POLY_RISCV_CTRL_SUBOP_TRAP_RETURN;
          landing_o = subop_i == POLY_RISCV_CTRL_SUBOP_LANDING;
        end
        default: begin
        end
      endcase
    end

    op_recognized = switch_o || call_o || trap_return_o || landing_o;
    target_checked = switch_o || call_o || landing_o;

    invalid_frontend_o =
      valid_i && poly_ctrl_i &&
      (!frontend_valid || (target_checked && !target_frontend_valid &&
        !target_is_x86_escape));
    invalid_subop_o = valid_i && poly_ctrl_i && frontend_valid && !op_recognized;
    noncanonical_target_o =
      valid_i && poly_ctrl_i && target_checked && !invalid_frontend_o &&
      !canonical64(target_pc_i);
    target_align_fault_o =
      valid_i && poly_ctrl_i && target_checked && !invalid_frontend_o &&
      canonical64(target_pc_i) &&
      !aligned_target(effective_target_frontend, target_pc_i);
    invalid_signature_slot_o =
      valid_i && poly_ctrl_i && signature_required && !signature_slot_valid_i;
    transition_stack_full_o =
      valid_i && poly_ctrl_i && call_o && transition_stack_full_i;

    error_o =
      invalid_frontend_o ||
      invalid_subop_o ||
      noncanonical_target_o ||
      target_align_fault_o ||
      invalid_signature_slot_o ||
      transition_stack_full_o;

    transition_o = valid_i && poly_ctrl_i && !error_o && (switch_o || call_o);
    push_transition_o = valid_i && poly_ctrl_i && !error_o && call_o;
    next_frontend_o = transition_o ? effective_target_frontend : current_frontend_i;
    next_pc_o = transition_o ? target_pc_i : fallthrough_pc_i;
    selected_signature_slot_o = signature_required ? signature_slot_i : 7'd0;
  end
endmodule
