`timescale 1ns/1ps

module tb_poly_raw_fetch_path;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic req_valid_i;
  logic [1:0] req_frontend;
  logic [63:0] req_pc;
  logic request_valid;
  logic [63:0] request_addr;
  logic [2:0] request_bytes;
  logic request_error;
  logic invalid_frontend;
  logic noncanonical_pc;
  logic align_fault;
  logic range_fault;

  logic resp_valid_i;
  logic [1:0] resp_frontend;
  logic [63:0] resp_pc;
  logic resp_request_valid;
  logic mem_resp_valid;
  logic mem_resp_fault;
  logic [31:0] mem_resp_word;
  logic wait_response;
  logic insn_valid;
  logic [31:0] insn;
  logic [2:0] insn_bytes;
  logic [63:0] next_pc;
  logic response_fault;
  logic mem_fault;
  logic response_align_fault;

  poly_raw_fetch_request request_dut (
    .valid_i(req_valid_i),
    .frontend_i(req_frontend),
    .pc_i(req_pc),
    .request_valid_o(request_valid),
    .request_addr_o(request_addr),
    .request_bytes_o(request_bytes),
    .error_o(request_error),
    .invalid_frontend_o(invalid_frontend),
    .noncanonical_pc_o(noncanonical_pc),
    .align_fault_o(align_fault),
    .range_fault_o(range_fault)
  );

  poly_raw_fetch_response_stage response_dut (
    .valid_i(resp_valid_i),
    .frontend_i(resp_frontend),
    .pc_i(resp_pc),
    .request_valid_i(resp_request_valid),
    .mem_resp_valid_i(mem_resp_valid),
    .mem_resp_fault_i(mem_resp_fault),
    .mem_resp_word_i(mem_resp_word),
    .wait_response_o(wait_response),
    .insn_valid_o(insn_valid),
    .insn_o(insn),
    .insn_bytes_o(insn_bytes),
    .next_pc_o(next_pc),
    .fault_o(response_fault),
    .mem_fault_o(mem_fault),
    .response_align_fault_o(response_align_fault)
  );

  task automatic clear_request;
    begin
      req_valid_i = 1'b0;
      req_frontend = POLY_FRONTEND_X86;
      req_pc = 64'd0;
    end
  endtask

  task automatic clear_response;
    begin
      resp_valid_i = 1'b0;
      resp_frontend = POLY_FRONTEND_X86;
      resp_pc = 64'd0;
      resp_request_valid = 1'b0;
      mem_resp_valid = 1'b0;
      mem_resp_fault = 1'b0;
      mem_resp_word = 32'd0;
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

  task automatic check_request_no_error;
    begin
      check(!request_error && !invalid_frontend, "request no frontend error");
      check(!noncanonical_pc && !align_fault && !range_fault, "request no address error");
    end
  endtask

  initial begin
    clear_request();
    clear_response();
    #1;
    check(!request_valid && !request_error, "idle request no action");
    check(request_bytes == 3'd0, "idle request bytes zero");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_X86;
    req_pc = 64'h1000;
    #1;
    check(!request_valid && !request_error, "x86 does not issue raw request");
    check(request_addr == 64'h1000 && request_bytes == 3'd0, "x86 request geometry");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_AARCH64;
    req_pc = 64'h4000;
    #1;
    check(request_valid, "aarch64 raw request valid");
    check(request_addr == 64'h4000 && request_bytes == 3'd4, "aarch64 request geometry");
    check_request_no_error();

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_RISCV;
    req_pc = 64'h8002;
    #1;
    check(request_valid, "riscv raw request valid");
    check(request_addr == 64'h8002 && request_bytes == 3'd4, "riscv request geometry");
    check_request_no_error();

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = 2'd3;
    req_pc = 64'h4000;
    #1;
    check(!request_valid && request_error && invalid_frontend, "invalid frontend faults");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_AARCH64;
    req_pc = 64'h0000800000000000;
    #1;
    check(!request_valid && request_error && noncanonical_pc, "noncanonical start faults");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_RISCV;
    req_pc = 64'h00007ffffffffffe;
    #1;
    check(!request_valid && request_error && noncanonical_pc, "noncanonical end faults");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_AARCH64;
    req_pc = 64'h4002;
    #1;
    check(!request_valid && request_error && align_fault, "aarch64 alignment faults");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_RISCV;
    req_pc = 64'h8001;
    #1;
    check(!request_valid && request_error && align_fault, "riscv alignment faults");

    clear_request();
    req_valid_i = 1'b1;
    req_frontend = POLY_FRONTEND_RISCV;
    req_pc = 64'hfffffffffffffffe;
    #1;
    check(!request_valid && request_error && range_fault, "range wrap faults");

    clear_response();
    resp_valid_i = 1'b1;
    resp_frontend = POLY_FRONTEND_AARCH64;
    resp_pc = 64'h4000;
    resp_request_valid = 1'b1;
    mem_resp_valid = 1'b0;
    #1;
    check(wait_response && !insn_valid && !response_fault, "response waits for memory");

    clear_response();
    resp_valid_i = 1'b1;
    resp_frontend = POLY_FRONTEND_AARCH64;
    resp_pc = 64'h4000;
    resp_request_valid = 1'b1;
    mem_resp_valid = 1'b1;
    mem_resp_word = 32'h52800000;
    #1;
    check(!wait_response && insn_valid, "aarch64 response insn valid");
    check(insn == 32'h52800000 && insn_bytes == 3'd4, "aarch64 response insn");
    check(next_pc == 64'h4004 && !response_fault, "aarch64 response next pc");

    clear_response();
    resp_valid_i = 1'b1;
    resp_frontend = POLY_FRONTEND_RISCV;
    resp_pc = 64'h8000;
    resp_request_valid = 1'b1;
    mem_resp_valid = 1'b1;
    mem_resp_word = 32'hffff0001;
    #1;
    check(insn_valid, "riscv16 response insn valid");
    check(insn == 32'h00000001 && insn_bytes == 3'd2, "riscv16 response insn");
    check(next_pc == 64'h8002 && !response_fault, "riscv16 response next pc");

    clear_response();
    resp_valid_i = 1'b1;
    resp_frontend = POLY_FRONTEND_RISCV;
    resp_pc = 64'h8000;
    resp_request_valid = 1'b1;
    mem_resp_valid = 1'b1;
    mem_resp_word = 32'h00000033;
    #1;
    check(insn_valid, "riscv32 response insn valid");
    check(insn == 32'h00000033 && insn_bytes == 3'd4, "riscv32 response insn");
    check(next_pc == 64'h8004 && !response_fault, "riscv32 response next pc");

    clear_response();
    resp_valid_i = 1'b1;
    resp_frontend = POLY_FRONTEND_RISCV;
    resp_pc = 64'h8000;
    resp_request_valid = 1'b1;
    mem_resp_valid = 1'b1;
    mem_resp_fault = 1'b1;
    #1;
    check(response_fault && mem_fault, "memory response fault");
    check(!insn_valid && !response_align_fault, "memory fault blocks instruction");

    clear_response();
    resp_valid_i = 1'b1;
    resp_frontend = POLY_FRONTEND_AARCH64;
    resp_pc = 64'h4002;
    resp_request_valid = 1'b1;
    mem_resp_valid = 1'b1;
    mem_resp_word = 32'h52800000;
    #1;
    check(response_fault && response_align_fault, "response alignment fault");
    check(!insn_valid && !mem_fault, "response align blocks instruction");

    $display("POLY_RTL_RAW_FETCH_PATH_SIM_OK");
    $finish;
  end
endmodule
