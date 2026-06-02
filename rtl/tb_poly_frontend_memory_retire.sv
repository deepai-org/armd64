`timescale 1ns/1ps

module tb_poly_frontend_memory_retire;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PSWITCH_MODE = 7'h04;
  localparam logic [6:0] POLY_X86_CTRL_TRAP_RETURN = 7'h62;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE = 7'h78;

  logic valid_i;
  logic [1:0] frontend;
  logic [63:0] pc;
  logic x86_fetch_valid;
  logic x86_fetch_fault_i;
  logic [31:0] x86_fetch_word;
  logic [63:0] x86_fallthrough_pc;
  logic raw_mem_resp_valid;
  logic raw_mem_resp_fault;
  logic [31:0] raw_mem_resp_word;
  logic older_fault_i;
  logic execute_ready;
  logic block_retire;
  logic execute_fault_i;
  logic [1:0] target_frontend;
  logic [63:0] target_pc;
  logic signature_slot_valid;
  logic transition_stack_full;
  logic trap_return_restore_valid;

  logic x86_fetch_req_valid;
  logic [63:0] x86_fetch_req_addr;
  logic [4:0] x86_fetch_req_bytes;
  logic raw_mem_req_valid;
  logic [63:0] raw_mem_req_addr;
  logic [2:0] raw_mem_req_bytes;
  logic wait_fetch;
  logic wait_execute;
  logic wait_retire;
  logic retire;
  logic commit_transition;
  logic commit_push_transition;
  logic [1:0] commit_frontend;
  logic [63:0] commit_pc;
  logic [6:0] commit_signature_slot;
  logic trap_return_decode;
  logic trap_return_retire;
  logic fault;
  logic [63:0] fault_pc;
  logic older_fault;
  logic fetch_fault;
  logic execute_fault;
  logic control_fault;
  logic invalid_frontend;
  logic x86_fetch_wait;
  logic x86_request_error;
  logic x86_mem_fault;
  logic x86_noncanonical_pc;
  logic x86_range_fault;
  logic poly_ctrl;
  logic [6:0] subop;
  logic raw_fetch_wait;
  logic raw_request_error;
  logic raw_mem_fault;
  logic raw_noncanonical_pc;
  logic raw_align_fault;
  logic raw_range_fault;
  logic invalid_subop;
  logic noncanonical_target;
  logic target_align_fault;
  logic invalid_signature_slot;
  logic transition_stack_full_o;

  poly_frontend_memory_retire dut (
    .valid_i(valid_i),
    .frontend_i(frontend),
    .pc_i(pc),
    .x86_fetch_valid_i(x86_fetch_valid),
    .x86_fetch_fault_i(x86_fetch_fault_i),
    .x86_fetch_word_i(x86_fetch_word),
    .x86_fallthrough_pc_i(x86_fallthrough_pc),
    .raw_mem_resp_valid_i(raw_mem_resp_valid),
    .raw_mem_resp_fault_i(raw_mem_resp_fault),
    .raw_mem_resp_word_i(raw_mem_resp_word),
    .older_fault_i(older_fault_i),
    .execute_ready_i(execute_ready),
    .block_retire_i(block_retire),
    .execute_fault_i(execute_fault_i),
    .target_frontend_i(target_frontend),
    .target_pc_i(target_pc),
    .signature_slot_valid_i(signature_slot_valid),
    .transition_stack_full_i(transition_stack_full),
    .trap_return_restore_valid_i(trap_return_restore_valid),
    .x86_fetch_req_valid_o(x86_fetch_req_valid),
    .x86_fetch_req_addr_o(x86_fetch_req_addr),
    .x86_fetch_req_bytes_o(x86_fetch_req_bytes),
    .raw_mem_req_valid_o(raw_mem_req_valid),
    .raw_mem_req_addr_o(raw_mem_req_addr),
    .raw_mem_req_bytes_o(raw_mem_req_bytes),
    .wait_fetch_o(wait_fetch),
    .wait_execute_o(wait_execute),
    .wait_retire_o(wait_retire),
    .retire_o(retire),
    .commit_transition_o(commit_transition),
    .commit_push_transition_o(commit_push_transition),
    .commit_frontend_o(commit_frontend),
    .commit_pc_o(commit_pc),
    .commit_signature_slot_o(commit_signature_slot),
    .trap_return_decode_o(trap_return_decode),
    .trap_return_retire_o(trap_return_retire),
    .fault_o(fault),
    .fault_pc_o(fault_pc),
    .older_fault_o(older_fault),
    .fetch_fault_o(fetch_fault),
    .execute_fault_o(execute_fault),
    .control_fault_o(control_fault),
    .invalid_frontend_o(invalid_frontend),
    .x86_fetch_wait_o(x86_fetch_wait),
    .x86_request_error_o(x86_request_error),
    .x86_mem_fault_o(x86_mem_fault),
    .x86_noncanonical_pc_o(x86_noncanonical_pc),
    .x86_range_fault_o(x86_range_fault),
    .poly_ctrl_o(poly_ctrl),
    .subop_o(subop),
    .raw_insn_valid_o(),
    .raw_memory_order_valid_o(),
    .raw_memory_load_o(),
    .raw_memory_store_o(),
    .raw_memory_atomic_o(),
    .raw_memory_barrier_o(),
    .raw_memory_access_bytes_o(),
    .raw_branch_o(),
    .raw_call_o(),
    .raw_return_o(),
    .raw_trap_o(),
    .raw_branch_target_valid_o(),
    .raw_branch_target_o(),
    .raw_fetch_wait_o(raw_fetch_wait),
    .raw_request_error_o(raw_request_error),
    .raw_mem_fault_o(raw_mem_fault),
    .raw_noncanonical_pc_o(raw_noncanonical_pc),
    .raw_align_fault_o(raw_align_fault),
    .raw_range_fault_o(raw_range_fault),
    .invalid_subop_o(invalid_subop),
    .noncanonical_target_o(noncanonical_target),
    .target_align_fault_o(target_align_fault),
    .invalid_signature_slot_o(invalid_signature_slot),
    .transition_stack_full_o(transition_stack_full_o)
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

  task automatic clear_inputs;
    begin
      valid_i = 1'b0;
      frontend = POLY_FRONTEND_X86;
      pc = 64'd0;
      x86_fetch_valid = 1'b0;
      x86_fetch_fault_i = 1'b0;
      x86_fetch_word = 32'd0;
      x86_fallthrough_pc = 64'd0;
      raw_mem_resp_valid = 1'b0;
      raw_mem_resp_fault = 1'b0;
      raw_mem_resp_word = 32'd0;
      older_fault_i = 1'b0;
      execute_ready = 1'b0;
      block_retire = 1'b0;
      execute_fault_i = 1'b0;
      target_frontend = POLY_FRONTEND_X86;
      target_pc = 64'd0;
      signature_slot_valid = 1'b0;
      transition_stack_full = 1'b0;
      trap_return_restore_valid = 1'b0;
    end
  endtask

  task automatic setup_x86_switch;
    begin
      valid_i = 1'b1;
      frontend = POLY_FRONTEND_X86;
      pc = 64'h1000;
      x86_fetch_valid = 1'b1;
      x86_fetch_word = x86_ctrl(POLY_X86_CTRL_PSWITCH_MODE);
      x86_fallthrough_pc = 64'h1004;
      execute_ready = 1'b1;
      target_frontend = POLY_FRONTEND_AARCH64;
      target_pc = 64'h4000;
      signature_slot_valid = 1'b1;
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
    check(!x86_fetch_req_valid && !raw_mem_req_valid, "idle issues no request");
    check(!retire && !fault && !wait_fetch, "idle has no event");

    clear_inputs();
    setup_x86_switch();
    #1;
    check(x86_fetch_req_valid && !raw_mem_req_valid, "x86 switch issues x86 fetch");
    check(x86_fetch_req_addr == 64'h1000 && x86_fetch_req_bytes == 5'd16,
      "x86 switch request geometry");
    check(retire && !fault && commit_transition, "x86 switch retires");
    check(!commit_push_transition, "x86 switch does not push");
    check(commit_frontend == POLY_FRONTEND_AARCH64 && commit_pc == 64'h4000,
      "x86 switch target commits");
    check(poly_ctrl && subop == POLY_X86_CTRL_PSWITCH_MODE, "x86 switch metadata");

    clear_inputs();
    setup_x86_switch();
    x86_fetch_word = x86_ctrl(POLY_X86_CTRL_TRAP_RETURN);
    trap_return_restore_valid = 1'b0;
    #1;
    check(wait_execute && trap_return_decode && !trap_return_retire,
      "x86 trap return waits for restore target");
    check(!retire && !fault && !commit_transition,
      "x86 trap return restore wait blocks retire");

    trap_return_restore_valid = 1'b1;
    #1;
    check(retire && trap_return_decode && trap_return_retire && !fault,
      "x86 trap return retires with restore target");
    check(!commit_transition && !commit_push_transition,
      "x86 trap return does not push transition");

    clear_inputs();
    setup_x86_switch();
    x86_fetch_valid = 1'b0;
    #1;
    check(x86_fetch_req_valid && x86_fetch_wait && wait_fetch,
      "x86 fetch wait propagates");
    check(!retire && !fault && !commit_transition, "x86 fetch wait blocks retire");

    clear_inputs();
    setup_x86_switch();
    x86_fetch_fault_i = 1'b1;
    #1;
    check(fault && fetch_fault && x86_mem_fault, "x86 fetch fault propagates");
    check(!retire && !commit_transition && fault_pc == 64'h1000,
      "x86 fetch fault blocks commit");

    clear_inputs();
    setup_x86_switch();
    execute_ready = 1'b0;
    #1;
    check(wait_execute && !wait_fetch && !wait_retire, "memory-retire waits for execute");
    check(!retire && !fault, "execute wait has no fault");

    clear_inputs();
    setup_x86_switch();
    block_retire = 1'b1;
    #1;
    check(wait_retire && !wait_fetch && !wait_execute, "memory-retire waits for retire");
    check(!retire && !fault, "retire wait has no fault");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_AARCH64;
    pc = 64'h4000;
    raw_mem_resp_valid = 1'b0;
    execute_ready = 1'b1;
    target_frontend = POLY_FRONTEND_X86;
    target_pc = 64'h1000;
    signature_slot_valid = 1'b1;
    #1;
    check(raw_mem_req_valid && raw_fetch_wait && wait_fetch,
      "raw fetch wait propagates");
    check(!x86_fetch_req_valid && !retire && !fault, "raw wait blocks retire");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_AARCH64;
    pc = 64'h4000;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_word = 32'h52800000;
    execute_ready = 1'b1;
    target_frontend = POLY_FRONTEND_X86;
    target_pc = 64'h1000;
    signature_slot_valid = 1'b1;
    #1;
    check(raw_mem_req_valid && !x86_fetch_req_valid, "raw non-control issues raw fetch");
    check(retire && !fault && !commit_transition, "raw non-control retires");
    check(commit_frontend == POLY_FRONTEND_AARCH64 && commit_pc == 64'h4004,
      "raw non-control fallthrough commits");
    check(!poly_ctrl, "raw non-control metadata suppressed");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_AARCH64;
    pc = 64'h4000;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_word = aarch64_ctrl(POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE);
    execute_ready = 1'b1;
    target_frontend = POLY_FRONTEND_RISCV;
    target_pc = 64'h8000;
    signature_slot_valid = 1'b1;
    #1;
    check(retire && !fault && commit_transition, "raw switch retires");
    check(commit_frontend == POLY_FRONTEND_RISCV && commit_pc == 64'h8000,
      "raw switch target commits");

    clear_inputs();
    valid_i = 1'b1;
    frontend = POLY_FRONTEND_RISCV;
    pc = 64'h8000;
    raw_mem_resp_valid = 1'b1;
    raw_mem_resp_fault = 1'b1;
    execute_ready = 1'b1;
    target_frontend = POLY_FRONTEND_X86;
    target_pc = 64'h1000;
    signature_slot_valid = 1'b1;
    #1;
    check(fault && fetch_fault && raw_mem_fault, "raw memory fault propagates");
    check(!retire && !commit_transition, "raw memory fault blocks commit");

    clear_inputs();
    setup_x86_switch();
    target_frontend = 2'd3;
    #1;
    check(fault && control_fault && invalid_frontend, "invalid target frontend faults");
    check(!retire && !commit_transition, "invalid target blocks transition");

    clear_inputs();
    valid_i = 1'b1;
    frontend = 2'd3;
    pc = 64'h1000;
    x86_fetch_valid = 1'b1;
    execute_ready = 1'b1;
    target_frontend = POLY_FRONTEND_X86;
    target_pc = 64'h2000;
    signature_slot_valid = 1'b1;
    #1;
    check(fault && fetch_fault && invalid_frontend, "invalid active frontend faults");
    check(!x86_fetch_req_valid && !raw_mem_req_valid && !retire,
      "invalid active frontend issues no fetch");

    clear_inputs();
    setup_x86_switch();
    pc = 64'h0000800000000000;
    #1;
    check(fault && fetch_fault && x86_request_error && x86_noncanonical_pc,
      "x86 noncanonical request faults");
    check(!x86_fetch_req_valid && !retire, "x86 noncanonical blocks fetch");

    $display("POLY_RTL_FRONTEND_MEMORY_RETIRE_SIM_OK");
    $finish;
  end
endmodule
