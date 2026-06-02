// Formal harness for poly_memory_order.
//
// This file captures the memory-order contract as assertions. It is intended
// for SymbiYosys/Yosys-style formal runs when that toolchain is available; the
// repository currently keeps a static check for the property set.
module poly_memory_order_formal (
    input logic clk
);
`ifdef FORMAL
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  (* anyseq *) logic       valid;
  (* anyseq *) logic [1:0] frontend;
  (* anyseq *) logic       load;
  (* anyseq *) logic       store;
  (* anyseq *) logic       atomic;
  (* anyseq *) logic       barrier;
  (* anyseq *) logic       older_store_pending;
  (* anyseq *) logic       store_buffer_full;

  logic retire_allowed;
  logic enqueue_store;
  logic wait_store_buffer;
  logic wait_atomic_order;
  logic barrier_noop;
  logic aarch64_barrier_noop;
  logic riscv_fence_noop;
  logic weak_reorder_allowed;
  logic invalid_frontend;
  logic invalid_op;
  logic fault;

  logic frontend_valid;
  logic raw_frontend;
  logic op_present;

  poly_memory_order dut (
    .valid_i(valid),
    .frontend_i(frontend),
    .load_i(load),
    .store_i(store),
    .atomic_i(atomic),
    .barrier_i(barrier),
    .older_store_pending_i(older_store_pending),
    .store_buffer_full_i(store_buffer_full),
    .retire_allowed_o(retire_allowed),
    .enqueue_store_o(enqueue_store),
    .wait_store_buffer_o(wait_store_buffer),
    .wait_atomic_order_o(wait_atomic_order),
    .barrier_noop_o(barrier_noop),
    .aarch64_barrier_noop_o(aarch64_barrier_noop),
    .riscv_fence_noop_o(riscv_fence_noop),
    .weak_reorder_allowed_o(weak_reorder_allowed),
    .invalid_frontend_o(invalid_frontend),
    .invalid_op_o(invalid_op),
    .fault_o(fault)
  );

  always_comb begin
    frontend_valid =
      frontend == POLY_FRONTEND_X86 ||
      frontend == POLY_FRONTEND_AARCH64 ||
      frontend == POLY_FRONTEND_RISCV;
    raw_frontend =
      frontend == POLY_FRONTEND_AARCH64 ||
      frontend == POLY_FRONTEND_RISCV;
    op_present = load || store || atomic || barrier;
  end

  always_ff @(posedge clk) begin
    // Foreign modes inherit x86 TSO; the block must never expose a weak
    // reordering permission while the advertised memory model is TSO.
    assert (!weak_reorder_allowed);

    assert (invalid_frontend == (valid && !frontend_valid));
    assert (invalid_op == (valid && !op_present));
    assert (fault == (invalid_frontend || invalid_op));

    assert (wait_store_buffer ==
      (valid && !fault && store_buffer_full && (store || atomic)));
    assert (wait_atomic_order ==
      (valid && !fault && atomic && older_store_pending));

    assert (retire_allowed ==
      (valid && !fault && !wait_store_buffer && !wait_atomic_order));
    assert (enqueue_store == (retire_allowed && store));

    // AArch64 barriers and RISC-V fences retire as TSO no-ops only in raw
    // foreign modes. They must not create a memory operation or weaken order.
    assert (barrier_noop == (retire_allowed && barrier && raw_frontend));
    assert (aarch64_barrier_noop ==
      (barrier_noop && frontend == POLY_FRONTEND_AARCH64));
    assert (riscv_fence_noop ==
      (barrier_noop && frontend == POLY_FRONTEND_RISCV));

    if (fault)
      assert (!retire_allowed && !enqueue_store);
    if (atomic && older_store_pending && valid && !fault)
      assert (wait_atomic_order && !retire_allowed);
    if ((store || atomic) && store_buffer_full && valid && !fault)
      assert (wait_store_buffer && !retire_allowed);
  end
`endif
endmodule
