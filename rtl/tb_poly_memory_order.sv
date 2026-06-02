`timescale 1ns/1ps

module tb_poly_memory_order;
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;

  logic valid;
  logic [1:0] frontend;
  logic load;
  logic store;
  logic atomic;
  logic barrier;
  logic older_store_pending;
  logic store_buffer_full;

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

  task automatic clear_inputs;
    begin
      valid = 1'b1;
      frontend = POLY_FRONTEND_X86;
      load = 1'b0;
      store = 1'b0;
      atomic = 1'b0;
      barrier = 1'b0;
      older_store_pending = 1'b0;
      store_buffer_full = 1'b0;
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

  task automatic check_no_faults;
    begin
      check(!fault && !invalid_frontend && !invalid_op, "no fault flags");
    end
  endtask

  initial begin
    clear_inputs();
    valid = 1'b0;
    load = 1'b1;
    #1;
    check(!retire_allowed && !enqueue_store, "invalid cycle does not retire");
    check(!fault && !weak_reorder_allowed, "invalid cycle no fault or weak reorder");

    clear_inputs();
    frontend = POLY_FRONTEND_AARCH64;
    load = 1'b1;
    older_store_pending = 1'b1;
    #1;
    check(retire_allowed, "load retires despite older store under TSO emulation");
    check(!wait_atomic_order && !wait_store_buffer, "load does not wait");
    check(!weak_reorder_allowed, "load weak reorder never allowed");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_RISCV;
    store = 1'b1;
    #1;
    check(retire_allowed && enqueue_store, "store retires and enqueues");
    check(!wait_store_buffer, "store no wait");
    check(!weak_reorder_allowed, "store weak reorder never allowed");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_RISCV;
    store = 1'b1;
    store_buffer_full = 1'b1;
    #1;
    check(!retire_allowed && !enqueue_store, "full store buffer blocks store");
    check(wait_store_buffer && !wait_atomic_order, "store waits on buffer");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_AARCH64;
    load = 1'b1;
    store = 1'b1;
    atomic = 1'b1;
    older_store_pending = 1'b1;
    #1;
    check(!retire_allowed && !enqueue_store, "atomic waits for older store");
    check(wait_atomic_order && !wait_store_buffer, "atomic order wait");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_AARCH64;
    load = 1'b1;
    store = 1'b1;
    atomic = 1'b1;
    store_buffer_full = 1'b1;
    #1;
    check(!retire_allowed && !enqueue_store, "atomic waits for store buffer");
    check(wait_store_buffer && !wait_atomic_order, "atomic store-buffer wait");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_X86;
    load = 1'b1;
    store = 1'b1;
    atomic = 1'b1;
    #1;
    check(retire_allowed && enqueue_store, "x86 atomic retires when unblocked");
    check(!wait_store_buffer && !wait_atomic_order, "x86 atomic no wait");
    check(!weak_reorder_allowed, "x86 atomic weak reorder never allowed");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_AARCH64;
    barrier = 1'b1;
    older_store_pending = 1'b1;
    store_buffer_full = 1'b1;
    #1;
    check(retire_allowed, "aarch64 barrier retires");
    check(barrier_noop && aarch64_barrier_noop, "aarch64 barrier no-op");
    check(!riscv_fence_noop, "aarch64 barrier not riscv fence");
    check_no_faults();

    clear_inputs();
    frontend = POLY_FRONTEND_RISCV;
    barrier = 1'b1;
    older_store_pending = 1'b1;
    store_buffer_full = 1'b1;
    #1;
    check(retire_allowed, "riscv fence retires");
    check(barrier_noop && riscv_fence_noop, "riscv fence no-op");
    check(!aarch64_barrier_noop, "riscv fence not aarch64 barrier");
    check_no_faults();

    clear_inputs();
    frontend = 2'd3;
    load = 1'b1;
    #1;
    check(fault && invalid_frontend, "invalid frontend faults");
    check(!retire_allowed, "invalid frontend blocks retire");

    clear_inputs();
    frontend = POLY_FRONTEND_X86;
    #1;
    check(fault && invalid_op, "missing op faults");
    check(!retire_allowed, "missing op blocks retire");

    $display("POLY_RTL_MEMORY_ORDER_SIM_OK");
    $finish;
  end
endmodule
