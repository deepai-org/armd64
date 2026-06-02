`timescale 1ns/1ps

module tb_poly_frontend_predecoded_retire;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PSWITCH_MODE = 7'h04;
  localparam logic [6:0] POLY_X86_CTRL_TRAP_RETURN = 7'h62;
  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_IMM_BASE = 7'h30;
  localparam logic [6:0] POLY_RISCV_CTRL_SUBOP_X86_ESCAPE = 7'd0;

  logic valid_i;
  logic fetch_valid;
  logic decode_valid;
  logic execute_ready;
  logic block_retire;
  logic older_fault_i;
  logic fetch_fault_i;
  logic execute_fault_i;
  logic [1:0] frontend;
  logic [63:0] pc;
  logic [63:0] fallthrough_pc;
  logic poly_ctrl_i;
  logic [6:0] subop_i;
  logic call_sig_imm_i;
  logic [6:0] signature_slot_i;
  logic [1:0] target_frontend;
  logic [63:0] target_pc;
  logic signature_slot_valid;
  logic transition_stack_full;
  logic trap_return_restore_valid;

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
  logic poly_ctrl;
  logic [6:0] subop;
  logic raw_align_fault;
  logic invalid_subop;
  logic invalid_frontend;
  logic noncanonical_target;
  logic target_align_fault;
  logic invalid_signature_slot;
  logic transition_stack_full_o;

  poly_frontend_predecoded_retire dut (
    .valid_i(valid_i),
    .fetch_valid_i(fetch_valid),
    .decode_valid_i(decode_valid),
    .execute_ready_i(execute_ready),
    .block_retire_i(block_retire),
    .older_fault_i(older_fault_i),
    .fetch_fault_i(fetch_fault_i),
    .execute_fault_i(execute_fault_i),
    .frontend_i(frontend),
    .pc_i(pc),
    .fallthrough_pc_i(fallthrough_pc),
    .poly_ctrl_i(poly_ctrl_i),
    .subop_i(subop_i),
    .call_sig_imm_i(call_sig_imm_i),
    .signature_slot_i(signature_slot_i),
    .target_frontend_i(target_frontend),
    .target_pc_i(target_pc),
    .signature_slot_valid_i(signature_slot_valid),
    .transition_stack_full_i(transition_stack_full),
    .trap_return_restore_valid_i(trap_return_restore_valid),
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

  task automatic clear_inputs;
    begin
      valid_i = 1'b0;
      fetch_valid = 1'b0;
      decode_valid = 1'b0;
      execute_ready = 1'b0;
      block_retire = 1'b0;
      older_fault_i = 1'b0;
      fetch_fault_i = 1'b0;
      execute_fault_i = 1'b0;
      frontend = POLY_FRONTEND_X86;
      pc = 64'd0;
      fallthrough_pc = 64'd0;
      poly_ctrl_i = 1'b0;
      subop_i = 7'd0;
      call_sig_imm_i = 1'b0;
      signature_slot_i = 7'd0;
      target_frontend = POLY_FRONTEND_X86;
      target_pc = 64'd0;
      signature_slot_valid = 1'b0;
      transition_stack_full = 1'b0;
      trap_return_restore_valid = 1'b0;
    end
  endtask

  task automatic setup_ready;
    begin
      valid_i = 1'b1;
      fetch_valid = 1'b1;
      decode_valid = 1'b1;
      execute_ready = 1'b1;
      frontend = POLY_FRONTEND_X86;
      pc = 64'h1000;
      fallthrough_pc = 64'h1004;
      target_frontend = POLY_FRONTEND_AARCH64;
      target_pc = 64'h4000;
      signature_slot_valid = 1'b1;
    end
  endtask

  task automatic setup_x86_pcall_sig;
    begin
      setup_ready();
      poly_ctrl_i = 1'b1;
      subop_i = POLY_X86_CTRL_PCALL_SIG_IMM_BASE + 7'd3;
      call_sig_imm_i = 1'b1;
      signature_slot_i = 7'd3;
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
    check(!retire && !fault && !wait_fetch, "idle has no event");

    clear_inputs();
    setup_ready();
    poly_ctrl_i = 1'b0;
    #1;
    check(retire && !fault, "non-control predecoded instruction retires");
    check(!commit_transition && !commit_push_transition, "non-control has no transition");
    check(commit_frontend == POLY_FRONTEND_X86 && commit_pc == 64'h1004,
      "non-control commits fallthrough");
    check(!poly_ctrl && subop == 7'd0 && !raw_align_fault,
      "non-control metadata suppressed");

    clear_inputs();
    setup_ready();
    poly_ctrl_i = 1'b1;
    subop_i = POLY_X86_CTRL_PSWITCH_MODE;
    target_frontend = POLY_FRONTEND_AARCH64;
    target_pc = 64'h4000;
    #1;
    check(retire && !fault && commit_transition, "predecoded switch retires");
    check(!commit_push_transition, "switch does not push transition stack");
    check(commit_frontend == POLY_FRONTEND_AARCH64 && commit_pc == 64'h4000,
      "switch target commits");
    check(poly_ctrl && subop == POLY_X86_CTRL_PSWITCH_MODE, "switch metadata");

    clear_inputs();
    setup_ready();
    poly_ctrl_i = 1'b1;
    subop_i = POLY_X86_CTRL_TRAP_RETURN;
    trap_return_restore_valid = 1'b0;
    #1;
    check(wait_execute && trap_return_decode && !trap_return_retire,
      "trap return waits for restore target");
    check(!retire && !fault && !commit_transition,
      "trap return restore wait blocks retire");

    trap_return_restore_valid = 1'b1;
    #1;
    check(retire && trap_return_decode && trap_return_retire && !fault,
      "trap return retires with restore target");
    check(!commit_transition && !commit_push_transition,
      "trap return does not use transition commit");

    clear_inputs();
    setup_x86_pcall_sig();
    #1;
    check(retire && !fault && commit_transition && commit_push_transition,
      "predecoded pcall retires and pushes");
    check(commit_signature_slot == 7'd3, "predecoded pcall signature slot");

    clear_inputs();
    setup_x86_pcall_sig();
    signature_slot_valid = 1'b0;
    #1;
    check(fault && control_fault && invalid_signature_slot,
      "predecoded invalid signature faults");
    check(!retire && !commit_push_transition, "invalid signature blocks pcall");

    clear_inputs();
    setup_x86_pcall_sig();
    transition_stack_full = 1'b1;
    #1;
    check(fault && control_fault && transition_stack_full_o,
      "predecoded stack full faults");
    check(!retire && !commit_transition, "stack full blocks transition");

    clear_inputs();
    setup_ready();
    fetch_valid = 1'b0;
    decode_valid = 1'b0;
    #1;
    check(wait_fetch && !wait_execute && !wait_retire, "predecoded waits for fetch");
    check(!retire && !fault, "fetch wait has no fault");

    clear_inputs();
    setup_ready();
    decode_valid = 1'b0;
    #1;
    check(!wait_execute && !retire && !fault, "decode wait blocks execute wait");

    clear_inputs();
    setup_ready();
    execute_ready = 1'b0;
    #1;
    check(wait_execute && !wait_fetch && !wait_retire, "predecoded waits for execute");
    check(!retire && !fault, "execute wait has no fault");

    clear_inputs();
    setup_ready();
    block_retire = 1'b1;
    #1;
    check(wait_retire && !wait_fetch && !wait_execute, "predecoded waits for retire");
    check(!retire && !fault, "retire wait has no fault");

    clear_inputs();
    setup_ready();
    older_fault_i = 1'b1;
    fetch_fault_i = 1'b1;
    execute_fault_i = 1'b1;
    #1;
    check(fault && older_fault && !fetch_fault && !execute_fault,
      "older fault dominates predecoded retire");
    check(fault_pc == 64'h1000 && !retire, "older fault pc");

    clear_inputs();
    setup_ready();
    fetch_fault_i = 1'b1;
    execute_fault_i = 1'b1;
    #1;
    check(fault && fetch_fault && !execute_fault, "fetch fault precedes execute");
    check(!retire && !control_fault, "fetch fault blocks control");

    clear_inputs();
    setup_ready();
    execute_fault_i = 1'b1;
    #1;
    check(fault && execute_fault && !fetch_fault, "execute fault propagates");
    check(!retire && !control_fault, "execute fault blocks control");

    clear_inputs();
    setup_ready();
    frontend = POLY_FRONTEND_RISCV;
    fallthrough_pc = 64'h8004;
    poly_ctrl_i = 1'b1;
    subop_i = POLY_RISCV_CTRL_SUBOP_X86_ESCAPE;
    target_frontend = POLY_FRONTEND_RISCV;
    target_pc = 64'h2000;
    #1;
    check(retire && commit_transition && !commit_push_transition,
      "riscv x86 escape retires");
    check(commit_frontend == POLY_FRONTEND_X86 && commit_pc == 64'h2000,
      "riscv escape forces x86 frontend");

    $display("POLY_RTL_FRONTEND_PREDECODED_RETIRE_SIM_OK");
    $finish;
  end
endmodule
