// Raw foreign frontend fetch response stage.
//
// Request generation and address validation are owned by
// poly_frontend_fetch_issue. This block waits for the shared instruction-memory
// response, extracts the raw instruction word, and reports response or
// post-response alignment faults before retirement.
module poly_raw_fetch_response_stage (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic        request_valid_i,
    input  logic        mem_resp_valid_i,
    input  logic        mem_resp_fault_i,
    input  logic [31:0] mem_resp_word_i,

    output logic        wait_response_o,
    output logic        insn_valid_o,
    output logic [31:0] insn_o,
    output logic [2:0]  insn_bytes_o,
    output logic [63:0] next_pc_o,

    output logic        fault_o,
    output logic        mem_fault_o,
    output logic        response_align_fault_o
);
  logic plan_valid;
  logic plan_raw_fetch;
  logic plan_align_fault;

  always_comb begin
    wait_response_o = valid_i && request_valid_i && !mem_resp_valid_i;
    plan_valid =
      valid_i && request_valid_i && mem_resp_valid_i &&
      !mem_resp_fault_i;
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
    mem_fault_o =
      valid_i && request_valid_i && mem_resp_valid_i &&
      mem_resp_fault_i;
    response_align_fault_o = plan_valid && plan_align_fault;
    fault_o = mem_fault_o || response_align_fault_o;
  end
endmodule
