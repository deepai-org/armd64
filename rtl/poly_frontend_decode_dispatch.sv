// Poly frontend fetch/decode dispatch boundary.
//
// This block normalizes one already-fetched instruction word from the active
// frontend into the Poly control decoder. It is intentionally policy-free:
// memory request issue, TLB/page faults, ABI translation, and OS trap handling
// are outside this stage.
module poly_frontend_decode_dispatch (
    input  logic        valid_i,
    input  logic [1:0]  frontend_i,
    input  logic [63:0] pc_i,
    input  logic [31:0] fetch_word_i,
    input  logic [63:0] x86_fallthrough_pc_i,

    output logic        raw_fetch_o,
    output logic [63:0] fetch_addr_o,
    output logic [2:0]  fetch_bytes_o,
    output logic [31:0] insn_o,
    output logic [63:0] fallthrough_pc_o,

    output logic        decode_valid_o,
    output logic        poly_ctrl_o,
    output logic [6:0]  subop_o,
    output logic        call_sig_imm_o,
    output logic [6:0]  signature_slot_o,

    output logic        raw_insn_valid_o,
    output logic        raw_memory_order_valid_o,
    output logic        raw_memory_load_o,
    output logic        raw_memory_store_o,
    output logic        raw_memory_atomic_o,
    output logic        raw_memory_barrier_o,
    output logic        raw_branch_o,
    output logic        raw_call_o,
    output logic        raw_return_o,
    output logic        raw_trap_o,
    output logic        raw_branch_target_valid_o,
    output logic [63:0] raw_branch_target_o,

    output logic        raw_align_fault_o
);
  logic raw_align_fault;
  logic [31:0] raw_insn;
  logic [63:0] raw_next_pc;
  logic [31:0] decode_insn;

  poly_raw_fetch_plan raw_fetch_plan (
    .valid_i(valid_i),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .fetch_word_i(fetch_word_i),
    .raw_fetch_o(raw_fetch_o),
    .align_fault_o(raw_align_fault),
    .fetch_addr_o(fetch_addr_o),
    .fetch_bytes_o(fetch_bytes_o),
    .insn_o(raw_insn),
    .next_pc_o(raw_next_pc)
  );

  always_comb begin
    decode_insn = raw_fetch_o ? raw_insn : fetch_word_i;
    decode_valid_o = valid_i && !raw_align_fault;
    insn_o = decode_insn;
    fallthrough_pc_o = raw_fetch_o ? raw_next_pc : x86_fallthrough_pc_i;
    raw_align_fault_o = raw_align_fault;
  end

  poly_ctrl_decode ctrl_decode (
    .valid_i(decode_valid_o),
    .frontend_i(frontend_i),
    .insn_i(decode_insn),
    .poly_ctrl_o(poly_ctrl_o),
    .subop_o(subop_o),
    .call_sig_imm_o(call_sig_imm_o),
    .signature_slot_o(signature_slot_o)
  );

  poly_raw_insn_decode raw_insn_decode (
    .valid_i(decode_valid_o && raw_fetch_o),
    .frontend_i(frontend_i),
    .pc_i(pc_i),
    .insn_i(decode_insn),
    .raw_insn_valid_o(raw_insn_valid_o),
    .memory_order_valid_o(raw_memory_order_valid_o),
    .memory_load_o(raw_memory_load_o),
    .memory_store_o(raw_memory_store_o),
    .memory_atomic_o(raw_memory_atomic_o),
    .memory_barrier_o(raw_memory_barrier_o),
    .branch_o(raw_branch_o),
    .call_o(raw_call_o),
    .return_o(raw_return_o),
    .trap_o(raw_trap_o),
    .branch_target_valid_o(raw_branch_target_valid_o),
    .branch_target_o(raw_branch_target_o)
  );
endmodule
