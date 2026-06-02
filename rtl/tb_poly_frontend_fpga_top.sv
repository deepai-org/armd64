`timescale 1ns/1ps

module tb_poly_frontend_fpga_top;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [6:0] POLY_X86_CTRL_PCALL_SIG_MODE = 7'h2d;

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
  logic [1:0] state_frontend_o;
  logic [63:0] state_pc_o;
  logic state_update_o;
  logic state_hold_o;
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
  logic [3:0] transition_stack_depth_o;
  logic fault_o;
  logic poly_ctrl_o;
  logic [6:0] subop_o;
  logic raw_data_mem_valid_o;
  logic raw_data_mem_load_o;
  logic [3:0] raw_data_mem_access_bytes_o;
  logic raw_data_mem_wait_o;
  logic raw_data_mem_req_valid_o;
  logic [63:0] raw_data_mem_req_addr_o;
  logic [3:0] raw_data_mem_req_bytes_o;
  logic raw_data_mem_req_load_o;
  logic raw_data_mem_req_error_o;
  logic raw_data_mem_resp_wait_o;
  logic raw_data_mem_resp_resolved_o;
  logic raw_data_mem_resp_fault_o;

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
    .state_frontend_o(state_frontend_o),
    .state_pc_o(state_pc_o),
    .state_update_o(state_update_o),
    .state_hold_o(state_hold_o),
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
    .transition_stack_empty_o(),
    .transition_stack_full_o(),
    .transition_stack_depth_o(transition_stack_depth_o),
    .return_cookie_hit_o(),
    .return_recover_pop_o(),
    .return_resume_o(),
    .return_resume_frontend_o(),
    .return_resume_pc_o(),
    .raw_data_mem_valid_o(raw_data_mem_valid_o),
    .raw_data_mem_load_o(raw_data_mem_load_o),
    .raw_data_mem_store_o(),
    .raw_data_mem_atomic_o(),
    .raw_data_mem_access_bytes_o(raw_data_mem_access_bytes_o),
    .raw_data_mem_wait_o(raw_data_mem_wait_o),
    .raw_data_mem_fault_o(),
    .raw_data_mem_req_valid_o(raw_data_mem_req_valid_o),
    .raw_data_mem_req_addr_o(raw_data_mem_req_addr_o),
    .raw_data_mem_req_bytes_o(raw_data_mem_req_bytes_o),
    .raw_data_mem_req_load_o(raw_data_mem_req_load_o),
    .raw_data_mem_req_store_o(),
    .raw_data_mem_req_atomic_o(),
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
    .trap_mem_write_valid_o(),
    .trap_mem_write_addr_o(),
    .trap_mem_write_bytes_o(),
    .trap_wait_response_o(),
    .trap_packet_delivered_o(),
    .trap_fault_o(),
    .cpuid_hit_o(),
    .cpuid_eax_o(),
    .cpuid_ebx_o(),
    .cpuid_ecx_o(),
    .cpuid_edx_o(),
    .cycle_budget_valid_o(),
    .cycle_total_o(),
    .cycle_few_cycle_fast_path_o(),
    .fault_o(fault_o),
    .fault_pc_o(),
    .fetch_fault_o(),
    .execute_fault_o(),
    .control_fault_o(),
    .invalid_frontend_o(),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o)
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
    tick();

    instr_resp_valid_i = 1'b1;
    instr_resp_frontend_i = POLY_FRONTEND_X86;
    instr_resp_word_i = 32'h52800000;
    #1;
    check(wait_fetch_o && !retire_o, "wrong frontend response is ignored");

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

    init_i = 1'b1;
    init_frontend_i = POLY_FRONTEND_X86;
    init_pc_i = 64'h1000;
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_X86 &&
      state_pc_o == 64'h1000, "reinit to x86 frontend");

    valid_i = 1'b1;
    sp_i = 64'h8000;
    transition_return_pc_i = 64'h1004;
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
    check(commit_frontend_o == POLY_FRONTEND_AARCH64 &&
      commit_pc_o == 64'h5000, "x86 pcall commits target frontend");
    tick();
    clear_inputs();
    #1;
    check(state_frontend_o == POLY_FRONTEND_AARCH64 &&
      state_pc_o == 64'h5000, "x86 pcall updates state");
    check(transition_stack_depth_o == 4'd1, "x86 pcall pushes transition");

    $display("POLY_RTL_FRONTEND_FPGA_TOP_SIM_OK");
    $finish;
  end
endmodule
