`timescale 1ns/1ps

module tb_poly_frontend_fetch_decode_pipeline;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PSWITCH_MODE = 7'h04;
  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_IMM_BASE = 7'h30;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE = 7'h50;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE = 7'h20;

  logic valid_i;
  logic [1:0] frontend;
  logic [63:0] pc;
  logic x86_fetch_resp_valid;
  logic x86_fetch_resp_fault;
  logic [31:0] x86_fetch_resp_word;
  logic [63:0] x86_fetch_resp_fallthrough_pc;
  logic raw_mem_resp_valid;
  logic raw_mem_resp_fault;
  logic [31:0] raw_mem_resp_word;

  logic x86_fetch_req_valid;
  logic [63:0] x86_fetch_req_addr;
  logic [4:0] x86_fetch_req_bytes;
  logic raw_mem_req_valid;
  logic [63:0] raw_mem_req_addr;
  logic [2:0] raw_mem_req_bytes;
  logic wait_fetch;
  logic fetch_valid;
  logic fetch_fault;
  logic [31:0] fetch_word;
  logic [63:0] fetch_fallthrough_pc;
  logic decode_valid;
  logic poly_ctrl;
  logic [6:0] subop;
  logic call_sig_imm;
  logic [6:0] signature_slot;
  logic invalid_frontend;
  logic x86_fetch_wait;
  logic x86_request_error;
  logic x86_mem_fault;
  logic x86_noncanonical_pc;
  logic x86_range_fault;
  logic raw_fetch_wait;
  logic raw_request_error;
  logic raw_mem_fault;
  logic raw_noncanonical_pc;
  logic raw_align_fault;
  logic raw_range_fault;

  poly_frontend_fetch_decode_pipeline dut (
    .valid_i(valid_i),
    .frontend_i(frontend),
    .pc_i(pc),
    .x86_fetch_resp_valid_i(x86_fetch_resp_valid),
    .x86_fetch_resp_fault_i(x86_fetch_resp_fault),
    .x86_fetch_resp_word_i(x86_fetch_resp_word),
    .x86_fetch_resp_fallthrough_pc_i(x86_fetch_resp_fallthrough_pc),
    .raw_mem_resp_valid_i(raw_mem_resp_valid),
    .raw_mem_resp_fault_i(raw_mem_resp_fault),
    .raw_mem_resp_word_i(raw_mem_resp_word),
    .x86_fetch_req_valid_o(x86_fetch_req_valid),
    .x86_fetch_req_addr_o(x86_fetch_req_addr),
    .x86_fetch_req_bytes_o(x86_fetch_req_bytes),
    .raw_mem_req_valid_o(raw_mem_req_valid),
    .raw_mem_req_addr_o(raw_mem_req_addr),
    .raw_mem_req_bytes_o(raw_mem_req_bytes),
    .wait_fetch_o(wait_fetch),
    .fetch_valid_o(fetch_valid),
    .fetch_fault_o(fetch_fault),
    .fetch_word_o(fetch_word),
    .fetch_fallthrough_pc_o(fetch_fallthrough_pc),
    .decode_valid_o(decode_valid),
    .poly_ctrl_o(poly_ctrl),
    .subop_o(subop),
    .call_sig_imm_o(call_sig_imm),
    .signature_slot_o(signature_slot),
    .invalid_frontend_o(invalid_frontend),
    .x86_fetch_wait_o(x86_fetch_wait),
    .x86_request_error_o(x86_request_error),
    .x86_mem_fault_o(x86_mem_fault),
    .x86_noncanonical_pc_o(x86_noncanonical_pc),
    .x86_range_fault_o(x86_range_fault),
    .raw_fetch_wait_o(raw_fetch_wait),
    .raw_request_error_o(raw_request_error),
    .raw_mem_fault_o(raw_mem_fault),
    .raw_noncanonical_pc_o(raw_noncanonical_pc),
    .raw_align_fault_o(raw_align_fault),
    .raw_range_fault_o(raw_range_fault)
  );

  function automatic logic [31:0] x86_ctrl(input logic [6:0] ctrl_subop);
    begin
      x86_ctrl = {1'b0, ctrl_subop, 8'hfc, 8'h3a, 8'h0f};
    end
  endfunction

  function automatic logic [31:0] aarch64_ctrl(input logic [6:0] ctrl_subop);
    begin
      aarch64_ctrl = 32'hd503201f | ({25'd0, ctrl_subop} << 5);
    end
  endfunction

  function automatic logic [31:0] riscv_ctrl(input logic [6:0] ctrl_subop);
    begin
      riscv_ctrl = 32'h0000700b | ({25'd0, ctrl_subop} << 25);
    end
  endfunction

  task automatic clear_inputs;
    begin
      valid_i = 1'b0;
      frontend = POLY_FRONTEND_X86;
      pc = 64'd0;
      x86_fetch_resp_valid = 1'b0;
      x86_fetch_resp_fault = 1'b0;
      x86_fetch_resp_word = 32'd0;
      x86_fetch_resp_fallthrough_pc = 64'd0;
      raw_mem_resp_valid = 1'b0;
      raw_mem_resp_fault = 1'b0;
      raw_mem_resp_word = 32'd0;
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
    check(!x86_fetch_req_valid && !raw_mem_req_valid, "idle issues no fetch");
    check(!wait_fetch && !fetch_valid && !fetch_fault, "idle has no pipeline event");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_X86;
    pc = 64'h1000;
    x86_fetch_resp_valid = 1'b1;
    x86_fetch_resp_word = x86_ctrl(POLY_X86_CTRL_PSWITCH_MODE);
    x86_fetch_resp_fallthrough_pc = 64'h1004;
    #1;
    check(x86_fetch_req_valid && !raw_mem_req_valid, "x86 issues x86 fetch only");
    check(x86_fetch_req_addr == 64'h1000 && x86_fetch_req_bytes == 5'd16,
      "x86 request geometry");
    check(fetch_valid && !fetch_fault && decode_valid, "x86 fetch decodes");
    check(fetch_word == x86_ctrl(POLY_X86_CTRL_PSWITCH_MODE), "x86 fetch word");
    check(fetch_fallthrough_pc == 64'h1004, "x86 fallthrough propagates");
    check(poly_ctrl && subop == POLY_X86_CTRL_PSWITCH_MODE, "x86 control subop");
    check(!call_sig_imm && signature_slot == 7'd0, "x86 non-signature control");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_X86;
    pc = 64'h2000;
    x86_fetch_resp_valid = 1'b0;
    #1;
    check(x86_fetch_req_valid && x86_fetch_wait && wait_fetch, "x86 waits for fetch");
    check(!fetch_valid && !fetch_fault && !decode_valid, "x86 wait blocks decode");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_X86;
    pc = 64'h3000;
    x86_fetch_resp_valid = 1'b1;
    x86_fetch_resp_fault = 1'b1;
    #1;
    check(fetch_fault && x86_mem_fault, "x86 memory fault propagates");
    check(!fetch_valid && !decode_valid && !poly_ctrl, "x86 memory fault blocks decode");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_AARCH64;
    pc = 64'h4000;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_word = aarch64_ctrl(POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE + 7'd2);
    #1;
    check(raw_mem_req_valid && !x86_fetch_req_valid, "aarch64 issues raw fetch only");
    check(raw_mem_req_addr == 64'h4000 && raw_mem_req_bytes == 3'd4,
      "aarch64 raw request geometry");
    check(fetch_valid && !fetch_fault && decode_valid, "aarch64 raw fetch decodes");
    check(poly_ctrl && call_sig_imm && signature_slot == 7'd2,
      "aarch64 signature control decodes");
    check(subop == POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE + 7'd2,
      "aarch64 signature subop");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_RISCV;
    pc = 64'h8000;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_word = riscv_ctrl(POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE + 7'd3);
    #1;
    check(raw_mem_req_valid && !x86_fetch_req_valid, "riscv issues raw fetch only");
    check(fetch_valid && !fetch_fault && decode_valid, "riscv raw fetch decodes");
    check(poly_ctrl && call_sig_imm && signature_slot == 7'd3,
      "riscv signature control decodes");
    check(subop == POLY_RISCV_CTRL_SUBOP_CALL_SIG_IMM_BASE + 7'd3,
      "riscv signature subop");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_AARCH64;
    pc = 64'h4002;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_word = aarch64_ctrl(POLY_AARCH64_CTRL_SUBOP_CALL_SIG_IMM_BASE);
    #1;
    check(fetch_fault && raw_request_error && raw_align_fault,
      "aarch64 request alignment fault");
    check(!raw_mem_req_valid && !fetch_valid && !decode_valid,
      "aarch64 align fault blocks fetch");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_RISCV;
    pc = 64'h8000;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_fault = 1'b1;
    #1;
    check(fetch_fault && raw_mem_fault, "riscv memory fault propagates");
    check(!fetch_valid && !decode_valid && !poly_ctrl, "riscv memory fault blocks decode");

    clear_inputs();
    valid_i = 1'b1;
    frontend = 2'd3;
    pc = 64'h1000;
    #1;
    check(fetch_fault && invalid_frontend, "invalid frontend faults");
    check(!x86_fetch_req_valid && !raw_mem_req_valid && !fetch_valid,
      "invalid frontend issues no request");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_X86;
    pc = 64'h00007fffffffffff;
    #1;
    check(fetch_fault && x86_request_error && x86_noncanonical_pc,
      "x86 noncanonical fetch window faults");
    check(!x86_fetch_req_valid && !fetch_valid, "x86 address fault blocks fetch");

    $display("POLY_RTL_FRONTEND_FETCH_DECODE_PIPELINE_SIM_OK");
    $finish;
  end
endmodule
