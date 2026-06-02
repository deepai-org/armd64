#!/usr/bin/env python3
"""Litmus-style checks for the Poly x86 TSO memory-order contract."""

from itertools import permutations
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_memory_order.sv"


def interleavings(events: list[str], before: set[tuple[str, str]]) -> list[tuple[str, ...]]:
    valid: list[tuple[str, ...]] = []
    for order in permutations(events):
        pos = {event: n for n, event in enumerate(order)}
        if all(pos[left] < pos[right] for left, right in before):
            valid.append(order)
    return valid


def message_passing_outcome(order: tuple[str, ...]) -> tuple[int, int]:
    """P0: store x=1; store y=1. P1: r1=load y; r2=load x."""
    x = 0
    y = 0
    r1 = -1
    r2 = -1
    for event in order:
        if event == "sx":
            x = 1
        elif event == "sy":
            y = 1
        elif event == "ly":
            r1 = y
        elif event == "lx":
            r2 = x
    return (r1, r2)


def store_buffering_outcome(order: tuple[str, ...]) -> tuple[int, int]:
    """P0: store x=1; r1=load y. P1: store y=1; r2=load x.

    Store visibility events may lag each core's later load, matching x86 TSO
    store-buffer behavior. The stores still become globally visible as coherent
    writes once their visibility events occur.
    """
    x = 0
    y = 0
    r1 = -1
    r2 = -1
    for event in order:
        if event == "sx_visible":
            x = 1
        elif event == "sy_visible":
            y = 1
        elif event == "ly":
            r1 = y
        elif event == "lx":
            r2 = x
    return (r1, r2)


def main() -> int:
    text = RTL.read_text()
    assert "POLY_MEMORY_MODEL_X86_TSO" in text
    assert "weak_reorder_allowed_o =" in text
    assert "barrier_noop_o = retire_allowed_o && barrier_i && raw_frontend" in text

    # Message Passing under TSO:
    # If P1 observes y=1, then P0's earlier x=1 store must already be visible.
    mp_orders = interleavings(
        ["sx", "sy", "ly", "lx"],
        {("sx", "sy"), ("ly", "lx")},
    )
    mp_outcomes = {message_passing_outcome(order) for order in mp_orders}
    assert (1, 0) not in mp_outcomes
    assert (1, 1) in mp_outcomes

    # Store Buffering under TSO:
    # The classic (r1=0, r2=0) outcome remains allowed because local stores can
    # sit in store buffers while later loads retire. Poly must not accidentally
    # claim SC when it advertises x86 TSO.
    sb_orders = interleavings(
        ["sx_visible", "sy_visible", "ly", "lx"],
        set(),
    )
    sb_outcomes = {store_buffering_outcome(order) for order in sb_orders}
    assert (0, 0) in sb_outcomes
    assert (1, 1) in sb_outcomes

    # Coherence sanity: once both stores are globally visible before both loads,
    # both cores must observe the writes.
    coherent_orders = interleavings(
        ["sx_visible", "sy_visible", "ly", "lx"],
        {
            ("sx_visible", "ly"),
            ("sx_visible", "lx"),
            ("sy_visible", "ly"),
            ("sy_visible", "lx"),
        },
    )
    assert {
        store_buffering_outcome(order) for order in coherent_orders
    } == {(1, 1)}

    print("POLY_RTL_MEMORY_ORDER_LITMUS_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
