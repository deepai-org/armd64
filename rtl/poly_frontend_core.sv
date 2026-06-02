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

    output logic        fault_o,
    output logic [63:0] fault_pc_o,
    output logic        older_fault_o,
    output logic        fetch_fault_o,
    output logic        execute_fault_o,
    output logic        control_fault_o,
    output logic        invalid_frontend_o,

    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
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
  logic interrupted_valid_q;
  logic [1:0] interrupted_frontend_q;
  logic [63:0] interrupted_pc_q;
  logic block_retire;

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
  assign execute_ready = !memory_order_valid_i || memory_retire_allowed_raw;
  assign execute_fault = execute_fault_i || memory_fault_o || interrupt_error_o;
  assign memory_retire_allowed_o = memory_retire_allowed_raw;
  assign memory_enqueue_store_o = retire_o && memory_enqueue_store_raw;
  assign memory_barrier_noop_o = retire_o && memory_barrier_noop_raw;
  assign memory_aarch64_barrier_noop_o =
    retire_o && memory_aarch64_barrier_noop_raw;
  assign memory_riscv_fence_noop_o =
    retire_o && memory_riscv_fence_noop_raw;
  assign block_retire = interrupt_enter_x86_o || interrupt_restore_raw_o;
  assign interrupted_valid_o = interrupted_valid_q;
  assign interrupted_frontend_o = interrupted_frontend_q;
  assign interrupted_pc_o = interrupted_pc_q;

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
    .fault_o(fault_o),
    .fault_pc_o(fault_pc_o),
    .older_fault_o(older_fault_o),
    .fetch_fault_o(fetch_fault_o),
    .execute_fault_o(execute_fault_o),
    .control_fault_o(control_fault_o),
    .invalid_frontend_o(invalid_frontend_o),
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
    .invalid_signature_slot_o(invalid_signature_slot_o),
    .transition_stack_full_o()
  );

  poly_memory_order memory_order (
    .valid_i(memory_order_valid_i),
    .frontend_i(frontend_i),
    .load_i(memory_load_i),
    .store_i(memory_store_i),
    .atomic_i(memory_atomic_i),
    .barrier_i(memory_barrier_i),
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
    .current_frontend_i(frontend_i),
    .current_pc_i(pc_i),
    .interrupted_valid_i(interrupted_valid_q),
    .interrupted_frontend_i(interrupted_frontend_q),
    .interrupted_pc_i(interrupted_pc_q),
    .user_return_pc_i(user_return_pc_i),
    .enter_x86_interrupt_o(interrupt_enter_x86_o),
    .save_interrupted_o(interrupt_save_interrupted_o),
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
