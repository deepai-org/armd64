`timescale 1ns/1ps

module tb_poly_trap_packet_stage;
  localparam logic [31:0] POLY_MODE_X86         = 32'd0;
  localparam logic [31:0] POLY_MODE_RAW_AARCH64 = 32'd1;
  localparam logic [31:0] POLY_MODE_RAW_RISCV   = 32'd2;

  localparam logic [31:0] POLY_TRAP_SYSCALL = 32'd1;
  localparam logic [31:0] POLY_TRAP_BREAK   = 32'd2;
  localparam logic [31:0] POLY_TRAP_IMPORT  = 32'd3;
  localparam logic [31:0] POLY_TRAP_ILLEGAL = 32'd4;

  localparam logic [63:0] POLY_TRAP_PACKET_REQUIRED_FLAGS = 64'h7f;

  logic valid;
  logic monitor_enabled;
  logic [63:0] monitor_packet_addr;
  logic [31:0] reason;
  logic [31:0] source_mode;
  logic [63:0] number;
  logic [63:0] selector;
  logic [63:0] trap_pc;
  logic [63:0] resume_pc;
  logic [63:0] arg0;
  logic [63:0] arg1;
  logic [63:0] arg2;
  logic [63:0] arg3;
  logic [63:0] arg4;
  logic [63:0] arg5;
  logic [63:0] arg6;
  logic [63:0] arg7;
  logic mem_write_resp_valid;
  logic mem_write_fault;

  logic mem_write_valid;
  logic [63:0] mem_write_addr;
  logic [7:0] mem_write_bytes;
  logic [63:0] mem_write_qword0;
  logic [63:0] mem_write_qword1;
  logic [63:0] mem_write_qword2;
  logic [63:0] mem_write_qword3;
  logic [63:0] mem_write_qword4;
  logic [63:0] mem_write_qword5;
  logic [63:0] mem_write_qword6;
  logic [63:0] mem_write_qword7;
  logic [63:0] mem_write_qword8;
  logic [63:0] mem_write_qword9;
  logic [63:0] mem_write_qword10;
  logic [63:0] mem_write_qword11;
  logic [63:0] mem_write_qword12;
  logic [63:0] mem_write_qword13;
  logic [63:0] mem_write_qword14;
  logic [63:0] mem_write_qword15;
  logic wait_response;
  logic packet_delivered;
  logic fault;
  logic encode_error;
  logic packet_mem_fault;
  logic monitor_disabled;
  logic noncanonical_packet;
  logic packet_align_fault;
  logic packet_range_fault;
  logic invalid_reason;
  logic invalid_source_mode;

  poly_trap_packet_stage dut (
    .valid_i(valid),
    .monitor_enabled_i(monitor_enabled),
    .monitor_packet_addr_i(monitor_packet_addr),
    .reason_i(reason),
    .source_mode_i(source_mode),
    .number_i(number),
    .selector_i(selector),
    .trap_pc_i(trap_pc),
    .resume_pc_i(resume_pc),
    .arg0_i(arg0),
    .arg1_i(arg1),
    .arg2_i(arg2),
    .arg3_i(arg3),
    .arg4_i(arg4),
    .arg5_i(arg5),
    .arg6_i(arg6),
    .arg7_i(arg7),
    .mem_write_resp_valid_i(mem_write_resp_valid),
    .mem_write_fault_i(mem_write_fault),
    .mem_write_valid_o(mem_write_valid),
    .mem_write_addr_o(mem_write_addr),
    .mem_write_bytes_o(mem_write_bytes),
    .mem_write_qword0_o(mem_write_qword0),
    .mem_write_qword1_o(mem_write_qword1),
    .mem_write_qword2_o(mem_write_qword2),
    .mem_write_qword3_o(mem_write_qword3),
    .mem_write_qword4_o(mem_write_qword4),
    .mem_write_qword5_o(mem_write_qword5),
    .mem_write_qword6_o(mem_write_qword6),
    .mem_write_qword7_o(mem_write_qword7),
    .mem_write_qword8_o(mem_write_qword8),
    .mem_write_qword9_o(mem_write_qword9),
    .mem_write_qword10_o(mem_write_qword10),
    .mem_write_qword11_o(mem_write_qword11),
    .mem_write_qword12_o(mem_write_qword12),
    .mem_write_qword13_o(mem_write_qword13),
    .mem_write_qword14_o(mem_write_qword14),
    .mem_write_qword15_o(mem_write_qword15),
    .wait_response_o(wait_response),
    .packet_delivered_o(packet_delivered),
    .fault_o(fault),
    .encode_error_o(encode_error),
    .packet_mem_fault_o(packet_mem_fault),
    .monitor_disabled_o(monitor_disabled),
    .noncanonical_packet_o(noncanonical_packet),
    .packet_align_fault_o(packet_align_fault),
    .packet_range_fault_o(packet_range_fault),
    .invalid_reason_o(invalid_reason),
    .invalid_source_mode_o(invalid_source_mode)
  );

  task automatic clear_inputs;
    begin
      valid = 1'b0;
      monitor_enabled = 1'b1;
      monitor_packet_addr = 64'h457000;
      reason = POLY_TRAP_SYSCALL;
      source_mode = POLY_MODE_RAW_AARCH64;
      number = 64'd172;
      selector = 64'd0;
      trap_pc = 64'h4000;
      resume_pc = 64'h4004;
      arg0 = 64'h100;
      arg1 = 64'h101;
      arg2 = 64'h102;
      arg3 = 64'h103;
      arg4 = 64'h104;
      arg5 = 64'h105;
      arg6 = 64'h106;
      arg7 = 64'h107;
      mem_write_resp_valid = 1'b0;
      mem_write_fault = 1'b0;
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

  task automatic check_no_fault_flags;
    begin
      check(!fault && !encode_error && !packet_mem_fault, "no fault flags");
      check(!monitor_disabled && !noncanonical_packet, "no monitor/canonical fault");
      check(!packet_align_fault && !packet_range_fault, "no address fault");
      check(!invalid_reason && !invalid_source_mode, "no metadata fault");
    end
  endtask

  initial begin
    clear_inputs();
    #1;
    check(!mem_write_valid && !wait_response, "idle no memory write");
    check(!packet_delivered && !fault, "idle no delivery or fault");

    clear_inputs();
    valid = 1'b1;
    mem_write_resp_valid = 1'b0;
    #1;
    check(mem_write_valid && wait_response, "valid packet waits for memory response");
    check(!packet_delivered, "waiting packet not delivered");
    check_no_fault_flags();
    check(mem_write_addr == 64'h457000, "packet address");
    check(mem_write_bytes == 8'd128, "packet byte count");
    check(mem_write_qword0 == {POLY_MODE_RAW_AARCH64, POLY_TRAP_SYSCALL}, "qword0");
    check(mem_write_qword1 == 64'd172, "qword1 syscall number");
    check(mem_write_qword2 == 64'd0, "qword2 selector");
    check(mem_write_qword3 == 64'h4000, "qword3 trap pc");
    check(mem_write_qword4 == 64'h4004, "qword4 resume pc");
    check(mem_write_qword5 == POLY_TRAP_PACKET_REQUIRED_FLAGS, "qword5 flags");
    check(mem_write_qword6 == 64'd0 && mem_write_qword7 == 64'd0, "reserved qwords");
    check(mem_write_qword8 == 64'h100, "arg0");
    check(mem_write_qword15 == 64'h107, "arg7");

    clear_inputs();
    valid = 1'b1;
    reason = POLY_TRAP_IMPORT;
    source_mode = POLY_MODE_RAW_RISCV;
    number = 64'd5;
    selector = 64'd9;
    trap_pc = 64'h8000;
    resume_pc = 64'h8002;
    mem_write_resp_valid = 1'b1;
    mem_write_fault = 1'b0;
    #1;
    check(mem_write_valid && packet_delivered, "packet delivered on clean response");
    check(!wait_response, "delivered packet does not wait");
    check_no_fault_flags();
    check(mem_write_qword0 == {POLY_MODE_RAW_RISCV, POLY_TRAP_IMPORT}, "import qword0");
    check(mem_write_qword1 == 64'd5, "import number");
    check(mem_write_qword2 == 64'd9, "import selector");

    clear_inputs();
    valid = 1'b1;
    reason = POLY_TRAP_BREAK;
    mem_write_resp_valid = 1'b1;
    mem_write_fault = 1'b1;
    #1;
    check(mem_write_valid, "faulting packet still writes request");
    check(fault && packet_mem_fault, "memory response fault");
    check(!packet_delivered && !wait_response, "memory fault not delivered or waiting");
    check(!encode_error, "memory fault not encode error");

    clear_inputs();
    valid = 1'b1;
    monitor_enabled = 1'b0;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && monitor_disabled, "disabled monitor faults");
    check(!mem_write_valid && !packet_delivered, "disabled monitor blocks write");

    clear_inputs();
    valid = 1'b1;
    monitor_packet_addr = 64'h457001;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && packet_align_fault, "unaligned packet faults");
    check(!mem_write_valid, "unaligned packet blocks write");

    clear_inputs();
    valid = 1'b1;
    monitor_packet_addr = 64'h0000800000000000;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && noncanonical_packet, "noncanonical packet faults");
    check(!mem_write_valid, "noncanonical packet blocks write");

    clear_inputs();
    valid = 1'b1;
    monitor_packet_addr = 64'hffffffffffffffc0;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && packet_range_fault, "range wrap packet faults");
    check(!mem_write_valid, "range wrap packet blocks write");

    clear_inputs();
    valid = 1'b1;
    reason = 32'd99;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && invalid_reason, "invalid trap reason faults");
    check(!mem_write_valid, "invalid reason blocks write");

    clear_inputs();
    valid = 1'b1;
    source_mode = POLY_MODE_X86;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && invalid_source_mode, "invalid source mode faults");
    check(!mem_write_valid, "invalid source mode blocks write");

    $display("POLY_RTL_TRAP_PACKET_STAGE_SIM_OK");
    $finish;
  end
endmodule
