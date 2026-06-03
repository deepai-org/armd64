// Poly frontend core bring-up wrapper.
//
// This composes fetch-to-retire control with the hardware transition stack.
// The caller supplies transition_return_pc_i because the precise return PC is
// frontend/decoder dependent and should not be inferred by the stack block.
module poly_frontend_core (
    input  logic        clk_i,
    input  logic        rst_ni,

    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic [63:0] sp_i,
    input  logic [63:0] transition_return_pc_i,
    input  logic [31:0] transition_flags_i,

    input  logic        x86_fetch_valid_i,
    input  logic        x86_fetch_fault_i,
    input  logic [31:0] x86_fetch_word_i,
    input  logic [63:0] x86_fallthrough_pc_i,

    input  logic        raw_mem_resp_valid_i,
    input  logic        raw_mem_resp_fault_i,
    input  logic [31:0] raw_mem_resp_word_i,

    input  logic        older_fault_i,
    input  logic        execute_fault_i,
    input  logic        raw_branch_resolved_i,
    input  logic        raw_branch_taken_i,
    input  logic [63:0] raw_branch_target_i,
    input  logic        raw_memory_resolved_i,
    input  logic        raw_memory_fault_i,
    input  logic        memory_order_valid_i,
    input  logic        memory_load_i,
    input  logic        memory_store_i,
    input  logic        memory_atomic_i,
    input  logic        memory_barrier_i,
    input  logic        older_store_pending_i,
    input  logic        store_buffer_full_i,
    input  logic [1:0]  target_frontend_i,
    input  logic [63:0] target_pc_i,
    input  logic        signature_slot_valid_i,

    input  logic        transition_pop_i,

    input  logic        return_recover_valid_i,
    input  logic [63:0] return_target_pc_i,

    input  logic        interrupt_feature_enabled_i,
    input  logic        cpl3_i,
    input  logic        interrupt_i,
    input  logic        user_return_i,
    input  logic [63:0] user_return_pc_i,

    input  logic        trap_valid_i,
    input  logic        trap_monitor_enabled_i,
    input  logic [63:0] trap_monitor_packet_addr_i,
    input  logic        trap_vector_valid_i,
    input  logic [1:0]  trap_vector_frontend_i,
    input  logic [63:0] trap_vector_pc_i,
    input  logic [31:0] trap_reason_i,
    input  logic [31:0] trap_source_mode_i,
    input  logic [63:0] trap_number_i,
    input  logic [63:0] trap_selector_i,
    input  logic [63:0] trap_pc_i,
    input  logic [63:0] trap_resume_pc_i,
    input  logic [63:0] trap_arg0_i,
    input  logic [63:0] trap_arg1_i,
    input  logic [63:0] trap_arg2_i,
    input  logic [63:0] trap_arg3_i,
    input  logic [63:0] trap_arg4_i,
    input  logic [63:0] trap_arg5_i,
    input  logic [63:0] trap_arg6_i,
    input  logic [63:0] trap_arg7_i,
    input  logic        trap_mem_write_resp_valid_i,
    input  logic        trap_mem_write_fault_i,
    input  logic        trap_return_restore_valid_i,
    input  logic [1:0]  trap_return_restore_frontend_i,
    input  logic [63:0] trap_return_restore_pc_i,

    input  logic        abi_signature_set_i,
    input  logic [3:0]  abi_signature_set_slot_i,
    input  logic [7:0]  abi_signature_set_kind_i,
    input  logic [31:0] abi_signature_set_map_i,

    input  logic        cpuid_valid_i,
    input  logic [31:0] cpuid_leaf_i,
    input  logic [31:0] cpuid_subleaf_i,
    input  logic [7:0]  cycle_memory_response_cycles_i,

    output logic        x86_fetch_req_valid_o,
    output logic [63:0] x86_fetch_req_addr_o,
    output logic [4:0]  x86_fetch_req_bytes_o,

    output logic        raw_mem_req_valid_o,
    output logic [63:0] raw_mem_req_addr_o,
    output logic [2:0]  raw_mem_req_bytes_o,

    output logic        wait_fetch_o,
    output logic        wait_execute_o,
    output logic        wait_retire_o,
    output logic        retire_o,
    output logic        commit_transition_o,
    output logic        commit_push_transition_o,
    output logic [1:0]  commit_frontend_o,
    output logic [63:0] commit_pc_o,
    output logic [6:0]  commit_signature_slot_o,

    output logic        transition_pop_valid_o,
    output logic [1:0]  transition_pop_frontend_o,
    output logic [63:0] transition_pop_pc_o,
    output logic [63:0] transition_pop_sp_o,
    output logic [31:0] transition_pop_flags_o,
    output logic        transition_stack_empty_o,
    output logic        transition_stack_full_o,
    output logic        transition_stack_overflow_o,
    output logic        transition_stack_underflow_o,
    output logic        transition_stack_conflict_o,
    output logic [3:0]  transition_stack_depth_o,
    output logic        transition_stack_unavailable_o,

    output logic        return_cookie_hit_o,
    output logic        return_recover_pop_o,
    output logic        return_resume_o,
    output logic [1:0]  return_resume_frontend_o,
    output logic [63:0] return_resume_pc_o,
    output logic [63:0] return_resume_sp_o,
    output logic [31:0] return_resume_flags_o,
    output logic        return_recover_error_o,
    output logic        return_recover_invalid_frontend_o,
    output logic        return_recover_missing_transition_o,
    output logic        return_recover_blocked_o,

    output logic        memory_retire_allowed_o,
    output logic        memory_enqueue_store_o,
    output logic        memory_wait_store_buffer_o,
    output logic        memory_wait_atomic_order_o,
    output logic        memory_barrier_noop_o,
    output logic        memory_aarch64_barrier_noop_o,
    output logic        memory_riscv_fence_noop_o,
    output logic        memory_weak_reorder_allowed_o,
    output logic        memory_invalid_frontend_o,
    output logic        memory_invalid_op_o,
    output logic        memory_fault_o,

    output logic        raw_data_mem_valid_o,
    output logic        raw_data_mem_load_o,
    output logic        raw_data_mem_store_o,
    output logic        raw_data_mem_atomic_o,
    output logic [3:0]  raw_data_mem_access_bytes_o,
    output logic        raw_data_mem_wait_o,
    output logic        raw_data_mem_fault_o,
    output logic        raw_branch_valid_o,
    output logic        raw_branch_unresolved_o,
    output logic        raw_branch_static_target_valid_o,
    output logic [63:0] raw_branch_static_target_o,
    output logic        raw_branch_wait_o,
    output logic        raw_branch_resolved_fault_o,

    output logic        interrupt_enter_x86_o,
    output logic        interrupt_save_interrupted_o,
    output logic [1:0]  interrupt_saved_frontend_o,
    output logic [63:0] interrupt_saved_pc_o,
    output logic        interrupt_restore_raw_o,
    output logic        interrupt_clear_interrupted_o,
    output logic [1:0]  interrupt_next_frontend_o,
    output logic [63:0] interrupt_next_pc_o,
    output logic        interrupted_valid_o,
    output logic [1:0]  interrupted_frontend_o,
    output logic [63:0] interrupted_pc_o,
    output logic        interrupt_error_o,
    output logic        interrupt_invalid_current_frontend_o,
    output logic        interrupt_invalid_current_pc_o,
    output logic        interrupt_invalid_interrupted_frontend_o,
    output logic        interrupt_invalid_interrupted_pc_o,

    output logic        trap_mem_write_valid_o,
    output logic [63:0] trap_mem_write_addr_o,
    output logic [7:0]  trap_mem_write_bytes_o,
    output logic [63:0] trap_mem_write_qword0_o,
    output logic [63:0] trap_mem_write_qword1_o,
    output logic [63:0] trap_mem_write_qword2_o,
    output logic [63:0] trap_mem_write_qword3_o,
    output logic [63:0] trap_mem_write_qword4_o,
    output logic [63:0] trap_mem_write_qword5_o,
    output logic [63:0] trap_mem_write_qword6_o,
    output logic [63:0] trap_mem_write_qword7_o,
    output logic [63:0] trap_mem_write_qword8_o,
    output logic [63:0] trap_mem_write_qword9_o,
    output logic [63:0] trap_mem_write_qword10_o,
    output logic [63:0] trap_mem_write_qword11_o,
    output logic [63:0] trap_mem_write_qword12_o,
    output logic [63:0] trap_mem_write_qword13_o,
    output logic [63:0] trap_mem_write_qword14_o,
    output logic [63:0] trap_mem_write_qword15_o,
    output logic        trap_wait_response_o,
    output logic        trap_packet_delivered_o,
    output logic        trap_fault_o,
    output logic        trap_encode_error_o,
    output logic        trap_packet_mem_fault_o,
    output logic        trap_monitor_disabled_o,
    output logic        trap_noncanonical_packet_o,
    output logic        trap_packet_align_fault_o,
    output logic        trap_packet_range_fault_o,
    output logic        trap_invalid_reason_o,
    output logic        trap_invalid_source_mode_o,
    output logic        trap_vector_apply_o,
    output logic [1:0]  trap_vector_frontend_o,
    output logic [63:0] trap_vector_pc_o,
    output logic        trap_return_decode_o,
    output logic        trap_return_restore_o,
    output logic [1:0]  trap_return_restore_frontend_o,
    output logic [63:0] trap_return_restore_pc_o,

    output logic        abi_signature_set_ok_o,
    output logic        abi_signature_set_error_o,
    output logic        abi_signature_apply_o,
    output logic        abi_signature_valid_o,
    output logic [7:0]  abi_signature_kind_o,
    output logic [6:0]  abi_signature_map_o,
    output logic        abi_signature_tls_base_o,

    output logic        cpuid_hit_o,
    output logic [31:0] cpuid_eax_o,
    output logic [31:0] cpuid_ebx_o,
    output logic [31:0] cpuid_ecx_o,
    output logic [31:0] cpuid_edx_o,

    output logic        cycle_budget_valid_o,
    output logic [7:0]  cycle_fixed_o,
    output logic [7:0]  cycle_variable_o,
    output logic [8:0]  cycle_total_o,
    output logic        cycle_few_cycle_fast_path_o,
    output logic        cycle_waits_for_memory_o,
    output logic        cycle_unsupported_o,
    output logic        cycle_invalid_op_o,
    output logic        cycle_blocked_o,

    output logic        fault_o,
    output logic [63:0] fault_pc_o,
    output logic        older_fault_o,
    output logic        fetch_fault_o,
    output logic        execute_fault_o,
    output logic        control_fault_o,
    output logic        invalid_frontend_o,
    output logic        x86_fetch_wait_o,
    output logic        x86_request_error_o,
    output logic        x86_mem_fault_o,
    output logic        x86_noncanonical_pc_o,
    output logic        x86_range_fault_o,

    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
    output logic        raw_branch_target_valid_o,
    output logic [63:0] raw_branch_target_o,
    output logic        raw_fetch_wait_o,
    output logic        raw_request_error_o,
    output logic        raw_mem_fault_o,
    output logic        raw_noncanonical_pc_o,
    output logic        raw_align_fault_o,
    output logic        raw_range_fault_o,
    output logic        invalid_subop_o,
    output logic        noncanonical_target_o,
    output logic        target_align_fault_o,
    output logic        invalid_signature_slot_o
);
  logic stack_full;
  logic stack_empty;
  logic stack_unavailable;
  logic commit_push_transition;
  logic stack_pop_request;
  logic peek_valid;
  logic [1:0] peek_frontend;
  logic [63:0] peek_pc;
  logic [63:0] peek_sp;
  logic [31:0] peek_flags;
  logic return_pop_raw;
  logic return_resume_raw;
  logic [1:0] return_resume_frontend_raw;
  logic [63:0] return_resume_pc_raw;
  logic [63:0] return_resume_sp_raw;
  logic [31:0] return_resume_flags_raw;
  logic return_error_raw;
  logic return_invalid_frontend_raw;
  logic return_missing_transition_raw;
  logic execute_ready;
  logic execute_fault;
  logic memory_retire_allowed_raw;
  logic memory_enqueue_store_raw;
  logic memory_barrier_noop_raw;
  logic memory_aarch64_barrier_noop_raw;
  logic memory_riscv_fence_noop_raw;
  logic abi_select_valid;
  logic [7:0] abi_select_kind;
  logic [6:0] abi_select_map;
  logic abi_select_tls_base;
  logic interrupted_valid_q;
  logic [1:0] interrupted_frontend_q;
  logic [63:0] interrupted_pc_q;
  logic block_retire;
  logic cycle_valid;
  logic [2:0] cycle_op;
  logic cycle_transition_stack_ready;
  logic raw_insn_valid;
  logic raw_memory_order_valid;
  logic raw_memory_load;
  logic raw_memory_store;
  logic raw_memory_atomic;
  logic raw_memory_barrier;
  logic [3:0] raw_memory_access_bytes;
  logic raw_branch;
  logic raw_call;
  logic raw_return;
  logic raw_trap;
  logic raw_branch_target_valid;
  logic [63:0] raw_branch_target;
  logic raw_unresolved_branch;
  logic raw_unresolved_branch_wait;
  logic raw_resolved_branch_target_valid;
  logic raw_resolved_branch_target_invalid;
  logic raw_commit_branch_target_valid;
  logic [63:0] raw_commit_branch_target;
  logic trap_return_retire;
  logic raw_memory_access;
  logic raw_memory_execute_wait;
  logic raw_memory_execute_fault;
  logic effective_memory_order_valid;
  logic effective_memory_load;
  logic effective_memory_store;
  logic effective_memory_atomic;
  logic effective_memory_barrier;
  logic poly_state_dirty_q;
  logic poly_state_dirty_set;
  logic poly_state_dirty_clear;
  logic interrupt_spill_full_state;
  logic interrupt_spill_header_only;
  logic interrupt_clear_state_dirty;

  localparam logic [6:0] POLY_X86_CTRL_PENTER_MODE = 7'h03;
  localparam logic [6:0] POLY_X86_CTRL_PRESTORE = 7'h70;
  localparam logic [2:0] POLY_CYCLE_OP_PSWITCH = 3'd1;
  localparam logic [2:0] POLY_CYCLE_OP_PCALL_REG = 3'd2;
  localparam logic [2:0] POLY_CYCLE_OP_RETURN_COOKIE = 3'd3;
  localparam logic [2:0] POLY_CYCLE_OP_TRAP_PACKET = 3'd4;

  function automatic logic canonical64(input logic [63:0] addr);
    begin
      canonical64 = addr[63:48] == {16{addr[47]}};
    end
  endfunction

  function automatic logic aligned_raw_target(
      input logic [1:0] frontend,
      input logic [63:0] addr
  );
    begin
      unique case (frontend)
        2'd1: aligned_raw_target = addr[1:0] == 2'b00;
        2'd2: aligned_raw_target = addr[0] == 1'b0;
        default: aligned_raw_target = 1'b1;
      endcase
    end
  endfunction

  assign stack_pop_request = transition_pop_i || (return_pop_raw && !transition_pop_i);
  assign stack_unavailable = stack_full || stack_pop_request;
  assign transition_stack_unavailable_o = stack_unavailable;
  assign commit_push_transition_o = commit_push_transition;
  assign transition_stack_empty_o = stack_empty;

  assign return_recover_blocked_o = transition_pop_i && return_cookie_hit_o;
  assign return_recover_pop_o = return_pop_raw && !transition_pop_i;
  assign return_resume_o = return_resume_raw && !transition_pop_i;
  assign return_resume_frontend_o =
    return_resume_o ? return_resume_frontend_raw : frontend_i;
  assign return_resume_pc_o =
    return_resume_o ? return_resume_pc_raw : return_target_pc_i;
  assign return_resume_sp_o = return_resume_o ? return_resume_sp_raw : 64'd0;
  assign return_resume_flags_o =
    return_resume_o ? return_resume_flags_raw : 32'd0;
  assign return_recover_error_o =
    return_error_raw || return_recover_blocked_o;
  assign return_recover_invalid_frontend_o = return_invalid_frontend_raw;
  assign return_recover_missing_transition_o = return_missing_transition_raw;
  assign execute_ready =
    (!effective_memory_order_valid || memory_retire_allowed_raw) &&
    !raw_unresolved_branch_wait &&
    !raw_memory_execute_wait;
  assign execute_fault =
    execute_fault_i || memory_fault_o || interrupt_error_o || trap_fault_o ||
    raw_resolved_branch_target_invalid || raw_memory_execute_fault;
  assign memory_retire_allowed_o = memory_retire_allowed_raw;
  assign memory_enqueue_store_o = retire_o && memory_enqueue_store_raw;
  assign memory_barrier_noop_o = retire_o && memory_barrier_noop_raw;
  assign memory_aarch64_barrier_noop_o =
    retire_o && memory_aarch64_barrier_noop_raw;
  assign memory_riscv_fence_noop_o =
    retire_o && memory_riscv_fence_noop_raw;
  assign raw_branch_target_valid_o =
    retire_o && raw_commit_branch_target_valid && !poly_ctrl_o;
  assign raw_branch_target_o =
    raw_branch_target_valid_o ? raw_commit_branch_target : 64'd0;
  assign block_retire =
    interrupt_enter_x86_o || interrupt_restore_raw_o ||
    trap_wait_response_o || trap_packet_delivered_o;
  assign abi_signature_apply_o = commit_push_transition_o;
  assign abi_signature_valid_o = commit_push_transition_o && abi_select_valid;
  assign abi_signature_kind_o =
    abi_signature_valid_o ? abi_select_kind : 8'd0;
  assign abi_signature_map_o =
    abi_signature_valid_o ? abi_select_map : 7'd0;
  assign abi_signature_tls_base_o =
    abi_signature_valid_o && abi_select_tls_base;
  assign interrupted_valid_o = interrupted_valid_q;
  assign interrupted_frontend_o = interrupted_frontend_q;
  assign interrupted_pc_o = interrupted_pc_q;
  assign cycle_valid =
    commit_transition_o || return_recover_pop_o ||
    (trap_mem_write_valid_o && !trap_fault_o);
  assign cycle_op =
    (trap_mem_write_valid_o && !trap_fault_o) ? POLY_CYCLE_OP_TRAP_PACKET :
    return_recover_pop_o ? POLY_CYCLE_OP_RETURN_COOKIE :
    commit_push_transition_o ? POLY_CYCLE_OP_PCALL_REG :
    commit_transition_o ? POLY_CYCLE_OP_PSWITCH :
    3'd0;
  assign cycle_transition_stack_ready =
    cycle_op == POLY_CYCLE_OP_PCALL_REG ? !transition_stack_full_o :
    cycle_op == POLY_CYCLE_OP_RETURN_COOKIE ? return_recover_pop_o :
    1'b1;
  assign effective_memory_order_valid =
    memory_order_valid_i || raw_memory_order_valid;
  assign effective_memory_load = memory_load_i || raw_memory_load;
  assign effective_memory_store = memory_store_i || raw_memory_store;
  assign effective_memory_atomic = memory_atomic_i || raw_memory_atomic;
  assign effective_memory_barrier = memory_barrier_i || raw_memory_barrier;
  assign poly_state_dirty_set = retire_o && raw_insn_valid && !poly_ctrl_o;
  assign poly_state_dirty_clear =
    interrupt_clear_state_dirty ||
    (retire_o && poly_ctrl_o &&
      (subop_o == POLY_X86_CTRL_PENTER_MODE ||
       subop_o == POLY_X86_CTRL_PRESTORE));
  assign raw_unresolved_branch =
    raw_branch && !raw_branch_target_valid && !return_recover_pop_o;
  assign raw_unresolved_branch_wait =
    raw_unresolved_branch && !raw_branch_resolved_i;
  assign raw_resolved_branch_target_valid =
    raw_unresolved_branch && raw_branch_resolved_i && raw_branch_taken_i;
  assign raw_resolved_branch_target_invalid =
    raw_resolved_branch_target_valid &&
    (!canonical64(raw_branch_target_i) ||
     !aligned_raw_target(frontend_i, raw_branch_target_i));
  assign raw_commit_branch_target_valid =
    raw_branch_target_valid || raw_resolved_branch_target_valid;
  assign raw_commit_branch_target =
    raw_branch_target_valid ? raw_branch_target : raw_branch_target_i;
  assign raw_branch_valid_o = raw_branch;
  assign raw_branch_unresolved_o = raw_unresolved_branch;
  assign raw_branch_static_target_valid_o = raw_branch && raw_branch_target_valid;
  assign raw_branch_static_target_o =
    raw_branch_static_target_valid_o ? raw_branch_target : 64'd0;
  assign raw_branch_wait_o = raw_unresolved_branch_wait;
  assign raw_branch_resolved_fault_o = raw_resolved_branch_target_invalid;
  assign raw_memory_access =
    raw_memory_order_valid &&
    (raw_memory_load || raw_memory_store || raw_memory_atomic);
  assign raw_memory_execute_wait =
    raw_memory_access && !raw_memory_resolved_i && !raw_memory_fault_i;
  assign raw_memory_execute_fault = raw_memory_access && raw_memory_fault_i;
  assign raw_data_mem_valid_o = raw_memory_access;
  assign raw_data_mem_load_o = raw_memory_access && raw_memory_load;
  assign raw_data_mem_store_o = raw_memory_access && raw_memory_store;
  assign raw_data_mem_atomic_o = raw_memory_access && raw_memory_atomic;
  assign raw_data_mem_access_bytes_o =
    raw_memory_access ? raw_memory_access_bytes : 4'd0;
  assign raw_data_mem_wait_o = raw_memory_execute_wait;
  assign raw_data_mem_fault_o = raw_memory_execute_fault;
  assign trap_vector_apply_o =
    trap_packet_delivered_o && trap_vector_valid_i && !trap_fault_o;
  assign trap_vector_frontend_o =
    trap_vector_apply_o ? trap_vector_frontend_i : frontend_i;
  assign trap_vector_pc_o = trap_vector_apply_o ? trap_vector_pc_i : pc_i;
  assign trap_return_restore_o =
    trap_return_retire && trap_return_restore_valid_i;
  assign trap_return_restore_frontend_o =
    trap_return_restore_o ? trap_return_restore_frontend_i : frontend_i;
  assign trap_return_restore_pc_o =
    trap_return_restore_o ? trap_return_restore_pc_i : pc_i;

  poly_frontend_memory_retire frontend_memory_retire (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .x86_fetch_valid_i(x86_fetch_valid_i),
    .x86_fetch_fault_i(x86_fetch_fault_i),
    .x86_fetch_word_i(x86_fetch_word_i),
    .x86_fallthrough_pc_i(x86_fallthrough_pc_i),
    .raw_mem_resp_valid_i(raw_mem_resp_valid_i),
    .raw_mem_resp_fault_i(raw_mem_resp_fault_i),
    .raw_mem_resp_word_i(raw_mem_resp_word_i),
    .older_fault_i(older_fault_i),
    .execute_ready_i(execute_ready),
    .block_retire_i(block_retire),
    .execute_fault_i(execute_fault),
    .target_frontend_i(target_frontend_i),
    .target_pc_i(target_pc_i),
    .signature_slot_valid_i(signature_slot_valid_i),
    .transition_stack_full_i(stack_unavailable),
    .trap_return_restore_valid_i(trap_return_restore_valid_i),
    .x86_fetch_req_valid_o(x86_fetch_req_valid_o),
    .x86_fetch_req_addr_o(x86_fetch_req_addr_o),
    .x86_fetch_req_bytes_o(x86_fetch_req_bytes_o),
    .raw_mem_req_valid_o(raw_mem_req_valid_o),
    .raw_mem_req_addr_o(raw_mem_req_addr_o),
    .raw_mem_req_bytes_o(raw_mem_req_bytes_o),
    .wait_fetch_o(wait_fetch_o),
    .wait_execute_o(wait_execute_o),
    .wait_retire_o(wait_retire_o),
    .retire_o(retire_o),
    .commit_transition_o(commit_transition_o),
    .commit_push_transition_o(commit_push_transition),
    .commit_frontend_o(commit_frontend_o),
    .commit_pc_o(commit_pc_o),
    .commit_signature_slot_o(commit_signature_slot_o),
    .trap_return_decode_o(trap_return_decode_o),
    .trap_return_retire_o(trap_return_retire),
    .fault_o(fault_o),
    .fault_pc_o(fault_pc_o),
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
    .raw_insn_valid_o(raw_insn_valid),
    .raw_memory_order_valid_o(raw_memory_order_valid),
    .raw_memory_load_o(raw_memory_load),
    .raw_memory_store_o(raw_memory_store),
    .raw_memory_atomic_o(raw_memory_atomic),
    .raw_memory_barrier_o(raw_memory_barrier),
    .raw_memory_access_bytes_o(raw_memory_access_bytes),
    .raw_branch_o(raw_branch),
    .raw_call_o(raw_call),
    .raw_return_o(raw_return),
    .raw_trap_o(raw_trap),
    .raw_branch_target_valid_o(raw_branch_target_valid),
    .raw_branch_target_o(raw_branch_target),
    .raw_fetch_wait_o(raw_fetch_wait_o),
    .raw_request_error_o(raw_request_error_o),
    .raw_mem_fault_o(raw_mem_fault_o),
    .raw_noncanonical_pc_o(raw_noncanonical_pc_o),
    .raw_align_fault_o(raw_align_fault_o),
    .raw_range_fault_o(raw_range_fault_o),
    .invalid_subop_o(invalid_subop_o),
    .noncanonical_target_o(noncanonical_target_o),
    .target_align_fault_o(target_align_fault_o),
    .invalid_signature_slot_o(invalid_signature_slot_o),
    .transition_stack_full_o()
  );

  poly_memory_order memory_order (
    .valid_i(effective_memory_order_valid),
    .frontend_i(frontend_i),
    .load_i(effective_memory_load),
    .store_i(effective_memory_store),
    .atomic_i(effective_memory_atomic),
    .barrier_i(effective_memory_barrier),
    .older_store_pending_i(older_store_pending_i),
    .store_buffer_full_i(store_buffer_full_i),
    .retire_allowed_o(memory_retire_allowed_raw),
    .enqueue_store_o(memory_enqueue_store_raw),
    .wait_store_buffer_o(memory_wait_store_buffer_o),
    .wait_atomic_order_o(memory_wait_atomic_order_o),
    .barrier_noop_o(memory_barrier_noop_raw),
    .aarch64_barrier_noop_o(memory_aarch64_barrier_noop_raw),
    .riscv_fence_noop_o(memory_riscv_fence_noop_raw),
    .weak_reorder_allowed_o(memory_weak_reorder_allowed_o),
    .invalid_frontend_o(memory_invalid_frontend_o),
    .invalid_op_o(memory_invalid_op_o),
    .fault_o(memory_fault_o)
  );

  poly_interrupt_boundary interrupt_boundary (
    .valid_i(valid_i),
    .feature_enabled_i(interrupt_feature_enabled_i),
    .cpl3_i(cpl3_i),
    .interrupt_i(interrupt_i),
    .user_return_i(user_return_i),
    .state_dirty_i(poly_state_dirty_q),
    .current_frontend_i(frontend_i),
    .current_pc_i(pc_i),
    .interrupted_valid_i(interrupted_valid_q),
    .interrupted_frontend_i(interrupted_frontend_q),
    .interrupted_pc_i(interrupted_pc_q),
    .user_return_pc_i(user_return_pc_i),
    .enter_x86_interrupt_o(interrupt_enter_x86_o),
    .save_interrupted_o(interrupt_save_interrupted_o),
    .spill_full_state_o(interrupt_spill_full_state),
    .spill_header_only_o(interrupt_spill_header_only),
    .clear_state_dirty_o(interrupt_clear_state_dirty),
    .saved_frontend_o(interrupt_saved_frontend_o),
    .saved_pc_o(interrupt_saved_pc_o),
    .restore_raw_o(interrupt_restore_raw_o),
    .clear_interrupted_o(interrupt_clear_interrupted_o),
    .next_frontend_o(interrupt_next_frontend_o),
    .next_pc_o(interrupt_next_pc_o),
    .error_o(interrupt_error_o),
    .invalid_current_frontend_o(interrupt_invalid_current_frontend_o),
    .invalid_current_pc_o(interrupt_invalid_current_pc_o),
    .invalid_interrupted_frontend_o(interrupt_invalid_interrupted_frontend_o),
    .invalid_interrupted_pc_o(interrupt_invalid_interrupted_pc_o)
  );

  poly_trap_packet_stage trap_packet_stage (
    .valid_i(trap_valid_i),
    .monitor_enabled_i(trap_monitor_enabled_i),
    .monitor_packet_addr_i(trap_monitor_packet_addr_i),
    .reason_i(trap_reason_i),
    .source_mode_i(trap_source_mode_i),
    .number_i(trap_number_i),
    .selector_i(trap_selector_i),
    .trap_pc_i(trap_pc_i),
    .resume_pc_i(trap_resume_pc_i),
    .arg0_i(trap_arg0_i),
    .arg1_i(trap_arg1_i),
    .arg2_i(trap_arg2_i),
    .arg3_i(trap_arg3_i),
    .arg4_i(trap_arg4_i),
    .arg5_i(trap_arg5_i),
    .arg6_i(trap_arg6_i),
    .arg7_i(trap_arg7_i),
    .mem_write_resp_valid_i(trap_mem_write_resp_valid_i),
    .mem_write_fault_i(trap_mem_write_fault_i),
    .mem_write_valid_o(trap_mem_write_valid_o),
    .mem_write_addr_o(trap_mem_write_addr_o),
    .mem_write_bytes_o(trap_mem_write_bytes_o),
    .mem_write_qword0_o(trap_mem_write_qword0_o),
    .mem_write_qword1_o(trap_mem_write_qword1_o),
    .mem_write_qword2_o(trap_mem_write_qword2_o),
    .mem_write_qword3_o(trap_mem_write_qword3_o),
    .mem_write_qword4_o(trap_mem_write_qword4_o),
    .mem_write_qword5_o(trap_mem_write_qword5_o),
    .mem_write_qword6_o(trap_mem_write_qword6_o),
    .mem_write_qword7_o(trap_mem_write_qword7_o),
    .mem_write_qword8_o(trap_mem_write_qword8_o),
    .mem_write_qword9_o(trap_mem_write_qword9_o),
    .mem_write_qword10_o(trap_mem_write_qword10_o),
    .mem_write_qword11_o(trap_mem_write_qword11_o),
    .mem_write_qword12_o(trap_mem_write_qword12_o),
    .mem_write_qword13_o(trap_mem_write_qword13_o),
    .mem_write_qword14_o(trap_mem_write_qword14_o),
    .mem_write_qword15_o(trap_mem_write_qword15_o),
    .wait_response_o(trap_wait_response_o),
    .packet_delivered_o(trap_packet_delivered_o),
    .fault_o(trap_fault_o),
    .encode_error_o(trap_encode_error_o),
    .packet_mem_fault_o(trap_packet_mem_fault_o),
    .monitor_disabled_o(trap_monitor_disabled_o),
    .noncanonical_packet_o(trap_noncanonical_packet_o),
    .packet_align_fault_o(trap_packet_align_fault_o),
    .packet_range_fault_o(trap_packet_range_fault_o),
    .invalid_reason_o(trap_invalid_reason_o),
    .invalid_source_mode_o(trap_invalid_source_mode_o)
  );

  poly_abi_signature_slots abi_signature_slots (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .set_i(abi_signature_set_i),
    .set_slot_i(abi_signature_set_slot_i),
    .set_kind_i(abi_signature_set_kind_i),
    .set_map_i(abi_signature_set_map_i),
    .set_ok_o(abi_signature_set_ok_o),
    .set_error_o(abi_signature_set_error_o),
    .select_slot_i(commit_signature_slot_o[3:0]),
    .select_valid_o(abi_select_valid),
    .select_kind_o(abi_select_kind),
    .select_map_o(abi_select_map),
    .select_tls_base_o(abi_select_tls_base)
  );

  poly_cpuid_rom cpuid_rom (
    .valid_i(cpuid_valid_i),
    .leaf_i(cpuid_leaf_i),
    .subleaf_i(cpuid_subleaf_i),
    .hit_o(cpuid_hit_o),
    .eax_o(cpuid_eax_o),
    .ebx_o(cpuid_ebx_o),
    .ecx_o(cpuid_ecx_o),
    .edx_o(cpuid_edx_o)
  );

  poly_transition_cycle_budget transition_cycle_budget (
    .valid_i(cycle_valid),
    .op_i(cycle_op),
    .register_only_signature_i(abi_signature_valid_o),
    .signature_slot_valid_i(abi_signature_valid_o),
    .transition_stack_ready_i(cycle_transition_stack_ready),
    .monitor_packet_ready_i(trap_mem_write_valid_o),
    .memory_response_cycles_i(cycle_memory_response_cycles_i),
    .budget_valid_o(cycle_budget_valid_o),
    .fixed_cycles_o(cycle_fixed_o),
    .variable_cycles_o(cycle_variable_o),
    .total_cycles_o(cycle_total_o),
    .few_cycle_fast_path_o(cycle_few_cycle_fast_path_o),
    .waits_for_memory_o(cycle_waits_for_memory_o),
    .unsupported_o(cycle_unsupported_o),
    .invalid_op_o(cycle_invalid_op_o),
    .blocked_o(cycle_blocked_o)
  );

  poly_transition_stack transition_stack (
    .clk_i(clk_i),
    .rst_ni(rst_ni),
    .push_i(commit_push_transition),
    .push_frontend_i(frontend_i),
    .push_pc_i(transition_return_pc_i),
    .push_sp_i(sp_i),
    .push_flags_i(transition_flags_i),
    .pop_i(stack_pop_request),
    .pop_valid_o(transition_pop_valid_o),
    .pop_frontend_o(transition_pop_frontend_o),
    .pop_pc_o(transition_pop_pc_o),
    .pop_sp_o(transition_pop_sp_o),
    .pop_flags_o(transition_pop_flags_o),
    .peek_valid_o(peek_valid),
    .peek_frontend_o(peek_frontend),
    .peek_pc_o(peek_pc),
    .peek_sp_o(peek_sp),
    .peek_flags_o(peek_flags),
    .empty_o(stack_empty),
    .full_o(stack_full),
    .overflow_o(transition_stack_overflow_o),
    .underflow_o(transition_stack_underflow_o),
    .conflict_o(transition_stack_conflict_o),
    .depth_o(transition_stack_depth_o)
  );

  poly_return_cookie_recover return_cookie_recover (
    .valid_i(return_recover_valid_i),
    .current_frontend_i(frontend_i),
    .return_target_pc_i(return_target_pc_i),
    .transition_empty_i(!peek_valid),
    .pop_frontend_i(peek_frontend),
    .pop_pc_i(peek_pc),
    .pop_sp_i(peek_sp),
    .pop_flags_i(peek_flags),
    .cookie_hit_o(return_cookie_hit_o),
    .pop_transition_o(return_pop_raw),
    .resume_o(return_resume_raw),
    .resume_frontend_o(return_resume_frontend_raw),
    .resume_pc_o(return_resume_pc_raw),
    .resume_sp_o(return_resume_sp_raw),
    .resume_flags_o(return_resume_flags_raw),
    .error_o(return_error_raw),
    .invalid_frontend_o(return_invalid_frontend_raw),
    .missing_transition_o(return_missing_transition_raw)
  );

  assign transition_stack_full_o = stack_full;

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      poly_state_dirty_q <= 1'b0;
    end
    else if (poly_state_dirty_clear) begin
      poly_state_dirty_q <= 1'b0;
    end
    else if (poly_state_dirty_set) begin
      poly_state_dirty_q <= 1'b1;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      interrupted_valid_q <= 1'b0;
      interrupted_frontend_q <= 2'd0;
      interrupted_pc_q <= 64'd0;
    end
    else if (interrupt_save_interrupted_o) begin
      interrupted_valid_q <= 1'b1;
      interrupted_frontend_q <= interrupt_saved_frontend_o;
      interrupted_pc_q <= interrupt_saved_pc_o;
    end
    else if (interrupt_clear_interrupted_o) begin
      interrupted_valid_q <= 1'b0;
      interrupted_frontend_q <= 2'd0;
      interrupted_pc_q <= 64'd0;
    end
  end
endmodule
