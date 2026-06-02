`timescale 1ns/1ps

module tb_poly_raw_data_mem_response_stage;
  logic valid_i;
  logic request_valid_i;
  logic request_error_i;
  logic mem_resp_valid_i;
  logic mem_resp_fault_i;

  logic wait_response_o;
  logic resolved_o;
  logic fault_o;
  logic request_fault_o;
  logic memory_fault_o;

  poly_raw_data_mem_response_stage dut (
    .valid_i(valid_i),
    .request_valid_i(request_valid_i),
    .request_error_i(request_error_i),
    .mem_resp_valid_i(mem_resp_valid_i),
    .mem_resp_fault_i(mem_resp_fault_i),
    .wait_response_o(wait_response_o),
    .resolved_o(resolved_o),
    .fault_o(fault_o),
    .request_fault_o(request_fault_o),
    .memory_fault_o(memory_fault_o)
  );

  task automatic clear_inputs;
    begin
      valid_i = 1'b0;
      request_valid_i = 1'b0;
      request_error_i = 1'b0;
      mem_resp_valid_i = 1'b0;
      mem_resp_fault_i = 1'b0;
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
    check(!wait_response_o && !resolved_o && !fault_o, "idle response no action");

    valid_i = 1'b1;
    request_valid_i = 1'b1;
    #1;
    check(wait_response_o && !resolved_o && !fault_o,
      "valid request waits for memory response");

    mem_resp_valid_i = 1'b1;
    #1;
    check(!wait_response_o && resolved_o && !fault_o,
      "clean memory response resolves request");

    mem_resp_fault_i = 1'b1;
    #1;
    check(!resolved_o && fault_o && memory_fault_o && !request_fault_o,
      "memory response fault reports memory fault");

    clear_inputs();
    valid_i = 1'b1;
    request_error_i = 1'b1;
    #1;
    check(!wait_response_o && !resolved_o && fault_o && request_fault_o,
      "request validation error faults without memory wait");

    request_valid_i = 1'b1;
    mem_resp_valid_i = 1'b1;
    mem_resp_fault_i = 1'b0;
    #1;
    check(!resolved_o && fault_o && request_fault_o,
      "request validation error wins over clean memory response");

    $display("POLY_RTL_RAW_DATA_MEM_RESPONSE_STAGE_SIM_OK");
    $finish;
  end
endmodule
