// Raw foreign frontend instruction-fetch request geometry.
//
// This block is the memory-side partner to poly_raw_fetch_plan. It validates
// canonical PC, frontend-specific alignment, and request range before issuing a
// raw frontend instruction-memory request. It does not perform translation,
// permissions, TLB lookup, or memory reads.
module poly_raw_fetch_request (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,

    output logic        request_valid_o,
    output logic [63:0] request_addr_o,
    output logic [2:0]  request_bytes_o,

    output logic        error_o,
    output logic        invalid_frontend_o,
    output logic        noncanonical_pc_o,
    output logic        align_fault_o,
    output logic        range_fault_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [2:0] POLY_AARCH64_FETCH_REQUEST_BYTES = 3'd4;
  localparam logic [2:0] POLY_RISCV_FETCH_REQUEST_BYTES = 3'd4;
  localparam logic [63:0] POLY_AARCH64_LAST_OFFSET = 64'd3;
  localparam logic [63:0] POLY_RISCV_LAST_OFFSET = 64'd3;

  logic raw_frontend;
  logic [63:0] request_last_addr;
  logic [63:0] last_offset;

  function automatic logic canonical64(input logic [63:0] addr);
    return addr[63:48] == {16{addr[47]}};
  endfunction

  always_comb begin
    raw_frontend =
      frontend_i == POLY_FRONTEND_AARCH64 ||
      frontend_i == POLY_FRONTEND_RISCV;
    last_offset =
      frontend_i == POLY_FRONTEND_AARCH64 ?
        POLY_AARCH64_LAST_OFFSET : POLY_RISCV_LAST_OFFSET;
    request_last_addr = pc_i + last_offset;

    invalid_frontend_o =
      valid_i && frontend_i != POLY_FRONTEND_X86 && !raw_frontend;
    noncanonical_pc_o =
      valid_i && raw_frontend &&
      (!canonical64(pc_i) || !canonical64(request_last_addr));
    align_fault_o =
      valid_i && raw_frontend &&
      ((frontend_i == POLY_FRONTEND_AARCH64 && pc_i[1:0] != 2'b00) ||
       (frontend_i == POLY_FRONTEND_RISCV && pc_i[0] != 1'b0));
    range_fault_o =
      valid_i && raw_frontend && request_last_addr < pc_i;
    error_o =
      invalid_frontend_o ||
      noncanonical_pc_o ||
      align_fault_o ||
      range_fault_o;

    request_valid_o = valid_i && raw_frontend && !error_o;
    request_addr_o = pc_i;
    request_bytes_o =
      frontend_i == POLY_FRONTEND_AARCH64 ?
        POLY_AARCH64_FETCH_REQUEST_BYTES :
      frontend_i == POLY_FRONTEND_RISCV ?
        POLY_RISCV_FETCH_REQUEST_BYTES :
        3'd0;
  end
endmodule
