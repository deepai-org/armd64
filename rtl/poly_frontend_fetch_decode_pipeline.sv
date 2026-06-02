// Poly frontend fetch/decode pipeline boundary.
//
// This wrapper composes request issue, frontend response handling, and Poly
// control decode dispatch for one architectural frontend/PC. It does not
// perform retirement, execute non-control instructions, translate memory, or
// encode OS policy.
module poly_frontend_fetch_decode_pipeline (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,

    input  logic        x86_fetch_resp_valid_i,
    input  logic        x86_fetch_resp_fault_i,
    input  logic [31:0] x86_fetch_resp_word_i,
    input  logic [63:0] x86_fetch_resp_fallthrough_pc_i,

    input  logic        raw_mem_resp_valid_i,
    input  logic        raw_mem_resp_fault_i,
    input  logic [31:0] raw_mem_resp_word_i,

    output logic        x86_fetch_req_valid_o,
    output logic [63:0] x86_fetch_req_addr_o,
    output logic [4:0]  x86_fetch_req_bytes_o,

    output logic        raw_mem_req_valid_o,
    output logic [63:0] raw_mem_req_addr_o,
    output logic [2:0]  raw_mem_req_bytes_o,

    output logic        wait_fetch_o,
    output logic        fetch_valid_o,
    output logic        fetch_fault_o,
    output logic [31:0] fetch_word_o,
    output logic [63:0] fetch_fallthrough_pc_o,

    output logic        decode_valid_o,
    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
    output logic        call_sig_imm_o,
    output logic [6:0]  signature_slot_o,

    output logic        raw_insn_valid_o,
    output logic        raw_memory_order_valid_o,
    output logic        raw_memory_load_o,
    output logic        raw_memory_store_o,
    output logic        raw_memory_atomic_o,
    output logic        raw_memory_barrier_o,
    output logic        raw_branch_o,
    output logic        raw_call_o,
    output logic        raw_return_o,
    output logic        raw_trap_o,
    output logic        raw_branch_target_valid_o,
    output logic [63:0] raw_branch_target_o,

    output logic        invalid_frontend_o,
    output logic        x86_fetch_wait_o,
    output logic        x86_request_error_o,
    output logic        x86_mem_fault_o,
    output logic        x86_noncanonical_pc_o,
    output logic        x86_range_fault_o,
    output logic        raw_fetch_wait_o,
    output logic        raw_request_error_o,
    output logic        raw_mem_fault_o,
    output logic        raw_noncanonical_pc_o,
    output logic        raw_align_fault_o,
    output logic        raw_range_fault_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic x86_frontend;
  logic raw_frontend;
  logic fetch_issue_fault;
  logic fetch_issue_invalid_frontend;
  logic raw_invalid_frontend;
  logic x86_insn_valid;
  logic [31:0] x86_insn;
  logic [63:0] x86_fallthrough_pc;
  logic x86_fetch_fault;
  logic raw_insn_valid;
  logic [31:0] raw_insn;
  logic raw_fetch_fault;
  logic [63:0] fetch_stage_fallthrough_pc;
  logic [63:0] dispatch_fallthrough_pc;

  always_comb begin
    x86_frontend = frontend_i == POLY_FRONTEND_X86;
    raw_frontend =
      frontend_i == POLY_FRONTEND_AARCH64 ||
      frontend_i == POLY_FRONTEND_RISCV;
  end

  poly_frontend_fetch_issue fetch_issue (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .x86_fetch_req_valid_o(x86_fetch_req_valid_o),
    .x86_fetch_req_addr_o(x86_fetch_req_addr_o),
    .x86_fetch_req_bytes_o(x86_fetch_req_bytes_o),
    .raw_fetch_req_valid_o(raw_mem_req_valid_o),
    .raw_fetch_req_addr_o(raw_mem_req_addr_o),
    .raw_fetch_req_bytes_o(raw_mem_req_bytes_o),
    .fault_o(fetch_issue_fault),
    .invalid_frontend_o(fetch_issue_invalid_frontend),
    .x86_noncanonical_pc_o(x86_noncanonical_pc_o),
    .x86_range_fault_o(x86_range_fault_o),
    .raw_request_error_o(raw_request_error_o),
    .raw_invalid_frontend_o(raw_invalid_frontend),
    .raw_noncanonical_pc_o(raw_noncanonical_pc_o),
    .raw_align_fault_o(raw_align_fault_o),
    .raw_range_fault_o(raw_range_fault_o)
  );

  poly_x86_fetch_stage x86_fetch_stage (
    .valid_i(valid_i && x86_frontend),
    .request_valid_i(x86_fetch_req_valid_o),
    .fetch_resp_valid_i(x86_fetch_resp_valid_i),
    .fetch_resp_fault_i(x86_fetch_resp_fault_i),
    .fetch_resp_word_i(x86_fetch_resp_word_i),
    .fetch_resp_fallthrough_pc_i(x86_fetch_resp_fallthrough_pc_i),
    .wait_response_o(x86_fetch_wait_o),
    .insn_valid_o(x86_insn_valid),
    .insn_o(x86_insn),
    .fallthrough_pc_o(x86_fallthrough_pc),
    .fault_o(x86_fetch_fault),
    .mem_fault_o(x86_mem_fault_o)
  );

  poly_raw_fetch_response_stage raw_fetch_response_stage (
    .valid_i(valid_i && raw_frontend),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .request_valid_i(raw_mem_req_valid_o),
    .mem_resp_valid_i(raw_mem_resp_valid_i),
    .mem_resp_fault_i(raw_mem_resp_fault_i),
    .mem_resp_word_i(raw_mem_resp_word_i),
    .wait_response_o(raw_fetch_wait_o),
    .insn_valid_o(raw_insn_valid),
    .insn_o(raw_insn),
    .insn_bytes_o(),
    .next_pc_o(),
    .fault_o(raw_fetch_fault),
    .mem_fault_o(raw_mem_fault_o),
    .response_align_fault_o()
  );

  always_comb begin
    wait_fetch_o = x86_fetch_wait_o || raw_fetch_wait_o;
    fetch_valid_o =
      (x86_frontend && x86_insn_valid) ||
      (raw_frontend && raw_insn_valid);
    fetch_fault_o =
      fetch_issue_fault ||
      (x86_frontend && x86_fetch_fault) ||
      (raw_frontend && raw_fetch_fault);
    fetch_word_o = raw_frontend ? raw_insn : x86_insn;
    fetch_stage_fallthrough_pc = x86_frontend ? x86_fallthrough_pc : 64'd0;
    fetch_fallthrough_pc_o = dispatch_fallthrough_pc;
    x86_request_error_o = x86_noncanonical_pc_o || x86_range_fault_o;
    invalid_frontend_o =
      fetch_issue_invalid_frontend || raw_invalid_frontend;
  end

  poly_frontend_decode_dispatch decode_dispatch (
    .valid_i(fetch_valid_o),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fetch_word_i(fetch_word_o),
    .x86_fallthrough_pc_i(fetch_stage_fallthrough_pc),
    .raw_fetch_o(),
    .fetch_addr_o(),
    .fetch_bytes_o(),
    .insn_o(),
    .fallthrough_pc_o(dispatch_fallthrough_pc),
    .decode_valid_o(decode_valid_o),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .call_sig_imm_o(call_sig_imm_o),
    .signature_slot_o(signature_slot_o),
    .raw_insn_valid_o(raw_insn_valid_o),
    .raw_memory_order_valid_o(raw_memory_order_valid_o),
    .raw_memory_load_o(raw_memory_load_o),
    .raw_memory_store_o(raw_memory_store_o),
    .raw_memory_atomic_o(raw_memory_atomic_o),
    .raw_memory_barrier_o(raw_memory_barrier_o),
    .raw_branch_o(raw_branch_o),
    .raw_call_o(raw_call_o),
    .raw_return_o(raw_return_o),
    .raw_trap_o(raw_trap_o),
    .raw_branch_target_valid_o(raw_branch_target_valid_o),
    .raw_branch_target_o(raw_branch_target_o),
    .raw_align_fault_o()
  );
endmodule
