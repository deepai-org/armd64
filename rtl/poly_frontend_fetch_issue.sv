// Dual frontend instruction-fetch request issue.
//
// This block is the silicon-facing selector between the x86 byte frontend and
// the raw fixed-width foreign frontends. It validates the architectural PC and
// emits one fetch request for the active frontend. It does not decode
// instructions, translate addresses, check permissions, or read memory.
module poly_frontend_fetch_issue (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,

    output logic        x86_fetch_req_valid_o,
    output logic [63:0] x86_fetch_req_addr_o,
    output logic [4:0]  x86_fetch_req_bytes_o,

    output logic        raw_fetch_req_valid_o,
    output logic [63:0] raw_fetch_req_addr_o,
    output logic [2:0]  raw_fetch_req_bytes_o,

    output logic        fault_o,
    output logic        invalid_frontend_o,
    output logic        x86_noncanonical_pc_o,
    output logic        x86_range_fault_o,
    output logic        raw_request_error_o,
    output logic        raw_invalid_frontend_o,
    output logic        raw_noncanonical_pc_o,
    output logic        raw_align_fault_o,
    output logic        raw_range_fault_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [4:0] POLY_X86_FETCH_REQUEST_BYTES = 5'd16;
  localparam logic [63:0] POLY_X86_LAST_OFFSET = 64'd15;

  logic x86_frontend;
  logic raw_frontend;
  logic [63:0] x86_last_addr;

  assign x86_frontend = frontend_i == POLY_FRONTEND_X86;
  assign raw_frontend =
    frontend_i == POLY_FRONTEND_AARCH64 ||
    frontend_i == POLY_FRONTEND_RISCV;
  assign x86_last_addr = pc_i + POLY_X86_LAST_OFFSET;

  function automatic logic canonical64(input logic [63:0] addr);
    begin
      canonical64 = addr[63:48] == {16{addr[47]}};
    end
  endfunction

  poly_raw_fetch_request raw_fetch_request (
    .valid_i(valid_i && raw_frontend),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .request_valid_o(raw_fetch_req_valid_o),
    .request_addr_o(raw_fetch_req_addr_o),
    .request_bytes_o(raw_fetch_req_bytes_o),
    .error_o(raw_request_error_o),
    .invalid_frontend_o(raw_invalid_frontend_o),
    .noncanonical_pc_o(raw_noncanonical_pc_o),
    .align_fault_o(raw_align_fault_o),
    .range_fault_o(raw_range_fault_o)
  );

  always_comb begin
    invalid_frontend_o = valid_i && !x86_frontend && !raw_frontend;
    x86_noncanonical_pc_o =
      valid_i && x86_frontend &&
      (!canonical64(pc_i) || !canonical64(x86_last_addr));
    x86_range_fault_o =
      valid_i && x86_frontend && x86_last_addr < pc_i;

    x86_fetch_req_valid_o =
      valid_i && x86_frontend &&
      !invalid_frontend_o &&
      !x86_noncanonical_pc_o &&
      !x86_range_fault_o;
    x86_fetch_req_addr_o = pc_i;
    x86_fetch_req_bytes_o =
      x86_frontend ? POLY_X86_FETCH_REQUEST_BYTES : 5'd0;

    fault_o =
      invalid_frontend_o ||
      x86_noncanonical_pc_o ||
      x86_range_fault_o ||
      raw_request_error_o;
  end
endmodule
