// x86 frontend fetch response stage.
//
// Request generation and address validation are owned by
// poly_frontend_fetch_issue. This block only waits for the x86 byte frontend
// response and turns a fault-free response into one instruction word plus the
// frontend-provided fallthrough PC.
module poly_x86_fetch_stage (
    input  logic        valid_i,
    input  logic        request_valid_i,
    input  logic        fetch_resp_valid_i,
    input  logic        fetch_resp_fault_i,
    input  logic [31:0] fetch_resp_word_i,
    input  logic [63:0] fetch_resp_fallthrough_pc_i,

    output logic        wait_response_o,
    output logic        insn_valid_o,
    output logic [31:0] insn_o,
    output logic [63:0] fallthrough_pc_o,

    output logic        fault_o,
    output logic        mem_fault_o
);
  always_comb begin
    wait_response_o = valid_i && request_valid_i && !fetch_resp_valid_i;
    insn_valid_o =
      valid_i && request_valid_i && fetch_resp_valid_i &&
      !fetch_resp_fault_i;
    insn_o = insn_valid_o ? fetch_resp_word_i : 32'd0;
    fallthrough_pc_o =
      insn_valid_o ? fetch_resp_fallthrough_pc_i : 64'd0;
    mem_fault_o =
      valid_i && request_valid_i && fetch_resp_valid_i &&
      fetch_resp_fault_i;
    fault_o = mem_fault_o;
  end
endmodule
