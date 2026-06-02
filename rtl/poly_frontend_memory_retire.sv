// Poly frontend fetch-to-retire prototype.
//
// This composes the fetch/decode pipeline boundary with the frontend
// retirement gate. Fetch issue, frontend responses, and fetch faults are handled
// before retirement; transition validation and architectural commits consume
// the predecoded control record without re-running frontend decode.
module poly_frontend_memory_retire (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,

    input  logic        x86_fetch_valid_i,
    input  logic        x86_fetch_fault_i,
    input  logic [31:0] x86_fetch_word_i,
    input  logic [63:0] x86_fallthrough_pc_i,

    input  logic        raw_mem_resp_valid_i,
    input  logic        raw_mem_resp_fault_i,
    input  logic [31:0] raw_mem_resp_word_i,

    input  logic        older_fault_i,
    input  logic        execute_ready_i,
    input  logic        block_retire_i,
    input  logic        execute_fault_i,
    input  logic [1:0]  target_frontend_i,
    input  logic [63:0] target_pc_i,
    input  logic        signature_slot_valid_i,
    input  logic        transition_stack_full_i,

    output logic        x86_fetch_req_valid_o,
    output logic [63:0] x86_fetch_req_addr_o,
    output logic [4:0]  x86_fetch_req_bytes_o,

    output logic        raw_mem_req_valid_o,
    output logic [63:0] raw_mem_req_addr_o,
    output logic [2:0]  raw_mem_req_bytes_o,

    output logic        wait_fetch_o,
    output logic        wait_execute_o,
    output logic        wait_retire_o,
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
    output logic        invalid_frontend_o,
    output logic        x86_fetch_wait_o,
    output logic        x86_request_error_o,
    output logic        x86_mem_fault_o,
    output logic        x86_noncanonical_pc_o,
    output logic        x86_range_fault_o,

    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
    output logic        raw_fetch_wait_o,
    output logic        raw_request_error_o,
    output logic        raw_mem_fault_o,
    output logic        raw_noncanonical_pc_o,
    output logic        raw_align_fault_o,
    output logic        raw_range_fault_o,
    output logic        invalid_subop_o,
    output logic        noncanonical_target_o,
    output logic        target_align_fault_o,
    output logic        invalid_signature_slot_o,
    output logic        transition_stack_full_o
);
  logic fetch_pipeline_valid;
  logic fetch_pipeline_fault;
  logic [63:0] fetch_pipeline_fallthrough_pc;
  logic decode_pipeline_valid;
  logic decode_pipeline_poly_ctrl;
  logic [6:0] decode_pipeline_subop;
  logic decode_pipeline_call_sig_imm;
  logic [6:0] decode_pipeline_signature_slot;
  logic fetch_pipeline_invalid_frontend;
  logic retire_fetch_valid;
  logic retire_fetch_fault;
  logic retire_invalid_frontend;
  logic retire_fault;

  poly_frontend_fetch_decode_pipeline fetch_decode_pipeline (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .x86_fetch_resp_valid_i(x86_fetch_valid_i),
    .x86_fetch_resp_fault_i(x86_fetch_fault_i),
    .x86_fetch_resp_word_i(x86_fetch_word_i),
    .x86_fetch_resp_fallthrough_pc_i(x86_fallthrough_pc_i),
    .raw_mem_resp_valid_i(raw_mem_resp_valid_i),
    .raw_mem_resp_fault_i(raw_mem_resp_fault_i),
    .raw_mem_resp_word_i(raw_mem_resp_word_i),
    .x86_fetch_req_valid_o(x86_fetch_req_valid_o),
    .x86_fetch_req_addr_o(x86_fetch_req_addr_o),
    .x86_fetch_req_bytes_o(x86_fetch_req_bytes_o),
    .raw_mem_req_valid_o(raw_mem_req_valid_o),
    .raw_mem_req_addr_o(raw_mem_req_addr_o),
    .raw_mem_req_bytes_o(raw_mem_req_bytes_o),
    .wait_fetch_o(),
    .fetch_valid_o(fetch_pipeline_valid),
    .fetch_fault_o(fetch_pipeline_fault),
    .fetch_word_o(),
    .fetch_fallthrough_pc_o(fetch_pipeline_fallthrough_pc),
    .decode_valid_o(decode_pipeline_valid),
    .poly_ctrl_o(decode_pipeline_poly_ctrl),
    .subop_o(decode_pipeline_subop),
    .call_sig_imm_o(decode_pipeline_call_sig_imm),
    .signature_slot_o(decode_pipeline_signature_slot),
    .invalid_frontend_o(fetch_pipeline_invalid_frontend),
    .x86_fetch_wait_o(x86_fetch_wait_o),
    .x86_request_error_o(x86_request_error_o),
    .x86_mem_fault_o(x86_mem_fault_o),
    .x86_noncanonical_pc_o(x86_noncanonical_pc_o),
    .x86_range_fault_o(x86_range_fault_o),
    .raw_fetch_wait_o(raw_fetch_wait_o),
    .raw_request_error_o(raw_request_error_o),
    .raw_mem_fault_o(raw_mem_fault_o),
    .raw_noncanonical_pc_o(raw_noncanonical_pc_o),
    .raw_align_fault_o(raw_align_fault_o),
    .raw_range_fault_o(raw_range_fault_o)
  );

  always_comb begin
    retire_fetch_valid = fetch_pipeline_valid;
    retire_fetch_fault = fetch_pipeline_fault;
  end

  poly_frontend_predecoded_retire frontend_predecoded_retire (
    .valid_i(valid_i),
    .fetch_valid_i(retire_fetch_valid),
    .decode_valid_i(decode_pipeline_valid),
    .execute_ready_i(execute_ready_i),
    .block_retire_i(block_retire_i),
    .older_fault_i(older_fault_i),
    .fetch_fault_i(retire_fetch_fault),
    .execute_fault_i(execute_fault_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fallthrough_pc_i(fetch_pipeline_fallthrough_pc),
    .poly_ctrl_i(decode_pipeline_poly_ctrl),
    .subop_i(decode_pipeline_subop),
    .call_sig_imm_i(decode_pipeline_call_sig_imm),
    .signature_slot_i(decode_pipeline_signature_slot),
    .target_frontend_i(target_frontend_i),
    .target_pc_i(target_pc_i),
    .signature_slot_valid_i(signature_slot_valid_i),
    .transition_stack_full_i(transition_stack_full_i),
    .wait_fetch_o(wait_fetch_o),
    .wait_execute_o(wait_execute_o),
    .wait_retire_o(wait_retire_o),
    .retire_o(retire_o),
    .commit_transition_o(commit_transition_o),
    .commit_push_transition_o(commit_push_transition_o),
    .commit_frontend_o(commit_frontend_o),
    .commit_pc_o(commit_pc_o),
    .commit_signature_slot_o(commit_signature_slot_o),
    .fault_o(retire_fault),
    .fault_pc_o(fault_pc_o),
    .older_fault_o(older_fault_o),
    .fetch_fault_o(fetch_fault_o),
    .execute_fault_o(execute_fault_o),
    .control_fault_o(control_fault_o),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .raw_align_fault_o(),
    .invalid_subop_o(invalid_subop_o),
    .invalid_frontend_o(retire_invalid_frontend),
    .noncanonical_target_o(noncanonical_target_o),
    .target_align_fault_o(target_align_fault_o),
    .invalid_signature_slot_o(invalid_signature_slot_o),
    .transition_stack_full_o(transition_stack_full_o)
  );

  always_comb begin
    fault_o = retire_fault;
    invalid_frontend_o =
      fetch_pipeline_invalid_frontend || retire_invalid_frontend;
  end
endmodule
