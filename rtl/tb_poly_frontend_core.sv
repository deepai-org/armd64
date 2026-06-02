`timescale 1ns/1ps

module tb_poly_frontend_core;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_MODE = 7'h2d;
  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_IMM_BASE = 7'h30;
  localparam logic [31:0] POLY_CPUID_BASE = 32'h40000000;
  localparam logic [31:0] POLY_CPUID_MAX = 32'h40000009;

  logic clk_i;
  logic rst_ni;

  logic valid_i;
  logic [1:0] frontend_i;
  logic [63:0] pc_i;
  logic [63:0] sp_i;
  logic [63:0] transition_return_pc_i;
  logic [31:0] transition_flags_i;
  logic x86_fetch_valid_i;
  logic x86_fetch_fault_i;
  logic [31:0] x86_fetch_word_i;
  logic [63:0] x86_fallthrough_pc_i;
  logic raw_mem_resp_valid_i;
  logic raw_mem_resp_fault_i;
  logic [31:0] raw_mem_resp_word_i;
  logic older_fault_i;
  logic execute_fault_i;
  logic raw_branch_resolved_i;
  logic raw_branch_taken_i;
  logic [63:0] raw_branch_target_i;
  logic raw_memory_resolved_i;
  logic raw_memory_fault_i;
  logic memory_order_valid_i;
  logic memory_load_i;
  logic memory_store_i;
  logic memory_atomic_i;
  logic memory_barrier_i;
  logic older_store_pending_i;
  logic store_buffer_full_i;
  logic [1:0] target_frontend_i;
  logic [63:0] target_pc_i;
  logic signature_slot_valid_i;
  logic transition_pop_i;
  logic return_recover_valid_i;
  logic [63:0] return_target_pc_i;
  logic interrupt_feature_enabled_i;
  logic cpl3_i;
  logic interrupt_i;
  logic user_return_i;
  logic [63:0] user_return_pc_i;
  logic trap_valid_i;
  logic trap_monitor_enabled_i;
  logic [63:0] trap_monitor_packet_addr_i;
  logic [31:0] trap_reason_i;
  logic [31:0] trap_source_mode_i;
  logic [63:0] trap_number_i;
  logic [63:0] trap_selector_i;
  logic [63:0] trap_pc_i;
  logic [63:0] trap_resume_pc_i;
  logic [63:0] trap_arg0_i;
  logic [63:0] trap_arg1_i;
  logic [63:0] trap_arg2_i;
  logic [63:0] trap_arg3_i;
  logic [63:0] trap_arg4_i;
  logic [63:0] trap_arg5_i;
  logic [63:0] trap_arg6_i;
  logic [63:0] trap_arg7_i;
  logic trap_mem_write_resp_valid_i;
  logic trap_mem_write_fault_i;
  logic abi_signature_set_i;
  logic [3:0] abi_signature_set_slot_i;
  logic [7:0] abi_signature_set_kind_i;
  logic [31:0] abi_signature_set_map_i;
  logic cpuid_valid_i;
  logic [31:0] cpuid_leaf_i;
  logic [31:0] cpuid_subleaf_i;
  logic [7:0] cycle_memory_response_cycles_i;

  logic x86_fetch_req_valid_o;
  logic [63:0] x86_fetch_req_addr_o;
  logic [4:0] x86_fetch_req_bytes_o;
  logic raw_mem_req_valid_o;
  logic wait_fetch_o;
  logic wait_execute_o;
  logic wait_retire_o;
  logic retire_o;
  logic commit_transition_o;
  logic commit_push_transition_o;
  logic [1:0] commit_frontend_o;
  logic [63:0] commit_pc_o;
  logic [6:0] commit_signature_slot_o;
  logic transition_pop_valid_o;
  logic [1:0] transition_pop_frontend_o;
  logic [63:0] transition_pop_pc_o;
  logic [63:0] transition_pop_sp_o;
  logic [31:0] transition_pop_flags_o;
  logic transition_stack_empty_o;
  logic transition_stack_full_o;
  logic transition_stack_unavailable_o;
  logic [3:0] transition_stack_depth_o;
  logic abi_signature_apply_o;
  logic abi_signature_valid_o;
  logic [7:0] abi_signature_kind_o;
  logic [6:0] abi_signature_map_o;
  logic abi_signature_tls_base_o;
  logic cpuid_hit_o;
  logic [31:0] cpuid_eax_o;
  logic [31:0] cpuid_ebx_o;
  logic [31:0] cpuid_ecx_o;
  logic [31:0] cpuid_edx_o;
  logic cycle_budget_valid_o;
  logic [7:0] cycle_fixed_o;
  logic [8:0] cycle_total_o;
  logic cycle_few_cycle_fast_path_o;
  logic fault_o;
  logic [63:0] fault_pc_o;
  logic poly_ctrl_o;
  logic [6:0] subop_o;
  logic raw_branch_target_valid_o;
  logic [63:0] raw_branch_target_o;
  logic return_cookie_hit_o;
  logic return_recover_pop_o;
  logic return_resume_o;
  logic [1:0] return_resume_frontend_o;
  logic [63:0] return_resume_pc_o;
  logic [63:0] return_resume_sp_o;
  logic [31:0] return_resume_flags_o;
  logic return_recover_error_o;
  logic return_recover_blocked_o;
  logic memory_retire_allowed_o;
  logic memory_enqueue_store_o;
  logic memory_wait_store_buffer_o;
  logic memory_wait_atomic_order_o;
  logic raw_data_mem_valid_o;
  logic raw_data_mem_load_o;
  logic raw_data_mem_store_o;
  logic raw_data_mem_atomic_o;
  logic [3:0] raw_data_mem_access_bytes_o;
  logic raw_data_mem_wait_o;
  logic raw_data_mem_fault_o;

  poly_frontend_core dut (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .sp_i(sp_i),
    .transition_return_pc_i(transition_return_pc_i),
    .transition_flags_i(transition_flags_i),
    .x86_fetch_valid_i(x86_fetch_valid_i),
    .x86_fetch_fault_i(x86_fetch_fault_i),
    .x86_fetch_word_i(x86_fetch_word_i),
    .x86_fallthrough_pc_i(x86_fallthrough_pc_i),
    .raw_mem_resp_valid_i(raw_mem_resp_valid_i),
    .raw_mem_resp_fault_i(raw_mem_resp_fault_i),
    .raw_mem_resp_word_i(raw_mem_resp_word_i),
    .older_fault_i(older_fault_i),
    .execute_fault_i(execute_fault_i),
    .raw_branch_resolved_i(raw_branch_resolved_i),
    .raw_branch_taken_i(raw_branch_taken_i),
    .raw_branch_target_i(raw_branch_target_i),
    .raw_memory_resolved_i(raw_memory_resolved_i),
    .raw_memory_fault_i(raw_memory_fault_i),
    .memory_order_valid_i(memory_order_valid_i),
    .memory_load_i(memory_load_i),
    .memory_store_i(memory_store_i),
    .memory_atomic_i(memory_atomic_i),
    .memory_barrier_i(memory_barrier_i),
    .older_store_pending_i(older_store_pending_i),
    .store_buffer_full_i(store_buffer_full_i),
    .target_frontend_i(target_frontend_i),
    .target_pc_i(target_pc_i),
    .signature_slot_valid_i(signature_slot_valid_i),
    .transition_pop_i(transition_pop_i),
    .return_recover_valid_i(return_recover_valid_i),
    .return_target_pc_i(return_target_pc_i),
    .interrupt_feature_enabled_i(interrupt_feature_enabled_i),
    .cpl3_i(cpl3_i),
    .interrupt_i(interrupt_i),
    .user_return_i(user_return_i),
    .user_return_pc_i(user_return_pc_i),
    .trap_valid_i(trap_valid_i),
    .trap_monitor_enabled_i(trap_monitor_enabled_i),
    .trap_monitor_packet_addr_i(trap_monitor_packet_addr_i),
    .trap_reason_i(trap_reason_i),
    .trap_source_mode_i(trap_source_mode_i),
    .trap_number_i(trap_number_i),
    .trap_selector_i(trap_selector_i),
    .trap_pc_i(trap_pc_i),
    .trap_resume_pc_i(trap_resume_pc_i),
    .trap_arg0_i(trap_arg0_i),
    .trap_arg1_i(trap_arg1_i),
    .trap_arg2_i(trap_arg2_i),
    .trap_arg3_i(trap_arg3_i),
    .trap_arg4_i(trap_arg4_i),
    .trap_arg5_i(trap_arg5_i),
    .trap_arg6_i(trap_arg6_i),
    .trap_arg7_i(trap_arg7_i),
    .trap_mem_write_resp_valid_i(trap_mem_write_resp_valid_i),
    .trap_mem_write_fault_i(trap_mem_write_fault_i),
    .abi_signature_set_i(abi_signature_set_i),
    .abi_signature_set_slot_i(abi_signature_set_slot_i),
    .abi_signature_set_kind_i(abi_signature_set_kind_i),
    .abi_signature_set_map_i(abi_signature_set_map_i),
    .cpuid_valid_i(cpuid_valid_i),
    .cpuid_leaf_i(cpuid_leaf_i),
    .cpuid_subleaf_i(cpuid_subleaf_i),
    .cycle_memory_response_cycles_i(cycle_memory_response_cycles_i),
    .x86_fetch_req_valid_o(x86_fetch_req_valid_o),
    .x86_fetch_req_addr_o(x86_fetch_req_addr_o),
    .x86_fetch_req_bytes_o(x86_fetch_req_bytes_o),
    .raw_mem_req_valid_o(raw_mem_req_valid_o),
    .raw_mem_req_addr_o(),
    .raw_mem_req_bytes_o(),
    .wait_fetch_o(wait_fetch_o),
    .wait_execute_o(wait_execute_o),
    .wait_retire_o(wait_retire_o),
    .retire_o(retire_o),
    .commit_transition_o(commit_transition_o),
    .commit_push_transition_o(commit_push_transition_o),
    .commit_frontend_o(commit_frontend_o),
    .commit_pc_o(commit_pc_o),
    .commit_signature_slot_o(commit_signature_slot_o),
    .transition_pop_valid_o(transition_pop_valid_o),
    .transition_pop_frontend_o(transition_pop_frontend_o),
    .transition_pop_pc_o(transition_pop_pc_o),
    .transition_pop_sp_o(transition_pop_sp_o),
    .transition_pop_flags_o(transition_pop_flags_o),
    .transition_stack_empty_o(transition_stack_empty_o),
    .transition_stack_full_o(transition_stack_full_o),
    .transition_stack_overflow_o(),
    .transition_stack_underflow_o(),
    .transition_stack_conflict_o(),
    .transition_stack_depth_o(transition_stack_depth_o),
    .transition_stack_unavailable_o(transition_stack_unavailable_o),
    .return_cookie_hit_o(return_cookie_hit_o),
    .return_recover_pop_o(return_recover_pop_o),
    .return_resume_o(return_resume_o),
    .return_resume_frontend_o(return_resume_frontend_o),
    .return_resume_pc_o(return_resume_pc_o),
    .return_resume_sp_o(return_resume_sp_o),
    .return_resume_flags_o(return_resume_flags_o),
    .return_recover_error_o(return_recover_error_o),
    .return_recover_invalid_frontend_o(),
    .return_recover_missing_transition_o(),
    .return_recover_blocked_o(return_recover_blocked_o),
    .memory_retire_allowed_o(memory_retire_allowed_o),
    .memory_enqueue_store_o(memory_enqueue_store_o),
    .memory_wait_store_buffer_o(memory_wait_store_buffer_o),
    .memory_wait_atomic_order_o(memory_wait_atomic_order_o),
    .memory_barrier_noop_o(),
    .memory_aarch64_barrier_noop_o(),
    .memory_riscv_fence_noop_o(),
    .memory_weak_reorder_allowed_o(),
    .memory_invalid_frontend_o(),
    .memory_invalid_op_o(),
    .memory_fault_o(),
    .raw_data_mem_valid_o(raw_data_mem_valid_o),
    .raw_data_mem_load_o(raw_data_mem_load_o),
    .raw_data_mem_store_o(raw_data_mem_store_o),
    .raw_data_mem_atomic_o(raw_data_mem_atomic_o),
    .raw_data_mem_access_bytes_o(raw_data_mem_access_bytes_o),
    .raw_data_mem_wait_o(raw_data_mem_wait_o),
    .raw_data_mem_fault_o(raw_data_mem_fault_o),
    .interrupt_enter_x86_o(),
    .interrupt_save_interrupted_o(),
    .interrupt_saved_frontend_o(),
    .interrupt_saved_pc_o(),
    .interrupt_restore_raw_o(),
    .interrupt_clear_interrupted_o(),
    .interrupt_next_frontend_o(),
    .interrupt_next_pc_o(),
    .interrupted_valid_o(),
    .interrupted_frontend_o(),
    .interrupted_pc_o(),
    .interrupt_error_o(),
    .interrupt_invalid_current_frontend_o(),
    .interrupt_invalid_current_pc_o(),
    .interrupt_invalid_interrupted_frontend_o(),
    .interrupt_invalid_interrupted_pc_o(),
    .trap_mem_write_valid_o(),
    .trap_mem_write_addr_o(),
    .trap_mem_write_bytes_o(),
    .trap_mem_write_qword0_o(),
    .trap_mem_write_qword1_o(),
    .trap_mem_write_qword2_o(),
    .trap_mem_write_qword3_o(),
    .trap_mem_write_qword4_o(),
    .trap_mem_write_qword5_o(),
    .trap_mem_write_qword6_o(),
    .trap_mem_write_qword7_o(),
    .trap_mem_write_qword8_o(),
    .trap_mem_write_qword9_o(),
    .trap_mem_write_qword10_o(),
    .trap_mem_write_qword11_o(),
    .trap_mem_write_qword12_o(),
    .trap_mem_write_qword13_o(),
    .trap_mem_write_qword14_o(),
    .trap_mem_write_qword15_o(),
    .trap_wait_response_o(),
    .trap_packet_delivered_o(),
    .trap_fault_o(),
    .trap_encode_error_o(),
    .trap_packet_mem_fault_o(),
    .trap_monitor_disabled_o(),
    .trap_noncanonical_packet_o(),
    .trap_packet_align_fault_o(),
    .trap_packet_range_fault_o(),
    .trap_invalid_reason_o(),
    .trap_invalid_source_mode_o(),
    .abi_signature_set_ok_o(),
    .abi_signature_set_error_o(),
    .abi_signature_apply_o(abi_signature_apply_o),
    .abi_signature_valid_o(abi_signature_valid_o),
    .abi_signature_kind_o(abi_signature_kind_o),
    .abi_signature_map_o(abi_signature_map_o),
    .abi_signature_tls_base_o(abi_signature_tls_base_o),
    .cpuid_hit_o(cpuid_hit_o),
    .cpuid_eax_o(cpuid_eax_o),
    .cpuid_ebx_o(cpuid_ebx_o),
    .cpuid_ecx_o(cpuid_ecx_o),
    .cpuid_edx_o(cpuid_edx_o),
    .cycle_budget_valid_o(cycle_budget_valid_o),
    .cycle_fixed_o(cycle_fixed_o),
    .cycle_variable_o(),
    .cycle_total_o(cycle_total_o),
    .cycle_few_cycle_fast_path_o(cycle_few_cycle_fast_path_o),
    .cycle_waits_for_memory_o(),
    .cycle_unsupported_o(),
    .cycle_invalid_op_o(),
    .cycle_blocked_o(),
    .fault_o(fault_o),
    .fault_pc_o(fault_pc_o),
    .older_fault_o(),
    .fetch_fault_o(),
    .execute_fault_o(),
    .control_fault_o(),
    .invalid_frontend_o(),
    .x86_fetch_wait_o(),
    .x86_request_error_o(),
    .x86_mem_fault_o(),
    .x86_noncanonical_pc_o(),
    .x86_range_fault_o(),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .raw_branch_target_valid_o(raw_branch_target_valid_o),
    .raw_branch_target_o(raw_branch_target_o),
    .raw_fetch_wait_o(),
    .raw_request_error_o(),
    .raw_mem_fault_o(),
    .raw_noncanonical_pc_o(),
    .raw_align_fault_o(),
    .raw_range_fault_o(),
    .invalid_subop_o(),
    .noncanonical_target_o(),
    .target_align_fault_o(),
    .invalid_signature_slot_o()
  );

  function automatic logic [31:0] x86_ctrl(input logic [6:0] ctrl_subop);
    begin
      x86_ctrl = {1'b0, ctrl_subop, 8'hfc, 8'h3a, 8'h0f};
    end
  endfunction

  task automatic clear_inputs;
    begin
      valid_i = 1'b0;
      frontend_i = POLY_FRONTEND_X86;
      pc_i = 64'd0;
      sp_i = 64'd0;
      transition_return_pc_i = 64'd0;
      transition_flags_i = 32'd0;
      x86_fetch_valid_i = 1'b0;
      x86_fetch_fault_i = 1'b0;
      x86_fetch_word_i = 32'd0;
      x86_fallthrough_pc_i = 64'd0;
      raw_mem_resp_valid_i = 1'b0;
      raw_mem_resp_fault_i = 1'b0;
      raw_mem_resp_word_i = 32'd0;
      older_fault_i = 1'b0;
      execute_fault_i = 1'b0;
      raw_branch_resolved_i = 1'b0;
      raw_branch_taken_i = 1'b0;
      raw_branch_target_i = 64'd0;
      raw_memory_resolved_i = 1'b0;
      raw_memory_fault_i = 1'b0;
      memory_order_valid_i = 1'b0;
      memory_load_i = 1'b0;
      memory_store_i = 1'b0;
      memory_atomic_i = 1'b0;
      memory_barrier_i = 1'b0;
      older_store_pending_i = 1'b0;
      store_buffer_full_i = 1'b0;
      target_frontend_i = POLY_FRONTEND_X86;
      target_pc_i = 64'd0;
      signature_slot_valid_i = 1'b0;
      transition_pop_i = 1'b0;
      return_recover_valid_i = 1'b0;
      return_target_pc_i = 64'd0;
      interrupt_feature_enabled_i = 1'b0;
      cpl3_i = 1'b1;
      interrupt_i = 1'b0;
      user_return_i = 1'b0;
      user_return_pc_i = 64'd0;
      trap_valid_i = 1'b0;
      trap_monitor_enabled_i = 1'b0;
      trap_monitor_packet_addr_i = 64'd0;
      trap_reason_i = 32'd0;
      trap_source_mode_i = 32'd0;
      trap_number_i = 64'd0;
      trap_selector_i = 64'd0;
      trap_pc_i = 64'd0;
      trap_resume_pc_i = 64'd0;
      trap_arg0_i = 64'd0;
      trap_arg1_i = 64'd0;
      trap_arg2_i = 64'd0;
      trap_arg3_i = 64'd0;
      trap_arg4_i = 64'd0;
      trap_arg5_i = 64'd0;
      trap_arg6_i = 64'd0;
      trap_arg7_i = 64'd0;
      trap_mem_write_resp_valid_i = 1'b0;
      trap_mem_write_fault_i = 1'b0;
      abi_signature_set_i = 1'b0;
      abi_signature_set_slot_i = 4'd0;
      abi_signature_set_kind_i = 8'd0;
      abi_signature_set_map_i = 32'd0;
      cpuid_valid_i = 1'b0;
      cpuid_leaf_i = 32'd0;
      cpuid_subleaf_i = 32'd0;
      cycle_memory_response_cycles_i = 8'd0;
    end
  endtask

  task automatic tick;
    begin
      #1 clk_i = 1'b1;
      #1 clk_i = 1'b0;
      #1;
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
    clk_i = 1'b0;
    rst_ni = 1'b0;
    clear_inputs();
    tick();
    rst_ni = 1'b1;
    tick();
    check(transition_stack_empty_o && transition_stack_depth_o == 4'd0,
      "core reset leaves transition stack empty");

    cpuid_valid_i = 1'b1;
    cpuid_leaf_i = POLY_CPUID_BASE;
    #1;
    check(cpuid_hit_o && cpuid_eax_o == POLY_CPUID_MAX, "core cpuid base hit");
    check(cpuid_ebx_o == 32'h796c6f50 && cpuid_ecx_o == 32'h21555043 &&
      cpuid_edx_o == 32'h746f6c67, "core cpuid vendor");
    cpuid_valid_i = 1'b0;

    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_X86;
    pc_i = 64'h1000;
    sp_i = 64'h8000;
    transition_return_pc_i = 64'h1004;
    transition_flags_i = 32'h55;
    x86_fetch_valid_i = 1'b1;
    x86_fetch_word_i = x86_ctrl(POLY_X86_CTRL_PCALL_SIG_MODE);
    x86_fallthrough_pc_i = 64'h1004;
    target_frontend_i = POLY_FRONTEND_AARCH64;
    target_pc_i = 64'h4000;
    signature_slot_valid_i = 1'b1;
    #1;
    check(x86_fetch_req_valid_o && !raw_mem_req_valid_o, "core pcall uses x86 fetch");
    check(x86_fetch_req_addr_o == 64'h1000 && x86_fetch_req_bytes_o == 5'd16,
      "core pcall x86 request geometry");
    check(retire_o && !fault_o, "core pcall retires");
    check(commit_transition_o && commit_push_transition_o, "core pcall commits push");
    check(commit_frontend_o == POLY_FRONTEND_AARCH64 && commit_pc_o == 64'h4000,
      "core pcall target commits");
    check(commit_signature_slot_o == 7'd0, "core pcall default signature slot");
    check(abi_signature_apply_o && abi_signature_valid_o, "core applies abi signature");
    check(abi_signature_kind_o == 8'd0 && abi_signature_map_o == 7'd0,
      "core selects default exchange signature");
    check(!abi_signature_tls_base_o, "core default abi signature has no tls base");
    check(cycle_budget_valid_o && cycle_fixed_o == 8'd4 &&
      cycle_total_o == 9'd4 && cycle_few_cycle_fast_path_o,
      "core pcall is few-cycle budgeted");
    check(poly_ctrl_o && subop_o == POLY_X86_CTRL_PCALL_SIG_MODE,
      "core pcall decode metadata");

    tick();
    valid_i = 1'b0;
    x86_fetch_valid_i = 1'b0;
    #1;
    check(transition_stack_depth_o == 4'd1 && !transition_stack_empty_o,
      "core pcall pushes transition frame");

    transition_pop_i = 1'b1;
    #1;
    check(transition_stack_unavailable_o, "manual pop makes stack unavailable");
    tick();
    transition_pop_i = 1'b0;
    #1;
    check(transition_pop_valid_o, "manual transition pop valid");
    check(transition_pop_frontend_o == POLY_FRONTEND_X86, "popped frontend");
    check(transition_pop_pc_o == 64'h1004, "popped return pc");
    check(transition_pop_sp_o == 64'h8000, "popped sp");
    check(transition_pop_flags_o == 32'h55, "popped flags");
    check(transition_stack_depth_o == 4'd0 && transition_stack_empty_o,
      "manual pop drains transition stack");

    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_X86;
    pc_i = 64'h1100;
    sp_i = 64'h7ff0;
    transition_return_pc_i = 64'h1104;
    transition_flags_i = 32'd0;
    x86_fetch_valid_i = 1'b1;
    x86_fetch_word_i = x86_ctrl(POLY_X86_CTRL_PCALL_SIG_IMM_BASE + 7'd8);
    x86_fallthrough_pc_i = 64'h1104;
    target_frontend_i = POLY_FRONTEND_RISCV;
    target_pc_i = 64'h8000;
    signature_slot_valid_i = 1'b1;
    #1;
    check(retire_o && commit_push_transition_o, "core immediate-slot pcall retires");
    check(commit_signature_slot_o == 7'd8, "core immediate-slot pcall slot");
    check(abi_signature_valid_o && abi_signature_kind_o == 8'd9 &&
      abi_signature_map_o == 7'd8, "core immediate-slot abi metadata");
    tick();
    valid_i = 1'b0;
    x86_fetch_valid_i = 1'b0;
    #1;
    check(transition_stack_depth_o == 4'd1, "immediate-slot pcall pushes frame");

    return_recover_valid_i = 1'b1;
    return_target_pc_i = 64'hfffffffffffff000;
    frontend_i = POLY_FRONTEND_RISCV;
    #1;
    check(return_cookie_hit_o && return_recover_pop_o && return_resume_o,
      "core return cookie recovers through stack");
    check(return_resume_frontend_o == POLY_FRONTEND_X86 &&
      return_resume_pc_o == 64'h1104 && return_resume_sp_o == 64'h7ff0,
      "core return cookie resume frame");
    check(!return_recover_error_o && !return_recover_blocked_o,
      "core return cookie has no error");
    check(cycle_budget_valid_o && cycle_fixed_o == 8'd3 &&
      cycle_total_o == 9'd3 && cycle_few_cycle_fast_path_o,
      "core return cookie is few-cycle budgeted");
    tick();
    return_recover_valid_i = 1'b0;
    #1;
    check(transition_stack_depth_o == 4'd0 && transition_stack_empty_o,
      "return cookie pop drains transition stack");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_AARCH64;
    pc_i = 64'h4000;
    raw_mem_resp_valid_i = 1'b1;
    raw_mem_resp_word_i = 32'hf9000000;
    raw_memory_resolved_i = 1'b1;
    store_buffer_full_i = 1'b1;
    #1;
    check(raw_mem_req_valid_o, "raw store issues raw fetch");
    check(wait_execute_o && !retire_o && !fault_o,
      "decoded raw store waits for full store buffer");
    check(memory_wait_store_buffer_o && !memory_retire_allowed_o,
      "decoded raw store drives tso store-buffer wait");

    store_buffer_full_i = 1'b0;
    #1;
    check(memory_retire_allowed_o && retire_o && !fault_o,
      "decoded raw store retires when store buffer has space");
    check(memory_enqueue_store_o, "decoded raw store enqueues store on retire");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_AARCH64;
    pc_i = 64'h5000;
    raw_mem_resp_valid_i = 1'b1;
    raw_mem_resp_word_i = 32'hf9400000;
    #1;
    check(wait_execute_o && !retire_o && !fault_o,
      "decoded raw load waits for data-memory resolution");
    check(raw_data_mem_valid_o && raw_data_mem_load_o &&
      !raw_data_mem_store_o && !raw_data_mem_atomic_o,
      "decoded raw load exposes data-memory load sideband");
    check(raw_data_mem_access_bytes_o == 4'd8 && raw_data_mem_wait_o,
      "decoded raw load exposes 8-byte pending data-memory access");

    raw_memory_resolved_i = 1'b1;
    #1;
    check(memory_retire_allowed_o && retire_o && !fault_o,
      "decoded raw load retires after data-memory resolution");
    check(raw_data_mem_valid_o && !raw_data_mem_wait_o,
      "resolved raw load keeps metadata without wait");

    raw_memory_resolved_i = 1'b0;
    raw_memory_fault_i = 1'b1;
    #1;
    check(fault_o && !retire_o,
      "decoded raw load data-memory fault blocks retirement");
    check(raw_data_mem_fault_o, "decoded raw load reports data-memory fault");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_AARCH64;
    pc_i = 64'h6000;
    raw_mem_resp_valid_i = 1'b1;
    raw_mem_resp_word_i = 32'h94000002;
    #1;
    check(retire_o && !fault_o, "decoded raw direct branch retires");
    check(raw_branch_target_valid_o && raw_branch_target_o == 64'h6008,
      "decoded raw direct branch exposes retired target");

    clear_inputs();
    valid_i = 1'b1;
    frontend_i = POLY_FRONTEND_AARCH64;
    pc_i = 64'h6010;
    raw_mem_resp_valid_i = 1'b1;
    raw_mem_resp_word_i = 32'h54000040;
    #1;
    check(wait_execute_o && !retire_o && !raw_branch_target_valid_o,
      "decoded raw unresolved branch waits for execute resolution");

    raw_branch_resolved_i = 1'b1;
    raw_branch_taken_i = 1'b0;
    #1;
    check(retire_o && !fault_o && !raw_branch_target_valid_o,
      "decoded raw resolved not-taken branch retires fallthrough");

    raw_branch_taken_i = 1'b1;
    raw_branch_target_i = 64'h7000;
    #1;
    check(retire_o && !fault_o && raw_branch_target_valid_o &&
      raw_branch_target_o == 64'h7000,
      "decoded raw resolved taken branch exposes execute target");

    $display("POLY_RTL_FRONTEND_CORE_SIM_OK");
    $finish;
  end
endmodule
