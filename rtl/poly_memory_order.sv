// Poly memory-order retirement policy.
//
// Poly foreign frontends inherit the x86 TSO memory model. This block does not
// implement a cache, TLB, store buffer, or atomic unit; it gates memory-op
// retirement so weaker AArch64/RISC-V ordering is never exposed architecturally.
module poly_memory_order (
    input  logic       valid_i,
    input  logic [1:0] frontend_i,
    input  logic       load_i,
    input  logic       store_i,
    input  logic       atomic_i,
    input  logic       barrier_i,
    input  logic       older_store_pending_i,
    input  logic       store_buffer_full_i,

    output logic       retire_allowed_o,
    output logic       enqueue_store_o,
    output logic       wait_store_buffer_o,
    output logic       wait_atomic_order_o,
    output logic       barrier_noop_o,
    output logic       aarch64_barrier_noop_o,
    output logic       riscv_fence_noop_o,
    output logic       weak_reorder_allowed_o,
    output logic       invalid_frontend_o,
    output logic       invalid_op_o,
    output logic       fault_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  localparam logic [31:0] POLY_MEMORY_MODEL_X86_TSO = 32'd1;

  logic frontend_valid;
  logic raw_frontend;
  logic op_present;
  logic memory_op;

  always_comb begin
    frontend_valid =
      frontend_i == POLY_FRONTEND_X86 ||
      frontend_i == POLY_FRONTEND_AARCH64 ||
      frontend_i == POLY_FRONTEND_RISCV;
    raw_frontend =
      frontend_i == POLY_FRONTEND_AARCH64 ||
      frontend_i == POLY_FRONTEND_RISCV;
    op_present = load_i || store_i || atomic_i || barrier_i;
    memory_op = load_i || store_i || atomic_i;

    invalid_frontend_o = valid_i && !frontend_valid;
    invalid_op_o = valid_i && !op_present;
    fault_o = invalid_frontend_o || invalid_op_o;

    wait_store_buffer_o =
      valid_i && !fault_o && store_buffer_full_i && (store_i || atomic_i);
    wait_atomic_order_o =
      valid_i && !fault_o && atomic_i && older_store_pending_i;

    retire_allowed_o =
      valid_i && !fault_o && !wait_store_buffer_o && !wait_atomic_order_o;
    enqueue_store_o = retire_allowed_o && store_i;

    barrier_noop_o = retire_allowed_o && barrier_i && raw_frontend;
    aarch64_barrier_noop_o =
      barrier_noop_o && frontend_i == POLY_FRONTEND_AARCH64;
    riscv_fence_noop_o =
      barrier_noop_o && frontend_i == POLY_FRONTEND_RISCV;

    weak_reorder_allowed_o =
      valid_i && memory_op && POLY_MEMORY_MODEL_X86_TSO != 32'd1;
  end
endmodule
