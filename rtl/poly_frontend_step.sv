// One-step Poly frontend planner for FPGA/silicon bring-up.
//
// This composes raw fetch geometry, Poly control decode, and frontend handoff
// validation. Memory fetch and full ISA execution are still separate blocks;
// this module only plans instruction width, control transitions, and pre-commit
// transition faults for one already-fetched word.
module poly_frontend_step (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic [31:0] fetch_word_i,
    input  logic [63:0] x86_fallthrough_pc_i,

    input  logic [1:0]  target_frontend_i,
    input  logic [63:0] target_pc_i,
    input  logic        signature_slot_valid_i,
    input  logic        transition_stack_full_i,

    output logic        raw_fetch_o,
    output logic [63:0] fetch_addr_o,
    output logic [2:0]  fetch_bytes_o,
    output logic [31:0] insn_o,

    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
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
    output logic        raw_align_fault_o,
    output logic        invalid_subop_o,
    output logic        invalid_frontend_o,
    output logic        noncanonical_target_o,
    output logic        target_align_fault_o,
    output logic        invalid_signature_slot_o,
    output logic        transition_stack_full_o
);
  logic raw_align_fault;
  logic [31:0] raw_insn;
  logic [63:0] raw_next_pc;
  logic [31:0] decode_insn;
  logic decode_valid;
  logic call_sig_imm;
  logic [6:0] signature_slot;
  logic [63:0] fallthrough_pc;
  logic handoff_error;
  logic [1:0] handoff_next_frontend;
  logic [63:0] handoff_next_pc;

  poly_raw_fetch_plan raw_fetch_plan (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fetch_word_i(fetch_word_i),
    .raw_fetch_o(raw_fetch_o),
    .align_fault_o(raw_align_fault),
    .fetch_addr_o(fetch_addr_o),
    .fetch_bytes_o(fetch_bytes_o),
    .insn_o(raw_insn),
    .next_pc_o(raw_next_pc)
  );

  always_comb begin
    decode_insn = raw_fetch_o ? raw_insn : fetch_word_i;
    decode_valid = valid_i && !raw_align_fault;
    fallthrough_pc = raw_fetch_o ? raw_next_pc : x86_fallthrough_pc_i;
    insn_o = decode_insn;
  end

  poly_ctrl_decode ctrl_decode (
    .valid_i(decode_valid),
    .frontend_i(frontend_i),
    .insn_i(decode_insn),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .call_sig_imm_o(call_sig_imm),
    .signature_slot_o(signature_slot)
  );

  poly_frontend_handoff frontend_handoff (
    .valid_i(decode_valid),
    .current_frontend_i(frontend_i),
    .poly_ctrl_i(poly_ctrl_o),
    .subop_i(subop_o),
    .call_sig_imm_i(call_sig_imm),
    .signature_slot_i(signature_slot),
    .signature_slot_valid_i(signature_slot_valid_i),
    .target_frontend_i(target_frontend_i),
    .target_pc_i(target_pc_i),
    .fallthrough_pc_i(fallthrough_pc),
    .transition_stack_full_i(transition_stack_full_i),
    .transition_o(transition_o),
    .call_o(call_o),
    .switch_o(switch_o),
    .trap_return_o(trap_return_o),
    .landing_o(landing_o),
    .push_transition_o(push_transition_o),
    .next_frontend_o(handoff_next_frontend),
    .next_pc_o(handoff_next_pc),
    .selected_signature_slot_o(selected_signature_slot_o),
    .error_o(handoff_error),
    .invalid_subop_o(invalid_subop_o),
    .invalid_frontend_o(invalid_frontend_o),
    .noncanonical_target_o(noncanonical_target_o),
    .target_align_fault_o(target_align_fault_o),
    .invalid_signature_slot_o(invalid_signature_slot_o),
    .transition_stack_full_o(transition_stack_full_o)
  );

  always_comb begin
    raw_align_fault_o = raw_align_fault;
    error_o = raw_align_fault || handoff_error;
    next_frontend_o = raw_align_fault ? frontend_i : handoff_next_frontend;
    next_pc_o = raw_align_fault ? pc_i : handoff_next_pc;
  end
endmodule
