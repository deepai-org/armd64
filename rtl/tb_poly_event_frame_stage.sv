`timescale 1ns/1ps

module tb_poly_event_frame_stage;
  localparam logic [31:0] POLY_MODE_X86         = 32'd0;
  localparam logic [31:0] POLY_MODE_RAW_AARCH64 = 32'd1;
  localparam logic [31:0] POLY_MODE_RAW_RISCV   = 32'd2;

  localparam logic [31:0] POLY_TRAP_SYSCALL = 32'd1;
  localparam logic [31:0] POLY_TRAP_BREAK   = 32'd2;
  localparam logic [31:0] POLY_TRAP_IMPORT  = 32'd3;
  localparam logic [31:0] POLY_TRAP_ILLEGAL = 32'd4;

  localparam logic [63:0] POLY_V2_EVENT_MAGIC = 64'h32545645594c4f50;
  localparam logic [63:0] POLY_V2_EVENT_SIZE_WORD =
    {16'd408, 16'd2, 32'd512};
  localparam logic [63:0] POLY_V2_EVENT_ARG_COUNT_WORD =
    {16'd8, 16'd0, 32'd0};

  logic valid;
  logic event_enabled;
  logic [63:0] event_frame_addr;
  logic [31:0] event_kind;
  logic [31:0] source_frontend;
  logic [63:0] number;
  logic [63:0] selector;
  logic [63:0] insn_pc;
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
  logic [9:0] mem_write_bytes;
  logic [4095:0] mem_write_data;
  logic wait_response;
  logic frame_delivered;
  logic fault;
  logic encode_error;
  logic frame_mem_fault;
  logic event_disabled;
  logic noncanonical_frame;
  logic frame_align_fault;
  logic frame_range_fault;
  logic invalid_event_kind;
  logic invalid_source_frontend;

  poly_event_frame_stage dut (
    .valid_i(valid),
    .event_enabled_i(event_enabled),
    .event_frame_addr_i(event_frame_addr),
    .event_kind_i(event_kind),
    .source_frontend_i(source_frontend),
    .number_i(number),
    .selector_i(selector),
    .insn_pc_i(insn_pc),
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
    .mem_write_data_o(mem_write_data),
    .wait_response_o(wait_response),
    .frame_delivered_o(frame_delivered),
    .fault_o(fault),
    .encode_error_o(encode_error),
    .frame_mem_fault_o(frame_mem_fault),
    .event_disabled_o(event_disabled),
    .noncanonical_frame_o(noncanonical_frame),
    .frame_align_fault_o(frame_align_fault),
    .frame_range_fault_o(frame_range_fault),
    .invalid_event_kind_o(invalid_event_kind),
    .invalid_source_frontend_o(invalid_source_frontend)
  );

  function automatic logic [63:0] frame_qword(input int index);
    begin
      frame_qword = mem_write_data[(index * 64) +: 64];
    end
  endfunction

  task automatic clear_inputs;
    begin
      valid = 1'b0;
      event_enabled = 1'b1;
      event_frame_addr = 64'h457000;
      event_kind = POLY_TRAP_SYSCALL;
      source_frontend = POLY_MODE_RAW_AARCH64;
      number = 64'd172;
      selector = 64'd0;
      insn_pc = 64'h4000;
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
      check(!fault && !encode_error && !frame_mem_fault, "no fault flags");
      check(!event_disabled && !noncanonical_frame, "no enable/canonical fault");
      check(!frame_align_fault && !frame_range_fault, "no address fault");
      check(!invalid_event_kind && !invalid_source_frontend, "no metadata fault");
    end
  endtask

  initial begin
    clear_inputs();
    #1;
    check(!mem_write_valid && !wait_response, "idle no memory write");
    check(!frame_delivered && !fault, "idle no delivery or fault");

    clear_inputs();
    valid = 1'b1;
    mem_write_resp_valid = 1'b0;
    #1;
    check(mem_write_valid && wait_response, "valid frame waits for memory response");
    check(!frame_delivered, "waiting frame not delivered");
    check_no_fault_flags();
    check(mem_write_addr == 64'h457000, "frame address");
    check(mem_write_bytes == 10'd512, "frame byte count");
    check(frame_qword(0) == POLY_V2_EVENT_MAGIC, "event magic");
    check(frame_qword(1) == POLY_V2_EVENT_SIZE_WORD, "event size/version word");
    check(frame_qword(3) == {16'd0, POLY_TRAP_SYSCALL[15:0], POLY_MODE_RAW_AARCH64},
      "event frontend/kind word");
    check(frame_qword(5) == POLY_V2_EVENT_ARG_COUNT_WORD, "event arg count word");
    check(frame_qword(6) == 64'h4000, "insn pc");
    check(frame_qword(7) == 64'h4004, "resume pc");
    check(frame_qword(8) == 64'h4004, "fallthrough pc");
    check(frame_qword(14) == 64'd0, "selector");
    check(frame_qword(15) == 64'h100, "arg0");
    check(frame_qword(22) == 64'h107, "arg7");
    check(frame_qword(24) == 64'd172, "opaque number");

    clear_inputs();
    valid = 1'b1;
    event_kind = POLY_TRAP_IMPORT;
    source_frontend = POLY_MODE_RAW_RISCV;
    number = 64'd5;
    selector = 64'd9;
    insn_pc = 64'h8000;
    resume_pc = 64'h8002;
    mem_write_resp_valid = 1'b1;
    mem_write_fault = 1'b0;
    #1;
    check(mem_write_valid && frame_delivered, "frame delivered on clean response");
    check(!wait_response, "delivered frame does not wait");
    check_no_fault_flags();
    check(frame_qword(3) == {16'd0, POLY_TRAP_IMPORT[15:0], POLY_MODE_RAW_RISCV},
      "import frontend/kind word");
    check(frame_qword(14) == 64'd9, "import selector");
    check(frame_qword(24) == 64'd5, "import number");

    clear_inputs();
    valid = 1'b1;
    event_kind = POLY_TRAP_BREAK;
    mem_write_resp_valid = 1'b1;
    mem_write_fault = 1'b1;
    #1;
    check(mem_write_valid, "faulting frame still writes request");
    check(fault && frame_mem_fault, "memory response fault");
    check(!frame_delivered && !wait_response, "memory fault not delivered or waiting");
    check(!encode_error, "memory fault not encode error");

    clear_inputs();
    valid = 1'b1;
    event_enabled = 1'b0;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && event_disabled, "disabled event frame faults");
    check(!mem_write_valid && !frame_delivered, "disabled event frame blocks write");

    clear_inputs();
    valid = 1'b1;
    event_frame_addr = 64'h457008;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && frame_align_fault, "unaligned frame faults");
    check(!mem_write_valid, "unaligned frame blocks write");

    clear_inputs();
    valid = 1'b1;
    event_frame_addr = 64'h0000800000000000;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && noncanonical_frame, "noncanonical frame faults");
    check(!mem_write_valid, "noncanonical frame blocks write");

    clear_inputs();
    valid = 1'b1;
    event_frame_addr = 64'hfffffffffffffe40;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && frame_range_fault, "range wrap frame faults");
    check(!mem_write_valid, "range wrap frame blocks write");

    clear_inputs();
    valid = 1'b1;
    event_kind = 32'd99;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && invalid_event_kind, "invalid event kind faults");
    check(!mem_write_valid, "invalid event kind blocks write");

    clear_inputs();
    valid = 1'b1;
    source_frontend = POLY_MODE_X86;
    mem_write_resp_valid = 1'b1;
    #1;
    check(fault && encode_error && invalid_source_frontend,
      "invalid source frontend faults");
    check(!mem_write_valid, "invalid source frontend blocks write");

    $display("POLY_RTL_EVENT_FRAME_STAGE_SIM_OK");
    $finish;
  end
endmodule
