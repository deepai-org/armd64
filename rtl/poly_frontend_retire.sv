// Poly frontend retirement gate.
//
// Decode/handoff planning is speculative until retirement. This block prevents
// frontend, PC, and transition-stack mutations until fetch and execute are
// ready, and when an older exception, fetch fault, execution fault, or Poly
// control validation fault is present.
module poly_frontend_retire (
    input  logic        valid_i,
    input  logic        fetch_valid_i,
    input  logic        execute_ready_i,
    input  logic        older_fault_i,
    input  logic        fetch_fault_i,
    input  logic        execute_fault_i,

    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic [31:0] fetch_word_i,
    input  logic [63:0] x86_fallthrough_pc_i,

    input  logic [1:0]  target_frontend_i,
    input  logic [63:0] target_pc_i,
    input  logic        signature_slot_valid_i,
    input  logic        transition_stack_full_i,

    output logic        wait_fetch_o,
    output logic        wait_execute_o,
    output logic        retire_o,
    output logic        commit_transition_o,
    output logic        commit_push_transition_o,
    output logic [1:0]  commit_frontend_o,
    output logic [63:0] commit_pc_o,
    output logic [6:0]  commit_signature_slot_o,

    output logic        fault_o,
    output logic [63:0] fault_pc_o,
    output logic        older_fault_o,
    output logic        fetch_fault_o,
    output logic        execute_fault_o,
    output logic        control_fault_o,

    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
    output logic        raw_align_fault_o,
    output logic        invalid_subop_o,
    output logic        invalid_frontend_o,
    output logic        noncanonical_target_o,
    output logic        target_align_fault_o,
    output logic        invalid_signature_slot_o,
    output logic        transition_stack_full_o
);
  logic step_valid;
  logic step_transition;
  logic step_push_transition;
  logic [1:0] step_next_frontend;
  logic [63:0] step_next_pc;
  logic [6:0] step_signature_slot;
  logic step_error;

  always_comb begin
    wait_fetch_o =
      valid_i && !fetch_valid_i && !older_fault_i && !fetch_fault_i &&
      !execute_fault_i;
    wait_execute_o =
      valid_i && fetch_valid_i && !execute_ready_i && !older_fault_i &&
      !fetch_fault_i && !execute_fault_i;
    step_valid =
      valid_i && fetch_valid_i && execute_ready_i && !older_fault_i && !fetch_fault_i &&
      !execute_fault_i;
  end

  poly_frontend_step frontend_step (
    .valid_i(step_valid),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fetch_word_i(fetch_word_i),
    .x86_fallthrough_pc_i(x86_fallthrough_pc_i),
    .target_frontend_i(target_frontend_i),
    .target_pc_i(target_pc_i),
    .signature_slot_valid_i(signature_slot_valid_i),
    .transition_stack_full_i(transition_stack_full_i),
    .raw_fetch_o(),
    .fetch_addr_o(),
    .fetch_bytes_o(),
    .insn_o(),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .transition_o(step_transition),
    .call_o(),
    .switch_o(),
    .trap_return_o(),
    .landing_o(),
    .push_transition_o(step_push_transition),
    .next_frontend_o(step_next_frontend),
    .next_pc_o(step_next_pc),
    .selected_signature_slot_o(step_signature_slot),
    .error_o(step_error),
    .raw_align_fault_o(raw_align_fault_o),
    .invalid_subop_o(invalid_subop_o),
    .invalid_frontend_o(invalid_frontend_o),
    .noncanonical_target_o(noncanonical_target_o),
    .target_align_fault_o(target_align_fault_o),
    .invalid_signature_slot_o(invalid_signature_slot_o),
    .transition_stack_full_o(transition_stack_full_o)
  );

  always_comb begin
    older_fault_o = valid_i && older_fault_i;
    fetch_fault_o = valid_i && !older_fault_i && fetch_fault_i;
    execute_fault_o =
      valid_i && !older_fault_i && !fetch_fault_i && execute_fault_i;
    control_fault_o = step_valid && step_error;
    fault_o =
      older_fault_o || fetch_fault_o || execute_fault_o || control_fault_o;
    fault_pc_o = fault_o ? pc_i : 64'd0;

    retire_o = step_valid && !step_error;
    commit_transition_o = retire_o && step_transition;
    commit_push_transition_o = retire_o && step_push_transition;
    commit_frontend_o = retire_o ? step_next_frontend : frontend_i;
    commit_pc_o = retire_o ? step_next_pc : pc_i;
    commit_signature_slot_o = retire_o ? step_signature_slot : 7'd0;
  end
endmodule
