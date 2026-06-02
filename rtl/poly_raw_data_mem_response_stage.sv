// Raw foreign data-memory response stage.
//
// Request validation is owned by poly_raw_data_mem_request. This block converts
// the validated request plus memory response into the precise execute-stage
// completion/fault signals consumed by the Poly frontend core. It does not
// translate addresses, classify page faults, or apply OS policy.
module poly_raw_data_mem_response_stage (
    input  logic valid_i,
    input  logic request_valid_i,
    input  logic request_error_i,
    input  logic mem_resp_valid_i,
    input  logic mem_resp_fault_i,

    output logic wait_response_o,
    output logic resolved_o,
    output logic fault_o,
    output logic request_fault_o,
    output logic memory_fault_o
);
  always_comb begin
    wait_response_o =
      valid_i && request_valid_i && !mem_resp_valid_i && !request_error_i;
    request_fault_o = valid_i && request_error_i;
    memory_fault_o =
      valid_i && request_valid_i && mem_resp_valid_i && mem_resp_fault_i;
    resolved_o =
      valid_i && request_valid_i && mem_resp_valid_i && !mem_resp_fault_i &&
      !request_error_i;
    fault_o = request_fault_o || memory_fault_o;
  end
endmodule
