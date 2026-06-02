// Raw foreign data-memory request geometry.
//
// This block validates an execute-produced effective address and decoded raw
// load/store/atomic metadata before issuing a data-memory request. Scalar
// unaligned loads/stores are allowed through so a downstream memory system can
// handle or split them; unaligned atomics fault here. This block does not
// translate addresses, walk page tables, apply permissions, or implement OS
// policy; those belong to the memory system behind this boundary.
module poly_raw_data_mem_request (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] addr_i,
    input  logic        load_i,
    input  logic        store_i,
    input  logic        atomic_i,
    input  logic [3:0]  access_bytes_i,

    output logic        request_valid_o,
    output logic [63:0] request_addr_o,
    output logic [3:0]  request_bytes_o,
    output logic        request_load_o,
    output logic        request_store_o,
    output logic        request_atomic_o,

    output logic        error_o,
    output logic        invalid_frontend_o,
    output logic        invalid_op_o,
    output logic        invalid_width_o,
    output logic        noncanonical_addr_o,
    output logic        align_fault_o,
    output logic        range_fault_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic raw_frontend;
  logic one_op;
  logic valid_width;
  logic [63:0] last_offset;
  logic [63:0] request_last_addr;

  function automatic logic canonical64(input logic [63:0] addr);
    begin
      canonical64 = addr[63:48] == {16{addr[47]}};
    end
  endfunction

  function automatic logic naturally_aligned(
      input logic [63:0] addr,
      input logic [3:0] bytes
  );
    begin
      unique case (bytes)
        4'd1: naturally_aligned = 1'b1;
        4'd2: naturally_aligned = addr[0] == 1'b0;
        4'd4: naturally_aligned = addr[1:0] == 2'b00;
        4'd8: naturally_aligned = addr[2:0] == 3'b000;
        default: naturally_aligned = 1'b0;
      endcase
    end
  endfunction

  always_comb begin
    raw_frontend =
      frontend_i == POLY_FRONTEND_AARCH64 ||
      frontend_i == POLY_FRONTEND_RISCV;
    one_op = (load_i ^ store_i ^ atomic_i) && !(load_i && store_i && atomic_i);
    valid_width =
      access_bytes_i == 4'd1 ||
      access_bytes_i == 4'd2 ||
      access_bytes_i == 4'd4 ||
      access_bytes_i == 4'd8;
    last_offset = valid_width ? {60'd0, access_bytes_i} - 64'd1 : 64'd0;
    request_last_addr = addr_i + last_offset;

    invalid_frontend_o =
      valid_i && frontend_i != POLY_FRONTEND_X86 && !raw_frontend;
    invalid_op_o = valid_i && raw_frontend && !one_op;
    invalid_width_o = valid_i && raw_frontend && one_op && !valid_width;
    noncanonical_addr_o =
      valid_i && raw_frontend && one_op && valid_width &&
      (!canonical64(addr_i) || !canonical64(request_last_addr));
    range_fault_o =
      valid_i && raw_frontend && one_op && valid_width &&
      request_last_addr < addr_i;
    align_fault_o =
      valid_i && raw_frontend && one_op && valid_width && atomic_i &&
      !naturally_aligned(addr_i, access_bytes_i);
    error_o =
      invalid_frontend_o ||
      invalid_op_o ||
      invalid_width_o ||
      noncanonical_addr_o ||
      align_fault_o ||
      range_fault_o;

    request_valid_o = valid_i && raw_frontend && !error_o;
    request_addr_o = addr_i;
    request_bytes_o = request_valid_o ? access_bytes_i : 4'd0;
    request_load_o = request_valid_o && load_i;
    request_store_o = request_valid_o && store_i;
    request_atomic_o = request_valid_o && atomic_i;
  end
endmodule
