`timescale 1ns/1ps

module tb_poly_raw_data_mem_request;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic valid_i;
  logic [1:0] frontend_i;
  logic [63:0] addr_i;
  logic load_i;
  logic store_i;
  logic atomic_i;
  logic [3:0] access_bytes_i;

  logic request_valid_o;
  logic [63:0] request_addr_o;
  logic [3:0] request_bytes_o;
  logic request_load_o;
  logic request_store_o;
  logic request_atomic_o;
  logic error_o;
  logic invalid_frontend_o;
  logic invalid_op_o;
  logic invalid_width_o;
  logic noncanonical_addr_o;
  logic align_fault_o;
  logic range_fault_o;

  poly_raw_data_mem_request dut (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .addr_i(addr_i),
    .load_i(load_i),
    .store_i(store_i),
    .atomic_i(atomic_i),
    .access_bytes_i(access_bytes_i),
    .request_valid_o(request_valid_o),
    .request_addr_o(request_addr_o),
    .request_bytes_o(request_bytes_o),
    .request_load_o(request_load_o),
    .request_store_o(request_store_o),
    .request_atomic_o(request_atomic_o),
    .error_o(error_o),
    .invalid_frontend_o(invalid_frontend_o),
    .invalid_op_o(invalid_op_o),
    .invalid_width_o(invalid_width_o),
    .noncanonical_addr_o(noncanonical_addr_o),
    .align_fault_o(align_fault_o),
    .range_fault_o(range_fault_o)
  );

  task automatic clear_inputs;
    begin
      valid_i = 1'b0;
      frontend_i = POLY_FRONTEND_X86;
      addr_i = 64'd0;
      load_i = 1'b0;
      store_i = 1'b0;
      atomic_i = 1'b0;
      access_bytes_i = 4'd0;
    end
  endtask

  task automatic check(input logic condition, input [1023:0] message);
    begin
      if (!condition) begin
        $display("FAIL: %0s", message);
        $fatal;
      end
    end
  endtask

  initial begin
    clear_inputs();
    #1;
    check(!request_valid_o && !error_o, "idle data request no action");

    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_AARCH64;
    addr_i = 64'h4000;
    load_i = 1'b1;
    access_bytes_i = 4'd8;
    #1;
    check(request_valid_o && !error_o, "aarch64 load request valid");
    check(request_addr_o == 64'h4000 && request_bytes_o == 4'd8,
      "aarch64 load request geometry");
    check(request_load_o && !request_store_o && !request_atomic_o,
      "aarch64 load request op");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_RISCV;
    addr_i = 64'h8004;
    store_i = 1'b1;
    access_bytes_i = 4'd4;
    #1;
    check(request_valid_o && request_store_o && request_bytes_o == 4'd4,
      "riscv store request valid");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_RISCV;
    addr_i = 64'h8010;
    atomic_i = 1'b1;
    access_bytes_i = 4'd8;
    #1;
    check(request_valid_o && request_atomic_o && request_bytes_o == 4'd8,
      "riscv atomic request valid");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = 2'd3;
    addr_i = 64'h4000;
    load_i = 1'b1;
    access_bytes_i = 4'd8;
    #1;
    check(!request_valid_o && error_o && invalid_frontend_o,
      "invalid frontend faults");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_AARCH64;
    addr_i = 64'h4000;
    access_bytes_i = 4'd8;
    #1;
    check(!request_valid_o && error_o && invalid_op_o, "missing op faults");

    load_i = 1'b1;
    store_i = 1'b1;
    #1;
    check(!request_valid_o && error_o && invalid_op_o, "multiple ops fault");

    store_i = 1'b0;
    access_bytes_i = 4'd3;
    #1;
    check(!request_valid_o && error_o && invalid_width_o, "bad width faults");

    access_bytes_i = 4'd8;
    addr_i = 64'h4002;
    #1;
    check(request_valid_o && !error_o && request_load_o,
      "unaligned scalar load remains a request");

    load_i = 1'b0;
    atomic_i = 1'b1;
    #1;
    check(!request_valid_o && error_o && align_fault_o, "atomic alignment faults");

    addr_i = 64'h0000800000000000;
    #1;
    check(!request_valid_o && error_o && noncanonical_addr_o,
      "noncanonical start faults");

    addr_i = 64'h00007ffffffffffc;
    #1;
    check(!request_valid_o && error_o && noncanonical_addr_o,
      "noncanonical end faults");

    addr_i = 64'hfffffffffffffffc;
    #1;
    check(!request_valid_o && error_o && range_fault_o, "range wrap faults");

    $display("POLY_RTL_RAW_DATA_MEM_REQUEST_SIM_OK");
    $finish;
  end
endmodule
