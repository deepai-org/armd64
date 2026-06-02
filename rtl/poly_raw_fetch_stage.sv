// Raw foreign frontend fetch stage.
//
// This composes request-side address validation with response-side instruction
// extraction. The memory subsystem still owns translation, TLB, permissions,
// and page-fault classification; this stage only prevents bad raw fetch
// requests from issuing and blocks instruction retirement on memory faults.
module poly_raw_fetch_stage (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic        mem_resp_valid_i,
    input  logic        mem_resp_fault_i,
    input  logic [31:0] mem_resp_word_i,

    output logic        mem_req_valid_o,
    output logic [63:0] mem_req_addr_o,
    output logic [2:0]  mem_req_bytes_o,
    output logic        wait_response_o,

    output logic        insn_valid_o,
    output logic [31:0] insn_o,
    output logic [2:0]  insn_bytes_o,
    output logic [63:0] next_pc_o,

    output logic        fault_o,
    output logic [63:0] fault_pc_o,
    output logic        request_error_o,
    output logic        mem_fault_o,
    output logic        invalid_frontend_o,
    output logic        noncanonical_pc_o,
    output logic        align_fault_o,
    output logic        range_fault_o
);
  logic request_error;
  logic request_valid;
  logic plan_valid;
  logic plan_raw_fetch;
  logic plan_align_fault;

  poly_raw_fetch_request fetch_request (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .request_valid_o(request_valid),
    .request_addr_o(mem_req_addr_o),
    .request_bytes_o(mem_req_bytes_o),
    .error_o(request_error),
    .invalid_frontend_o(invalid_frontend_o),
    .noncanonical_pc_o(noncanonical_pc_o),
    .align_fault_o(align_fault_o),
    .range_fault_o(range_fault_o)
  );

  always_comb begin
    mem_req_valid_o = request_valid;
    wait_response_o = request_valid && !mem_resp_valid_i;
    plan_valid = request_valid && mem_resp_valid_i && !mem_resp_fault_i;
  end

  poly_raw_fetch_plan fetch_plan (
    .valid_i(plan_valid),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fetch_word_i(mem_resp_word_i),
    .raw_fetch_o(plan_raw_fetch),
    .align_fault_o(plan_align_fault),
    .fetch_addr_o(),
    .fetch_bytes_o(insn_bytes_o),
    .insn_o(insn_o),
    .next_pc_o(next_pc_o)
  );

  always_comb begin
    insn_valid_o = plan_raw_fetch && !plan_align_fault;
    request_error_o = request_error;
    mem_fault_o = request_valid && mem_resp_valid_i && mem_resp_fault_i;
    fault_o = request_error || mem_fault_o || (plan_valid && plan_align_fault);
    fault_pc_o = fault_o ? pc_i : 64'd0;
  end
endmodule
