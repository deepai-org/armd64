#!/usr/bin/env python3
"""Static checks for rtl/poly_memory_order_formal.sv."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_memory_order_formal.sv"


def main() -> int:
    text = RTL.read_text()

    required_fragments = [
        "`ifdef FORMAL",
        "(* anyseq *) logic",
        "poly_memory_order dut",
        "assert (!weak_reorder_allowed);",
        "assert (invalid_frontend == (valid && !frontend_valid));",
        "assert (invalid_op == (valid && !op_present));",
        "assert (fault == (invalid_frontend || invalid_op));",
        "assert (wait_store_buffer ==",
        "assert (wait_atomic_order ==",
        "assert (retire_allowed ==",
        "assert (enqueue_store == (retire_allowed && store));",
        "assert (barrier_noop == (retire_allowed && barrier && raw_frontend));",
        "assert (aarch64_barrier_noop ==",
        "assert (riscv_fence_noop ==",
        "if (fault)",
        "if (atomic && older_store_pending && valid && !fault)",
        "if ((store || atomic) && store_buffer_full && valid && !fault)",
    ]
    for fragment in required_fragments:
        assert fragment in text, f"missing formal property fragment: {fragment}"

    assert text.count("assert ") >= 14

    print("POLY_RTL_MEMORY_ORDER_FORMAL_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
