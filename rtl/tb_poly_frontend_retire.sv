`timescale 1ns/1ps

module tb_poly_frontend_retire;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_IMM_BASE = 7'h30;
  localparam logic [6:0] POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE = 7'h78;

  logic valid_i;
  logic fetch_valid;
  logic execute_ready;
  logic block_retire;
  logic older_fault_i;
  logic fetch_fault_i;
  logic execute_fault_i;
  logic [1:0] frontend;
  logic [63:0] pc;
  logic [31:0] fetch_word;
  logic [63:0] x86_fallthrough_pc;
  logic [1:0] target_frontend;
  logic [63:0] target_pc;
  logic signature_slot_valid;
  logic transition_stack_full;

  logic wait_fetch;
  logic wait_execute;
  logic wait_retire;
  logic retire;
  logic commit_transition;
  logic commit_push_transition;
  logic [1:0] commit_frontend;
  logic [63:0] commit_pc;
  logic [6:0] commit_signature_slot;
  logic fault;
  logic [63:0] fault_pc;
  logic older_fault;
  logic fetch_fault;
  logic execute_fault;
  logic control_fault;
  logic poly_ctrl;
  logic [6:0] subop;
  logic raw_align_fault;
  logic invalid_subop;
  logic invalid_frontend;
  logic noncanonical_target;
  logic target_align_fault;
  logic invalid_signature_slot;
  logic transition_stack_full_o;

  poly_frontend_retire dut (
    .valid_i(valid_i),
    .fetch_valid_i(fetch_valid),
    .execute_ready_i(execute_ready),
    .block_retire_i(block_retire),
    .older_fault_i(older_fault_i),
    .fetch_fault_i(fetch_fault_i),
    .execute_fault_i(execute_fault_i),
    .frontend_i(frontend),
    .pc_i(pc),
    .fetch_word_i(fetch_word),
    .x86_fallthrough_pc_i(x86_fallthrough_pc),
    .target_frontend_i(target_frontend),
    .target_pc_i(target_pc),
    .signature_slot_valid_i(signature_slot_valid),
    .transition_stack_full_i(transition_stack_full),
    .wait_fetch_o(wait_fetch),
    .wait_execute_o(wait_execute),
    .wait_retire_o(wait_retire),
    .retire_o(retire),
    .commit_transition_o(commit_transition),
    .commit_push_transition_o(commit_push_transition),
    .commit_frontend_o(commit_frontend),
    .commit_pc_o(commit_pc),
    .commit_signature_slot_o(commit_signature_slot),
    .fault_o(fault),
    .fault_pc_o(fault_pc),
    .older_fault_o(older_fault),
    .fetch_fault_o(fetch_fault),
    .execute_fault_o(execute_fault),
    .control_fault_o(control_fault),
    .poly_ctrl_o(poly_ctrl),
    .subop_o(subop),
    .raw_align_fault_o(raw_align_fault),
    .invalid_subop_o(invalid_subop),
    .invalid_frontend_o(invalid_frontend),
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
      fetch_valid = 1'b0;
      execute_ready = 1'b0;
      block_retire = 1'b0;
      older_fault_i = 1'b0;
      fetch_fault_i = 1'b0;
      execute_fault_i = 1'b0;
      frontend = POLY_FRONTEND_X86;
      pc = 64'd0;
      fetch_word = 32'd0;
      x86_fallthrough_pc = 64'd0;
      target_frontend = POLY_FRONTEND_X86;
      target_pc = 64'd0;
      signature_slot_valid = 1'b0;
      transition_stack_full = 1'b0;
    end
  endtask

  task automatic setup_x86_pcall;
    begin
      valid_i = 1'b1;
      fetch_valid = 1'b1;
      execute_ready = 1'b1;
      frontend = POLY_FRONTEND_X86;
      pc = 64'h1000;
      fetch_word = x86_ctrl(POLY_X86_CTRL_PCALL_SIG_IMM_BASE + 7'd2);
      x86_fallthrough_pc = 64'h1004;
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
    check(!wait_fetch && !wait_execute && !wait_retire, "idle has no waits");
    check(!retire && !fault && commit_frontend == POLY_FRONTEND_X86,
      "idle has no retire or fault");

    clear_inputs();
    setup_x86_pcall();
    #1;
    check(retire && !fault, "x86 pcall retires");
    check(commit_transition && commit_push_transition, "x86 pcall commits transition push");
    check(commit_frontend == POLY_FRONTEND_AARCH64, "x86 pcall frontend");
    check(commit_pc == 64'h4000, "x86 pcall target pc");
    check(commit_signature_slot == 7'd2, "x86 pcall signature slot");
    check(poly_ctrl && subop == POLY_X86_CTRL_PCALL_SIG_IMM_BASE + 7'd2,
      "x86 pcall decode metadata");

    clear_inputs();
    valid_i = 1'b1;
    fetch_valid = 1'b1;
    execute_ready = 1'b1;
    frontend = POLY_FRONTEND_X86;
    pc = 64'h1200;
    fetch_word = 32'h90909090;
    x86_fallthrough_pc = 64'h1204;
    target_frontend = POLY_FRONTEND_AARCH64;
    target_pc = 64'h4000;
    #1;
    check(retire && !fault && !poly_ctrl, "non-control instruction retires");
    check(!commit_transition && !commit_push_transition, "non-control has no transition");
    check(commit_frontend == POLY_FRONTEND_X86 && commit_pc == 64'h1204,
      "non-control advances to fallthrough");

    clear_inputs();
    setup_x86_pcall();
    fetch_valid = 1'b0;
    #1;
    check(wait_fetch && !wait_execute && !wait_retire, "fetch wait asserted");
    check(!retire && !fault && !commit_transition, "fetch wait blocks retirement");

    clear_inputs();
    setup_x86_pcall();
    execute_ready = 1'b0;
    #1;
    check(!wait_fetch && wait_execute && !wait_retire, "execute wait asserted");
    check(!retire && !fault && !commit_push_transition, "execute wait blocks retirement");

    clear_inputs();
    setup_x86_pcall();
    block_retire = 1'b1;
    #1;
    check(!wait_fetch && !wait_execute && wait_retire, "retire wait asserted");
    check(!retire && !fault && !commit_transition, "retire wait blocks commit");

    clear_inputs();
    setup_x86_pcall();
    older_fault_i = 1'b1;
    #1;
    check(fault && older_fault && !fetch_fault && !execute_fault, "older fault has priority");
    check(fault_pc == 64'h1000 && !retire && !commit_transition,
      "older fault blocks commit");

    clear_inputs();
    setup_x86_pcall();
    fetch_fault_i = 1'b1;
    execute_fault_i = 1'b1;
    #1;
    check(fault && !older_fault && fetch_fault && !execute_fault,
      "fetch fault precedes execute fault");
    check(!retire && !commit_push_transition, "fetch fault blocks transition push");

    clear_inputs();
    setup_x86_pcall();
    execute_fault_i = 1'b1;
    #1;
    check(fault && execute_fault && !fetch_fault && !control_fault,
      "execute fault propagates");
    check(!retire && !commit_transition, "execute fault blocks transition");

    clear_inputs();
    setup_x86_pcall();
    signature_slot_valid = 1'b0;
    #1;
    check(fault && control_fault && invalid_signature_slot,
      "invalid signature slot is control fault");
    check(!retire && !commit_push_transition, "invalid signature blocks pcall");

    clear_inputs();
    setup_x86_pcall();
    transition_stack_full = 1'b1;
    #1;
    check(fault && control_fault && transition_stack_full_o,
      "transition stack full is control fault");
    check(!retire && !commit_transition, "full stack blocks pcall");

    clear_inputs();
    valid_i = 1'b1;
    fetch_valid = 1'b1;
    execute_ready = 1'b1;
    frontend = POLY_FRONTEND_AARCH64;
    pc = 64'h4002;
    fetch_word = aarch64_ctrl(POLY_AARCH64_CTRL_SUBOP_SWITCH_MODE);
    target_frontend = POLY_FRONTEND_X86;
    target_pc = 64'h1000;
    signature_slot_valid = 1'b1;
    #1;
    check(fault && control_fault && raw_align_fault, "raw alignment control fault");
    check(!retire && !commit_transition, "raw alignment fault blocks transition");

    $display("POLY_RTL_FRONTEND_RETIRE_SIM_OK");
    $finish;
  end
endmodule
