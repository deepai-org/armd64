// Poly frontend fetch-to-retire prototype.
//
// This composes raw foreign instruction fetch with the frontend retirement
// gate. The x86 frontend still supplies its fetched word and fallthrough PC
// externally; AArch64/RISC-V issue raw memory requests through the shared
// instruction-memory path and cannot retire until the response is fault-free.
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
    input  logic        execute_fault_i,
    input  logic [1:0]  target_frontend_i,
    input  logic [63:0] target_pc_i,
    input  logic        signature_slot_valid_i,
    input  logic        transition_stack_full_i,

    output logic        raw_mem_req_valid_o,
    output logic [63:0] raw_mem_req_addr_o,
    output logic [2:0]  raw_mem_req_bytes_o,

    output logic        wait_fetch_o,
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
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic x86_frontend;
  logic raw_frontend;
  logic frontend_valid;
  logic raw_insn_valid;
  logic [31:0] raw_insn;
  logic raw_fetch_fault;
  logic retire_fetch_valid;
  logic retire_fetch_fault;
  logic [31:0] retire_fetch_word;
  logic pipeline_invalid_frontend;
  logic retire_invalid_frontend;
  logic retire_fault;

  always_comb begin
    x86_frontend = frontend_i == POLY_FRONTEND_X86;
    raw_frontend =
      frontend_i == POLY_FRONTEND_AARCH64 ||
      frontend_i == POLY_FRONTEND_RISCV;
    frontend_valid = x86_frontend || raw_frontend;
  end

  poly_raw_fetch_stage raw_fetch_stage (
    .valid_i(valid_i && raw_frontend),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .mem_resp_valid_i(raw_mem_resp_valid_i),
    .mem_resp_fault_i(raw_mem_resp_fault_i),
    .mem_resp_word_i(raw_mem_resp_word_i),
    .mem_req_valid_o(raw_mem_req_valid_o),
    .mem_req_addr_o(raw_mem_req_addr_o),
    .mem_req_bytes_o(raw_mem_req_bytes_o),
    .wait_response_o(raw_fetch_wait_o),
    .insn_valid_o(raw_insn_valid),
    .insn_o(raw_insn),
    .insn_bytes_o(),
    .next_pc_o(),
    .fault_o(raw_fetch_fault),
    .fault_pc_o(),
    .request_error_o(raw_request_error_o),
    .mem_fault_o(raw_mem_fault_o),
    .invalid_frontend_o(),
    .noncanonical_pc_o(raw_noncanonical_pc_o),
    .align_fault_o(raw_align_fault_o),
    .range_fault_o(raw_range_fault_o)
  );

  always_comb begin
    pipeline_invalid_frontend = valid_i && !frontend_valid;
    retire_fetch_valid =
      (x86_frontend && x86_fetch_valid_i && !x86_fetch_fault_i) ||
      (raw_frontend && raw_insn_valid);
    retire_fetch_fault =
      pipeline_invalid_frontend ||
      (x86_frontend && x86_fetch_fault_i) ||
      (raw_frontend && raw_fetch_fault);
    retire_fetch_word = raw_frontend ? raw_insn : x86_fetch_word_i;
  end

  poly_frontend_retire frontend_retire (
    .valid_i(valid_i),
    .fetch_valid_i(retire_fetch_valid),
    .older_fault_i(older_fault_i),
    .fetch_fault_i(retire_fetch_fault),
    .execute_fault_i(execute_fault_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fetch_word_i(retire_fetch_word),
    .x86_fallthrough_pc_i(x86_fallthrough_pc_i),
    .target_frontend_i(target_frontend_i),
    .target_pc_i(target_pc_i),
    .signature_slot_valid_i(signature_slot_valid_i),
    .transition_stack_full_i(transition_stack_full_i),
    .wait_fetch_o(wait_fetch_o),
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
    invalid_frontend_o = pipeline_invalid_frontend || retire_invalid_frontend;
  end
endmodule
