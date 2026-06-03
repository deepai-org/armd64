`timescale 1ns/1ps

module tb_poly_frontend_fpga_top;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_MODE = 7'h2d;
  localparam logic [6:0] POLY_X86_CTRL_TRAP_RETURN = 7'h62;
  localparam logic [31:0] POLY_TRAP_BREAK = 32'd2;
  localparam logic [31:0] POLY_MODE_RAW_AARCH64 = 32'd1;
  localparam logic [31:0] POLY_MODE_RAW_RISCV = 32'd2;
  localparam logic [63:0] POLY_RETURN_COOKIE = 64'hfffffffffffff000;

  logic clk_i;
  logic rst_ni;
  logic init_i;
  logic [1:0] init_frontend_i;
  logic [63:0] init_pc_i;
  logic valid_i;
  logic [63:0] sp_i;
  logic [63:0] transition_return_pc_i;
  logic [31:0] transition_flags_i;
  logic instr_req_valid_o;
  logic [1:0] instr_req_frontend_o;
  logic [63:0] instr_req_addr_o;
  logic [4:0] instr_req_bytes_o;
  logic instr_req_conflict_o;
  logic instr_resp_valid_i;
  logic [1:0] instr_resp_frontend_i;
  logic instr_resp_fault_i;
  logic [31:0] instr_resp_word_i;
  logic [63:0] instr_resp_fallthrough_pc_i;
  logic older_fault_i;
  logic execute_fault_i;
  logic raw_branch_resolved_i;
  logic raw_branch_taken_i;
  logic [63:0] raw_branch_target_i;
  logic [63:0] raw_data_mem_addr_i;
  logic raw_data_mem_resp_valid_i;
  logic raw_data_mem_resp_fault_i;
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
  logic trap_vector_valid_i;
  logic [1:0] trap_vector_frontend_i;
  logic [63:0] trap_vector_pc_i;
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
  logic trap_return_restore_valid_i;
  logic [1:0] trap_return_restore_frontend_i;
  logic [63:0] trap_return_restore_pc_i;
  logic abi_signature_set_i;
  logic [3:0] abi_signature_set_slot_i;
  logic [7:0] abi_signature_set_kind_i;
  logic [31:0] abi_signature_set_map_i;
  logic cpuid_valid_i;
  logic [31:0] cpuid_leaf_i;
  logic [31:0] cpuid_subleaf_i;
  logic [7:0] cycle_memory_response_cycles_i;
  logic [1:0] state_frontend_o;
  logic [63:0] state_pc_o;
  logic state_update_o;
  logic state_hold_o;
  logic state_conflict_o;
  logic state_invalid_frontend_o;
  logic state_invalid_pc_o;
  logic state_error_o;
  logic redirect_valid_o;
  logic [1:0] redirect_frontend_o;
  logic [63:0] redirect_pc_o;
  logic [2:0] redirect_reason_o;
  logic wait_fetch_o;
  logic wait_execute_o;
  logic retire_o;
  logic commit_push_transition_o;
  logic [1:0] commit_frontend_o;
  logic [63:0] commit_pc_o;
  logic transition_stack_empty_o;
  logic transition_stack_full_o;
  logic transition_pop_valid_o;
  logic [1:0] transition_pop_frontend_o;
  logic [63:0] transition_pop_pc_o;
  logic [63:0] transition_pop_sp_o;
  logic [31:0] transition_pop_flags_o;
  logic transition_stack_overflow_o;
  logic transition_stack_underflow_o;
  logic transition_stack_conflict_o;
  logic [3:0] transition_stack_depth_o;
  logic transition_stack_unavailable_o;
  logic return_cookie_hit_o;
  logic return_recover_pop_o;
  logic return_resume_o;
  logic [1:0] return_resume_frontend_o;
  logic [63:0] return_resume_pc_o;
  logic [63:0] return_resume_sp_o;
  logic [31:0] return_resume_flags_o;
  logic return_recover_error_o;
  logic return_recover_invalid_frontend_o;
  logic return_recover_missing_transition_o;
  logic return_recover_blocked_o;
  logic memory_retire_allowed_o;
  logic memory_enqueue_store_o;
  logic memory_wait_store_buffer_o;
  logic memory_wait_atomic_order_o;
  logic memory_barrier_noop_o;
  logic memory_aarch64_barrier_noop_o;
  logic memory_riscv_fence_noop_o;
  logic memory_weak_reorder_allowed_o;
  logic memory_invalid_frontend_o;
  logic memory_invalid_op_o;
  logic memory_fault_o;
  logic fault_o;
  logic fetch_fault_o;
  logic execute_fault_o;
  logic control_fault_o;
  logic invalid_frontend_o;
  logic poly_ctrl_o;
  logic [6:0] subop_o;
  logic raw_data_mem_valid_o;
  logic raw_data_mem_load_o;
  logic raw_data_mem_store_o;
  logic raw_data_mem_atomic_o;
  logic [3:0] raw_data_mem_access_bytes_o;
  logic raw_data_mem_wait_o;
  logic raw_data_mem_fault_o;
  logic raw_data_mem_req_valid_o;
  logic [63:0] raw_data_mem_req_addr_o;
  logic [3:0] raw_data_mem_req_bytes_o;
  logic raw_data_mem_req_load_o;
  logic raw_data_mem_req_store_o;
  logic raw_data_mem_req_atomic_o;
  logic raw_data_mem_req_error_o;
  logic raw_data_mem_resp_wait_o;
  logic raw_data_mem_resp_resolved_o;
  logic raw_data_mem_resp_fault_o;
  logic raw_branch_valid_o;
  logic raw_branch_unresolved_o;
  logic raw_branch_static_target_valid_o;
  logic [63:0] raw_branch_static_target_o;
  logic raw_branch_wait_o;
  logic raw_branch_resolved_fault_o;
  logic interrupt_enter_x86_o;
  logic interrupt_save_interrupted_o;
  logic [1:0] interrupt_saved_frontend_o;
  logic [63:0] interrupt_saved_pc_o;
  logic interrupt_restore_raw_o;
  logic interrupt_clear_interrupted_o;
  logic [1:0] interrupt_next_frontend_o;
  logic [63:0] interrupt_next_pc_o;
  logic interrupted_valid_o;
  logic [1:0] interrupted_frontend_o;
  logic [63:0] interrupted_pc_o;
  logic interrupt_error_o;
  logic interrupt_invalid_current_frontend_o;
  logic interrupt_invalid_current_pc_o;
  logic interrupt_invalid_interrupted_frontend_o;
  logic interrupt_invalid_interrupted_pc_o;
  logic abi_signature_set_ok_o;
  logic abi_signature_set_error_o;
  logic abi_signature_apply_o;
  logic abi_signature_valid_o;
  logic [7:0] abi_signature_kind_o;
  logic [6:0] abi_signature_map_o;
  logic abi_signature_tls_base_o;
  logic cycle_budget_valid_o;
  logic [7:0] cycle_fixed_o;
  logic [7:0] cycle_variable_o;
  logic [8:0] cycle_total_o;
  logic cycle_few_cycle_fast_path_o;
  logic cycle_waits_for_memory_o;
  logic cycle_unsupported_o;
  logic cycle_invalid_op_o;
  logic cycle_blocked_o;
  logic older_fault_o;
  logic x86_fetch_wait_o;
  logic x86_request_error_o;
  logic x86_mem_fault_o;
  logic x86_noncanonical_pc_o;
  logic x86_range_fault_o;
  logic raw_fetch_wait_o;
  logic raw_request_error_o;
  logic raw_mem_fault_o;
  logic raw_noncanonical_pc_o;
  logic raw_align_fault_o;
  logic raw_range_fault_o;
  logic invalid_subop_o;
  logic noncanonical_target_o;
  logic target_align_fault_o;
  logic invalid_signature_slot_o;
  logic trap_mem_write_valid_o;
  logic [63:0] trap_mem_write_addr_o;
  logic [7:0] trap_mem_write_bytes_o;
  logic [63:0] trap_mem_write_qword0_o;
  logic [63:0] trap_mem_write_qword1_o;
  logic [63:0] trap_mem_write_qword2_o;
  logic [63:0] trap_mem_write_qword3_o;
  logic [63:0] trap_mem_write_qword4_o;
  logic [63:0] trap_mem_write_qword5_o;
  logic [63:0] trap_mem_write_qword6_o;
  logic [63:0] trap_mem_write_qword7_o;
  logic [63:0] trap_mem_write_qword8_o;
  logic [63:0] trap_mem_write_qword9_o;
  logic [63:0] trap_mem_write_qword10_o;
  logic [63:0] trap_mem_write_qword11_o;
  logic [63:0] trap_mem_write_qword12_o;
  logic [63:0] trap_mem_write_qword13_o;
  logic [63:0] trap_mem_write_qword14_o;
  logic [63:0] trap_mem_write_qword15_o;
  logic trap_packet_delivered_o;
  logic trap_fault_o;
  logic trap_encode_error_o;
  logic trap_packet_mem_fault_o;
  logic trap_monitor_disabled_o;
  logic trap_noncanonical_packet_o;
  logic trap_packet_align_fault_o;
  logic trap_packet_range_fault_o;
  logic trap_invalid_reason_o;
  logic trap_invalid_source_mode_o;
  logic trap_vector_apply_o;
  logic [1:0] trap_vector_frontend_o;
  logic [63:0] trap_vector_pc_o;
  logic trap_return_decode_o;
  logic trap_return_restore_o;
  logic [1:0] trap_return_restore_frontend_o;
  logic [63:0] trap_return_restore_pc_o;

  poly_frontend_fpga_top dut (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .init_i(init_i),
    .init_frontend_i(init_frontend_i),
    .init_pc_i(init_pc_i),
    .valid_i(valid_i),
    .sp_i(sp_i),
    .transition_return_pc_i(transition_return_pc_i),
    .transition_flags_i(transition_flags_i),
    .instr_req_valid_o(instr_req_valid_o),
    .instr_req_frontend_o(instr_req_frontend_o),
    .instr_req_addr_o(instr_req_addr_o),
    .instr_req_bytes_o(instr_req_bytes_o),
    .instr_req_conflict_o(instr_req_conflict_o),
    .instr_resp_valid_i(instr_resp_valid_i),
    .instr_resp_frontend_i(instr_resp_frontend_i),
    .instr_resp_fault_i(instr_resp_fault_i),
    .instr_resp_word_i(instr_resp_word_i),
    .instr_resp_fallthrough_pc_i(instr_resp_fallthrough_pc_i),
    .older_fault_i(older_fault_i),
    .execute_fault_i(execute_fault_i),
    .raw_branch_resolved_i(raw_branch_resolved_i),
    .raw_branch_taken_i(raw_branch_taken_i),
    .raw_branch_target_i(raw_branch_target_i),
    .raw_data_mem_addr_i(raw_data_mem_addr_i),
    .raw_data_mem_resp_valid_i(raw_data_mem_resp_valid_i),
    .raw_data_mem_resp_fault_i(raw_data_mem_resp_fault_i),
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
    .trap_vector_valid_i(trap_vector_valid_i),
    .trap_vector_frontend_i(trap_vector_frontend_i),
    .trap_vector_pc_i(trap_vector_pc_i),
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
    .trap_return_restore_valid_i(trap_return_restore_valid_i),
    .trap_return_restore_frontend_i(trap_return_restore_frontend_i),
    .trap_return_restore_pc_i(trap_return_restore_pc_i),
    .abi_signature_set_i(abi_signature_set_i),
    .abi_signature_set_slot_i(abi_signature_set_slot_i),
    .abi_signature_set_kind_i(abi_signature_set_kind_i),
    .abi_signature_set_map_i(abi_signature_set_map_i),
    .cpuid_valid_i(cpuid_valid_i),
    .cpuid_leaf_i(cpuid_leaf_i),
    .cpuid_subleaf_i(cpuid_subleaf_i),
    .cycle_memory_response_cycles_i(cycle_memory_response_cycles_i),
    .state_frontend_o(state_frontend_o),
    .state_pc_o(state_pc_o),
    .state_update_o(state_update_o),
    .state_hold_o(state_hold_o),
    .state_conflict_o(state_conflict_o),
    .state_invalid_frontend_o(state_invalid_frontend_o),
    .state_invalid_pc_o(state_invalid_pc_o),
    .state_error_o(state_error_o),
    .redirect_valid_o(redirect_valid_o),
    .redirect_frontend_o(redirect_frontend_o),
    .redirect_pc_o(redirect_pc_o),
    .redirect_reason_o(redirect_reason_o),
    .wait_fetch_o(wait_fetch_o),
    .wait_execute_o(wait_execute_o),
    .wait_retire_o(),
    .retire_o(retire_o),
    .commit_transition_o(),
    .commit_push_transition_o(commit_push_transition_o),
    .commit_frontend_o(commit_frontend_o),
    .commit_pc_o(commit_pc_o),
    .commit_signature_slot_o(),
    .transition_stack_empty_o(transition_stack_empty_o),
    .transition_stack_full_o(transition_stack_full_o),
    .transition_pop_valid_o(transition_pop_valid_o),
    .transition_pop_frontend_o(transition_pop_frontend_o),
    .transition_pop_pc_o(transition_pop_pc_o),
    .transition_pop_sp_o(transition_pop_sp_o),
    .transition_pop_flags_o(transition_pop_flags_o),
    .transition_stack_overflow_o(transition_stack_overflow_o),
    .transition_stack_underflow_o(transition_stack_underflow_o),
    .transition_stack_conflict_o(transition_stack_conflict_o),
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
    .return_recover_invalid_frontend_o(return_recover_invalid_frontend_o),
    .return_recover_missing_transition_o(return_recover_missing_transition_o),
    .return_recover_blocked_o(return_recover_blocked_o),
    .memory_retire_allowed_o(memory_retire_allowed_o),
    .memory_enqueue_store_o(memory_enqueue_store_o),
    .memory_wait_store_buffer_o(memory_wait_store_buffer_o),
    .memory_wait_atomic_order_o(memory_wait_atomic_order_o),
    .memory_barrier_noop_o(memory_barrier_noop_o),
    .memory_aarch64_barrier_noop_o(memory_aarch64_barrier_noop_o),
    .memory_riscv_fence_noop_o(memory_riscv_fence_noop_o),
    .memory_weak_reorder_allowed_o(memory_weak_reorder_allowed_o),
    .memory_invalid_frontend_o(memory_invalid_frontend_o),
    .memory_invalid_op_o(memory_invalid_op_o),
    .memory_fault_o(memory_fault_o),
    .raw_data_mem_valid_o(raw_data_mem_valid_o),
    .raw_data_mem_load_o(raw_data_mem_load_o),
    .raw_data_mem_store_o(raw_data_mem_store_o),
    .raw_data_mem_atomic_o(raw_data_mem_atomic_o),
    .raw_data_mem_access_bytes_o(raw_data_mem_access_bytes_o),
    .raw_data_mem_wait_o(raw_data_mem_wait_o),
    .raw_data_mem_fault_o(raw_data_mem_fault_o),
    .raw_data_mem_req_valid_o(raw_data_mem_req_valid_o),
    .raw_data_mem_req_addr_o(raw_data_mem_req_addr_o),
    .raw_data_mem_req_bytes_o(raw_data_mem_req_bytes_o),
    .raw_data_mem_req_load_o(raw_data_mem_req_load_o),
    .raw_data_mem_req_store_o(raw_data_mem_req_store_o),
    .raw_data_mem_req_atomic_o(raw_data_mem_req_atomic_o),
    .raw_data_mem_req_error_o(raw_data_mem_req_error_o),
    .raw_data_mem_req_invalid_frontend_o(),
    .raw_data_mem_req_invalid_op_o(),
    .raw_data_mem_req_invalid_width_o(),
    .raw_data_mem_req_noncanonical_o(),
    .raw_data_mem_req_align_fault_o(),
    .raw_data_mem_req_range_fault_o(),
    .raw_data_mem_resp_wait_o(raw_data_mem_resp_wait_o),
    .raw_data_mem_resp_resolved_o(raw_data_mem_resp_resolved_o),
    .raw_data_mem_resp_fault_o(raw_data_mem_resp_fault_o),
    .raw_data_mem_resp_request_fault_o(),
    .raw_data_mem_resp_memory_fault_o(),
    .raw_branch_valid_o(raw_branch_valid_o),
    .raw_branch_unresolved_o(raw_branch_unresolved_o),
    .raw_branch_static_target_valid_o(raw_branch_static_target_valid_o),
    .raw_branch_static_target_o(raw_branch_static_target_o),
    .raw_branch_wait_o(raw_branch_wait_o),
    .raw_branch_resolved_fault_o(raw_branch_resolved_fault_o),
    .interrupt_enter_x86_o(interrupt_enter_x86_o),
    .interrupt_save_interrupted_o(interrupt_save_interrupted_o),
    .interrupt_saved_frontend_o(interrupt_saved_frontend_o),
    .interrupt_saved_pc_o(interrupt_saved_pc_o),
    .interrupt_restore_raw_o(interrupt_restore_raw_o),
    .interrupt_clear_interrupted_o(interrupt_clear_interrupted_o),
    .interrupt_next_frontend_o(interrupt_next_frontend_o),
    .interrupt_next_pc_o(interrupt_next_pc_o),
    .interrupted_valid_o(interrupted_valid_o),
    .interrupted_frontend_o(interrupted_frontend_o),
    .interrupted_pc_o(interrupted_pc_o),
    .interrupt_error_o(interrupt_error_o),
    .interrupt_invalid_current_frontend_o(interrupt_invalid_current_frontend_o),
    .interrupt_invalid_current_pc_o(interrupt_invalid_current_pc_o),
    .interrupt_invalid_interrupted_frontend_o(interrupt_invalid_interrupted_frontend_o),
    .interrupt_invalid_interrupted_pc_o(interrupt_invalid_interrupted_pc_o),
    .trap_mem_write_valid_o(trap_mem_write_valid_o),
    .trap_mem_write_addr_o(trap_mem_write_addr_o),
    .trap_mem_write_bytes_o(trap_mem_write_bytes_o),
    .trap_mem_write_qword0_o(trap_mem_write_qword0_o),
    .trap_mem_write_qword1_o(trap_mem_write_qword1_o),
    .trap_mem_write_qword2_o(trap_mem_write_qword2_o),
    .trap_mem_write_qword3_o(trap_mem_write_qword3_o),
    .trap_mem_write_qword4_o(trap_mem_write_qword4_o),
    .trap_mem_write_qword5_o(trap_mem_write_qword5_o),
    .trap_mem_write_qword6_o(trap_mem_write_qword6_o),
    .trap_mem_write_qword7_o(trap_mem_write_qword7_o),
    .trap_mem_write_qword8_o(trap_mem_write_qword8_o),
    .trap_mem_write_qword9_o(trap_mem_write_qword9_o),
    .trap_mem_write_qword10_o(trap_mem_write_qword10_o),
    .trap_mem_write_qword11_o(trap_mem_write_qword11_o),
    .trap_mem_write_qword12_o(trap_mem_write_qword12_o),
    .trap_mem_write_qword13_o(trap_mem_write_qword13_o),
    .trap_mem_write_qword14_o(trap_mem_write_qword14_o),
    .trap_mem_write_qword15_o(trap_mem_write_qword15_o),
    .trap_wait_response_o(),
    .trap_packet_delivered_o(trap_packet_delivered_o),
    .trap_fault_o(trap_fault_o),
    .trap_encode_error_o(trap_encode_error_o),
    .trap_packet_mem_fault_o(trap_packet_mem_fault_o),
    .trap_monitor_disabled_o(trap_monitor_disabled_o),
    .trap_noncanonical_packet_o(trap_noncanonical_packet_o),
    .trap_packet_align_fault_o(trap_packet_align_fault_o),
    .trap_packet_range_fault_o(trap_packet_range_fault_o),
    .trap_invalid_reason_o(trap_invalid_reason_o),
    .trap_invalid_source_mode_o(trap_invalid_source_mode_o),
    .trap_vector_apply_o(trap_vector_apply_o),
    .trap_vector_frontend_o(trap_vector_frontend_o),
    .trap_vector_pc_o(trap_vector_pc_o),
    .trap_return_decode_o(trap_return_decode_o),
    .trap_return_restore_o(trap_return_restore_o),
    .trap_return_restore_frontend_o(trap_return_restore_frontend_o),
    .trap_return_restore_pc_o(trap_return_restore_pc_o),
    .abi_signature_set_ok_o(abi_signature_set_ok_o),
    .abi_signature_set_error_o(abi_signature_set_error_o),
    .abi_signature_apply_o(abi_signature_apply_o),
    .abi_signature_valid_o(abi_signature_valid_o),
    .abi_signature_kind_o(abi_signature_kind_o),
    .abi_signature_map_o(abi_signature_map_o),
    .abi_signature_tls_base_o(abi_signature_tls_base_o),
    .cpuid_hit_o(),
    .cpuid_eax_o(),
    .cpuid_ebx_o(),
    .cpuid_ecx_o(),
    .cpuid_edx_o(),
    .cycle_budget_valid_o(cycle_budget_valid_o),
    .cycle_fixed_o(cycle_fixed_o),
    .cycle_variable_o(cycle_variable_o),
    .cycle_total_o(cycle_total_o),
    .cycle_few_cycle_fast_path_o(cycle_few_cycle_fast_path_o),
    .cycle_waits_for_memory_o(cycle_waits_for_memory_o),
    .cycle_unsupported_o(cycle_unsupported_o),
    .cycle_invalid_op_o(cycle_invalid_op_o),
    .cycle_blocked_o(cycle_blocked_o),
    .fault_o(fault_o),
    .fault_pc_o(),
    .older_fault_o(older_fault_o),
    .fetch_fault_o(fetch_fault_o),
    .execute_fault_o(execute_fault_o),
    .control_fault_o(control_fault_o),
    .invalid_frontend_o(invalid_frontend_o),
    .x86_fetch_wait_o(x86_fetch_wait_o),
    .x86_request_error_o(x86_request_error_o),
    .x86_mem_fault_o(x86_mem_fault_o),
    .x86_noncanonical_pc_o(x86_noncanonical_pc_o),
    .x86_range_fault_o(x86_range_fault_o),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .raw_fetch_wait_o(raw_fetch_wait_o),
    .raw_request_error_o(raw_request_error_o),
    .raw_mem_fault_o(raw_mem_fault_o),
    .raw_noncanonical_pc_o(raw_noncanonical_pc_o),
    .raw_align_fault_o(raw_align_fault_o),
    .raw_range_fault_o(raw_range_fault_o),
    .invalid_subop_o(invalid_subop_o),
    .noncanonical_target_o(noncanonical_target_o),
    .target_align_fault_o(target_align_fault_o),
    .invalid_signature_slot_o(invalid_signature_slot_o)
  );

  function automatic logic [31:0] x86_ctrl(input logic [6:0] ctrl_subop);
    begin
      x86_ctrl = {1'b0, ctrl_subop, 8'hfc, 8'h3a, 8'h0f};
    end
  endfunction

  task automatic clear_inputs;
    begin
      init_i = 1'b0;
      init_frontend_i = POLY_FRONTEND_X86;
      init_pc_i = 64'd0;
      valid_i = 1'b0;
      sp_i = 64'd0;
      transition_return_pc_i = 64'd0;
      transition_flags_i = 32'd0;
      instr_resp_valid_i = 1'b0;
      instr_resp_frontend_i = POLY_FRONTEND_X86;
      instr_resp_fault_i = 1'b0;
      instr_resp_word_i = 32'd0;
      instr_resp_fallthrough_pc_i = 64'd0;
      older_fault_i = 1'b0;
      execute_fault_i = 1'b0;
      raw_branch_resolved_i = 1'b0;
      raw_branch_taken_i = 1'b0;
      raw_branch_target_i = 64'd0;
      raw_data_mem_addr_i = 64'd0;
      raw_data_mem_resp_valid_i = 1'b0;
      raw_data_mem_resp_fault_i = 1'b0;
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
      trap_vector_valid_i = 1'b0;
      trap_vector_frontend_i = POLY_FRONTEND_X86;
      trap_vector_pc_i = 64'd0;
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
      trap_return_restore_valid_i = 1'b0;
      trap_return_restore_frontend_i = POLY_FRONTEND_X86;
      trap_return_restore_pc_i = 64'd0;
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

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_AARCH64;
    init_pc_i = 64'h4000;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_AARCH64 &&
      state_pc_o == 64'h4000, "init to raw frontend");

    valid_i = 1'b1;
    #1;
    check(instr_req_valid_o, "raw issues unified fetch request");
    check(instr_req_frontend_o == POLY_FRONTEND_AARCH64,
      "raw request carries frontend tag");
    check(instr_req_addr_o == 64'h4000 && instr_req_bytes_o == 5'd4,
      "raw request carries address and width");
    check(!instr_req_conflict_o, "raw request has no split-port conflict");
    check(raw_fetch_wait_o && !raw_request_error_o && !raw_mem_fault_o,
      "fpga top exposes raw fetch wait diagnostics");
    tick();

    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_X86;
    instr_resp_word_i = 32'h52800000;
    #1;
    check(wait_fetch_o && !retire_o, "wrong frontend response is ignored");
    check(raw_fetch_wait_o && !fetch_fault_o,
      "fpga top keeps raw fetch wait on wrong frontend response");

    instr_resp_frontend_i = POLY_FRONTEND_AARCH64;
    #1;
    check(retire_o && !fault_o, "raw response retires through fpga top");
    check(commit_pc_o == 64'h4004, "raw response commits fallthrough");
    tick();
    clear_inputs();
    #1;
    check(state_pc_o == 64'h4004, "raw response updates state");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_AARCH64;
    instr_resp_word_i = 32'hf9400000;
    raw_data_mem_addr_i = 64'h9000;
    #1;
    check(wait_execute_o && !retire_o && raw_data_mem_valid_o,
      "fpga top raw load waits on data-memory resolution");
    check(raw_data_mem_load_o && raw_data_mem_access_bytes_o == 4'd8 &&
      raw_data_mem_wait_o,
      "fpga top exposes raw load data-memory metadata");
    check(raw_data_mem_req_valid_o && !raw_data_mem_req_error_o &&
      raw_data_mem_req_load_o && raw_data_mem_req_addr_o == 64'h9000 &&
      raw_data_mem_req_bytes_o == 4'd8,
      "fpga top issues raw data-memory request metadata");
    check(raw_data_mem_resp_wait_o && !raw_data_mem_resp_resolved_o,
      "fpga top raw load waits for data-memory response");
    raw_data_mem_resp_valid_i = 1'b1;
    #1;
    check(retire_o && !fault_o && raw_data_mem_valid_o &&
      !raw_data_mem_wait_o, "fpga top raw load retires when data resolves");
    check(raw_data_mem_resp_resolved_o && !raw_data_mem_resp_fault_o,
      "fpga top raw load reports resolved response");
    tick();
    clear_inputs();
    #1;
    check(state_pc_o == 64'h4008, "resolved raw load updates state");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_AARCH64;
    instr_resp_word_i = 32'hf9000000;
    raw_data_mem_addr_i = 64'h9010;
    raw_data_mem_resp_valid_i = 1'b1;
    store_buffer_full_i = 1'b1;
    #1;
    check(wait_execute_o && !retire_o && !fault_o &&
      memory_wait_store_buffer_o && !memory_retire_allowed_o,
      "fpga top raw store waits for full store buffer");
    check(raw_data_mem_valid_o && !raw_data_mem_load_o &&
      raw_data_mem_store_o && !raw_data_mem_atomic_o &&
      !raw_data_mem_wait_o && !raw_data_mem_fault_o,
      "fpga top exposes raw store data-memory metadata");
    check(raw_data_mem_req_valid_o && !raw_data_mem_req_error_o &&
      !raw_data_mem_req_load_o && raw_data_mem_req_store_o &&
      !raw_data_mem_req_atomic_o && raw_data_mem_req_addr_o == 64'h9010 &&
      raw_data_mem_req_bytes_o == 4'd8,
      "fpga top issues raw store request metadata");
    check(!memory_enqueue_store_o && !memory_wait_atomic_order_o &&
      !memory_barrier_noop_o && !memory_aarch64_barrier_noop_o &&
      !memory_riscv_fence_noop_o && !memory_weak_reorder_allowed_o &&
      !memory_invalid_frontend_o && !memory_invalid_op_o && !memory_fault_o,
      "fpga top exposes clean store-buffer wait diagnostics");
    store_buffer_full_i = 1'b0;
    #1;
    check(retire_o && !fault_o && memory_retire_allowed_o &&
      memory_enqueue_store_o && !memory_wait_store_buffer_o,
      "fpga top raw store retires and enqueues when buffer has space");
    tick();
    clear_inputs();
    #1;
    check(state_pc_o == 64'h400c, "resolved raw store updates state");

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_AARCH64;
    init_pc_i = 64'h4008;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_AARCH64 &&
      state_pc_o == 64'h4008, "fpga top reinit after raw store");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_AARCH64;
    instr_resp_word_i = 32'h94000002;
    #1;
    check(retire_o && !fault_o && raw_branch_valid_o &&
      raw_branch_static_target_valid_o && raw_branch_static_target_o == 64'h4010,
      "fpga top raw direct branch exposes static target sideband");
    tick();
    clear_inputs();
    #1;
    check(state_pc_o == 64'h4010, "fpga top raw direct branch updates state");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_AARCH64;
    instr_resp_word_i = 32'h54000040;
    #1;
    check(wait_execute_o && !retire_o && raw_branch_valid_o &&
      raw_branch_unresolved_o && raw_branch_wait_o,
      "fpga top raw unresolved branch exposes wait sideband");
    raw_branch_resolved_i = 1'b1;
    raw_branch_taken_i = 1'b1;
    raw_branch_target_i = 64'h5000;
    #1;
    check(retire_o && !fault_o && raw_branch_unresolved_o &&
      !raw_branch_wait_o && !raw_branch_resolved_fault_o,
      "fpga top raw resolved branch clears wait sideband");
    tick();
    clear_inputs();
    #1;
    check(state_pc_o == 64'h5000, "fpga top raw resolved branch updates state");

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_RISCV;
    init_pc_i = 64'h8000;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_RISCV &&
      state_pc_o == 64'h8000, "fpga top reinit to interrupt raw state");

    valid_i = 1'b1;
    interrupt_feature_enabled_i = 1'b1;
    cpl3_i = 1'b1;
    interrupt_i = 1'b1;
    #1;
    check(interrupt_enter_x86_o && interrupt_save_interrupted_o &&
      interrupt_saved_frontend_o == POLY_FRONTEND_RISCV &&
      interrupt_saved_pc_o == 64'h8000,
      "fpga top exposes raw interrupt save sidebands");
    check(interrupt_next_frontend_o == POLY_FRONTEND_X86 &&
      interrupt_next_pc_o == 64'h8000 &&
      !interrupt_error_o && !interrupt_invalid_current_frontend_o &&
      !interrupt_invalid_current_pc_o,
      "fpga top exposes raw interrupt x86-entry target");
    check(state_update_o && redirect_frontend_o == POLY_FRONTEND_X86 &&
      redirect_pc_o == 64'h8000 && redirect_reason_o == 3'd3,
      "fpga top raw interrupt redirects state");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h8000,
      "fpga top raw interrupt updates state");
    check(interrupted_valid_o &&
      interrupted_frontend_o == POLY_FRONTEND_RISCV &&
      interrupted_pc_o == 64'h8000,
      "fpga top retains interrupted raw frame");

    valid_i = 1'b1;
    interrupt_feature_enabled_i = 1'b1;
    cpl3_i = 1'b1;
    user_return_i = 1'b1;
    user_return_pc_i = 64'h8000;
    #1;
    check(interrupt_restore_raw_o && interrupt_clear_interrupted_o &&
      interrupt_next_frontend_o == POLY_FRONTEND_RISCV &&
      interrupt_next_pc_o == 64'h8000,
      "fpga top exposes matching user-return restore sidebands");
    check(!interrupt_error_o && !interrupt_invalid_interrupted_frontend_o &&
      !interrupt_invalid_interrupted_pc_o,
      "fpga top exposes clean user-return diagnostics");
    check(state_update_o && redirect_frontend_o == POLY_FRONTEND_RISCV &&
      redirect_pc_o == 64'h8000 && redirect_reason_o == 3'd3,
      "fpga top user return redirects state");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_RISCV &&
      state_pc_o == 64'h8000,
      "fpga top user return restores raw state");
    check(!interrupted_valid_o, "fpga top clears interrupted frame");

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_X86;
    init_pc_i = 64'h1000;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1000, "reinit to x86 frontend");

    valid_i = 1'b1;
    #1;
    check(wait_fetch_o && x86_fetch_wait_o && !x86_request_error_o &&
      !x86_mem_fault_o && !fetch_fault_o,
      "fpga top exposes x86 fetch wait diagnostics");

    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_X86;
    instr_resp_fault_i = 1'b1;
    #1;
    check(fault_o && fetch_fault_o && x86_mem_fault_o &&
      !x86_request_error_o && !x86_noncanonical_pc_o &&
      !x86_range_fault_o,
      "fpga top exposes x86 fetch memory-fault diagnostics");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1000, "x86 fetch fault preserves state");

    abi_signature_set_i = 1'b1;
    abi_signature_set_slot_i = 4'd0;
    abi_signature_set_kind_i = 8'd7;
    abi_signature_set_map_i = 32'h0000001a;
    tick();
    abi_signature_set_i = 1'b0;
    #1;
    check(abi_signature_set_ok_o && !abi_signature_set_error_o,
      "fpga top programs abi signature slot");
    clear_inputs();
    #1;

    valid_i = 1'b1;
    sp_i = 64'h8000;
    transition_return_pc_i = 64'h1004;
    transition_flags_i = 32'h1234;
    target_frontend_i = POLY_FRONTEND_AARCH64;
    target_pc_i = 64'h5000;
    signature_slot_valid_i = 1'b1;
    #1;
    check(instr_req_valid_o && instr_req_frontend_o == POLY_FRONTEND_X86,
      "x86 issues unified fetch request");
    check(instr_req_addr_o == 64'h1000 && instr_req_bytes_o == 5'd16,
      "x86 request carries address and fetch width");

    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_RISCV;
    instr_resp_word_i = x86_ctrl(POLY_X86_CTRL_PCALL_SIG_MODE);
    instr_resp_fallthrough_pc_i = 64'h1004;
    #1;
    check(wait_fetch_o && !retire_o, "wrong raw response is ignored by x86");

    instr_resp_frontend_i = POLY_FRONTEND_X86;
    #1;
    check(poly_ctrl_o && subop_o == POLY_X86_CTRL_PCALL_SIG_MODE,
      "x86 response decodes pcall");
    check(retire_o && commit_push_transition_o, "x86 pcall retires");
    check(abi_signature_apply_o && abi_signature_valid_o &&
      abi_signature_kind_o == 8'd7 && abi_signature_map_o == 7'h1a &&
      !abi_signature_tls_base_o,
      "fpga top exposes pcall abi signature metadata");
    check(cycle_budget_valid_o && cycle_fixed_o == 8'd4 &&
      cycle_variable_o == 8'd0 && cycle_total_o == 9'd4 &&
      cycle_few_cycle_fast_path_o && !cycle_waits_for_memory_o &&
      !cycle_unsupported_o && !cycle_invalid_op_o && !cycle_blocked_o,
      "fpga top exposes pcall few-cycle budget");
    check(commit_frontend_o == POLY_FRONTEND_AARCH64 &&
      commit_pc_o == 64'h5000, "x86 pcall commits target frontend");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_AARCH64 &&
      state_pc_o == 64'h5000, "x86 pcall updates state");
    check(!transition_stack_empty_o && !transition_stack_full_o &&
      transition_stack_depth_o == 4'd1, "x86 pcall pushes transition");

    return_recover_valid_i = 1'b1;
    return_target_pc_i = POLY_RETURN_COOKIE;
    #1;
    check(return_cookie_hit_o && return_recover_pop_o && return_resume_o &&
      transition_stack_unavailable_o,
      "fpga top exposes return-cookie recovery control sidebands");
    check(return_resume_frontend_o == POLY_FRONTEND_X86 &&
      return_resume_pc_o == 64'h1004 &&
      return_resume_sp_o == 64'h8000 &&
      return_resume_flags_o == 32'h1234,
      "fpga top exposes return-cookie resume frame");
    check(!return_recover_error_o && !return_recover_invalid_frontend_o &&
      !return_recover_missing_transition_o && !return_recover_blocked_o,
      "fpga top exposes clean return-cookie diagnostics");
    check(cycle_budget_valid_o && cycle_fixed_o == 8'd3 &&
      cycle_variable_o == 8'd0 && cycle_total_o == 9'd3 &&
      cycle_few_cycle_fast_path_o && !cycle_waits_for_memory_o &&
      !cycle_unsupported_o && !cycle_invalid_op_o && !cycle_blocked_o,
      "fpga top exposes return-cookie few-cycle budget");
    check(state_update_o && redirect_frontend_o == POLY_FRONTEND_X86 &&
      redirect_pc_o == 64'h1004, "fpga top return-cookie redirects state");
    tick();
    clear_inputs();
    #1;
    check(transition_pop_valid_o &&
      transition_pop_frontend_o == POLY_FRONTEND_X86 &&
      transition_pop_pc_o == 64'h1004 &&
      transition_pop_sp_o == 64'h8000 &&
      transition_pop_flags_o == 32'h1234,
      "fpga top exposes transition pop frame");
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1004, "fpga top return-cookie updates state");
    check(transition_stack_empty_o && transition_stack_depth_o == 4'd0,
      "fpga top return-cookie drains transition");

    return_recover_valid_i = 1'b1;
    return_target_pc_i = POLY_RETURN_COOKIE;
    #1;
    check(return_cookie_hit_o && return_recover_error_o &&
      return_recover_missing_transition_o && !return_recover_pop_o &&
      !return_resume_o && !return_recover_blocked_o,
      "fpga top exposes empty-stack return-cookie diagnostics");
    clear_inputs();
    #1;

    valid_i = 1'b1;
    trap_valid_i = 1'b1;
    trap_monitor_enabled_i = 1'b1;
    trap_monitor_packet_addr_i = 64'h457000;
    trap_vector_valid_i = 1'b1;
    trap_vector_frontend_i = POLY_FRONTEND_RISCV;
    trap_vector_pc_i = 64'h8000;
    trap_reason_i = POLY_TRAP_BREAK;
    trap_source_mode_i = POLY_MODE_RAW_RISCV;
    trap_number_i = 64'h11;
    trap_selector_i = 64'h22;
    trap_pc_i = 64'h7000;
    trap_resume_pc_i = 64'h7004;
    trap_arg0_i = 64'h8000_0000_0000_0001;
    trap_arg1_i = 64'h8000_0000_0000_0002;
    trap_arg2_i = 64'h8000_0000_0000_0003;
    trap_arg3_i = 64'h8000_0000_0000_0004;
    trap_arg4_i = 64'h8000_0000_0000_0005;
    trap_arg5_i = 64'h8000_0000_0000_0006;
    trap_arg6_i = 64'h8000_0000_0000_0007;
    trap_arg7_i = 64'h8000_0000_0000_0008;
    trap_mem_write_resp_valid_i = 1'b1;
    cycle_memory_response_cycles_i = 8'd6;
    #1;
    check(trap_mem_write_valid_o && trap_mem_write_addr_o == 64'h457000 &&
      trap_mem_write_bytes_o == 8'd128,
      "fpga top exposes trap-packet memory write envelope");
    check(trap_mem_write_qword0_o == {POLY_MODE_RAW_RISCV, POLY_TRAP_BREAK} &&
      trap_mem_write_qword1_o == 64'h11 &&
      trap_mem_write_qword2_o == 64'h22 &&
      trap_mem_write_qword3_o == 64'h7000 &&
      trap_mem_write_qword4_o == 64'h7004 &&
      trap_mem_write_qword5_o == 64'h7f &&
      trap_mem_write_qword6_o == 64'h0 &&
      trap_mem_write_qword7_o == 64'h0,
      "fpga top exposes trap-packet metadata qwords");
    check(trap_mem_write_qword8_o == 64'h8000_0000_0000_0001 &&
      trap_mem_write_qword9_o == 64'h8000_0000_0000_0002 &&
      trap_mem_write_qword10_o == 64'h8000_0000_0000_0003 &&
      trap_mem_write_qword11_o == 64'h8000_0000_0000_0004 &&
      trap_mem_write_qword12_o == 64'h8000_0000_0000_0005 &&
      trap_mem_write_qword13_o == 64'h8000_0000_0000_0006 &&
      trap_mem_write_qword14_o == 64'h8000_0000_0000_0007 &&
      trap_mem_write_qword15_o == 64'h8000_0000_0000_0008,
      "fpga top exposes trap-packet argument qwords");
    check(trap_vector_apply_o && trap_vector_frontend_o == POLY_FRONTEND_RISCV &&
      trap_vector_pc_o == 64'h8000, "fpga top exposes trap-vector apply target");
    check(cycle_budget_valid_o && cycle_fixed_o == 8'd2 &&
      cycle_variable_o == 8'd6 && cycle_total_o == 9'd8 &&
      !cycle_few_cycle_fast_path_o && cycle_waits_for_memory_o &&
      !cycle_unsupported_o && !cycle_invalid_op_o && !cycle_blocked_o,
      "fpga top exposes trap-packet memory-response cycle budget");
    check(state_update_o && redirect_frontend_o == POLY_FRONTEND_RISCV &&
      redirect_pc_o == 64'h8000, "fpga top trap vector redirects state");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_RISCV &&
      state_pc_o == 64'h8000, "fpga top trap vector updates state");

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_AARCH64;
    init_pc_i = 64'h6000;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_AARCH64 &&
      state_pc_o == 64'h6000, "fpga top reinit to invalid trap case");

    valid_i = 1'b1;
    trap_valid_i = 1'b1;
    trap_monitor_enabled_i = 1'b0;
    trap_monitor_packet_addr_i = 64'h457000;
    trap_vector_valid_i = 1'b1;
    trap_vector_frontend_i = POLY_FRONTEND_RISCV;
    trap_vector_pc_i = 64'h8000;
    trap_reason_i = POLY_TRAP_BREAK;
    trap_source_mode_i = POLY_MODE_RAW_AARCH64;
    #1;
    check(trap_fault_o && trap_encode_error_o && trap_monitor_disabled_o,
      "fpga top exposes monitor-disabled trap encode fault");
    check(!trap_packet_mem_fault_o && !trap_noncanonical_packet_o &&
      !trap_packet_align_fault_o && !trap_packet_range_fault_o &&
      !trap_invalid_reason_o && !trap_invalid_source_mode_o,
      "fpga top exposes precise trap encode fault cause");
    check(!trap_mem_write_valid_o && !trap_packet_delivered_o &&
      !trap_vector_apply_o && !state_update_o,
      "fpga top suppresses invalid trap delivery and redirect");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_AARCH64 &&
      state_pc_o == 64'h6000, "invalid trap preserves state");

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_X86;
    init_pc_i = 64'h1800;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1800, "fpga top reinit to trap-return monitor");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_X86;
    instr_resp_word_i = x86_ctrl(POLY_X86_CTRL_TRAP_RETURN);
    instr_resp_fallthrough_pc_i = 64'h1804;
    trap_return_restore_valid_i = 1'b0;
    #1;
    check(wait_execute_o && trap_return_decode_o && !trap_return_restore_o,
      "fpga top trap return waits for restore target");
    check(!retire_o && !state_update_o && state_hold_o,
      "fpga top trap return wait preserves state");

    trap_return_restore_valid_i = 1'b1;
    trap_return_restore_frontend_i = POLY_FRONTEND_RISCV;
    trap_return_restore_pc_i = 64'h8400;
    #1;
    check(retire_o && trap_return_restore_o && !commit_push_transition_o,
      "fpga top trap return retires as restore redirect");
    check(trap_return_restore_frontend_o == POLY_FRONTEND_RISCV &&
      trap_return_restore_pc_o == 64'h8400,
      "fpga top trap return exposes restore target");
    check(state_update_o && redirect_frontend_o == POLY_FRONTEND_RISCV &&
      redirect_pc_o == 64'h8400 && redirect_reason_o == 3'd6,
      "fpga top trap return redirects state");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_RISCV &&
      state_pc_o == 64'h8400, "fpga top trap return updates state");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_RISCV;
    instr_resp_fault_i = 1'b1;
    #1;
    check(fault_o && fetch_fault_o && raw_mem_fault_o &&
      !raw_request_error_o && !raw_noncanonical_pc_o &&
      !raw_align_fault_o && !raw_range_fault_o,
      "fpga top exposes raw fetch memory-fault diagnostics");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_RISCV &&
      state_pc_o == 64'h8400, "raw fetch fault preserves state");

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_X86;
    init_pc_i = 64'h1800;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1800, "fpga top reinit to invalid trap-return case");

    valid_i = 1'b1;
    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_X86;
    instr_resp_word_i = x86_ctrl(POLY_X86_CTRL_TRAP_RETURN);
    instr_resp_fallthrough_pc_i = 64'h1804;
    trap_return_restore_valid_i = 1'b1;
    trap_return_restore_frontend_i = POLY_FRONTEND_RISCV;
    trap_return_restore_pc_i = 64'h8401;
    #1;
    check(retire_o && trap_return_restore_o,
      "fpga top invalid trap return reaches state boundary");
    check(state_error_o && state_invalid_pc_o && !state_update_o &&
      !redirect_valid_o && state_hold_o,
      "fpga top exposes invalid trap-return restore target");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1800, "invalid trap return preserves state");

    $display("POLY_RTL_FRONTEND_FPGA_TOP_SIM_OK");
    $finish;
  end
endmodule
