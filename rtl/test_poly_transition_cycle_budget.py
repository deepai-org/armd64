#!/usr/bin/env python3
"""Cycle-budget checks for rtl/poly_transition_cycle_budget.sv."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_transition_cycle_budget.sv"


def parse_sv_localparams(path: Path) -> dict[str, int]:
    text = path.read_text()
    constants: dict[str, int] = {}
    pattern = re.compile(
        r"localparam\s+logic\s+(?:\[[^\]]+\]\s+)?([A-Z0-9_]+)\s*=\s*([^;]+);"
    )
    for name, expr in pattern.findall(text):
        expr = expr.strip()
        sv = re.fullmatch(r"(\d+)'([hdb])([0-9a-fA-F_]+)", expr)
        if sv:
            _, base, value = sv.groups()
            constants[name] = int(value.replace("_", ""), {"h": 16, "d": 10, "b": 2}[base])
        else:
            constants[name] = int(expr, 0)
    return constants


def budget(
    valid: bool,
    op: int,
    register_only: bool,
    signature_valid: bool,
    stack_ready: bool,
    packet_ready: bool,
    memory_cycles: int,
    c: dict[str, int],
) -> dict[str, int | bool]:
    supported = op in {
        c["POLY_CYCLE_OP_PSWITCH"],
        c["POLY_CYCLE_OP_PCALL_REG"],
        c["POLY_CYCLE_OP_RETURN_COOKIE"],
        c["POLY_CYCLE_OP_TRAP_PACKET"],
    }
    requires_signature = op == c["POLY_CYCLE_OP_PCALL_REG"]
    requires_stack = op in {
        c["POLY_CYCLE_OP_PCALL_REG"],
        c["POLY_CYCLE_OP_RETURN_COOKIE"],
    }
    requires_packet = op == c["POLY_CYCLE_OP_TRAP_PACKET"]
    invalid = valid and not supported
    unsupported = valid and requires_signature and not register_only
    blocked = (
        valid and supported and not unsupported and (
            (requires_signature and not signature_valid) or
            (requires_stack and not stack_ready) or
            (requires_packet and not packet_ready)
        )
    )
    fixed = {
        c["POLY_CYCLE_OP_PSWITCH"]: 3,
        c["POLY_CYCLE_OP_PCALL_REG"]: 4,
        c["POLY_CYCLE_OP_RETURN_COOKIE"]: 3,
        c["POLY_CYCLE_OP_TRAP_PACKET"]: 2,
    }.get(op, 0)
    waits_for_memory = valid and op == c["POLY_CYCLE_OP_TRAP_PACKET"] and not invalid
    variable = memory_cycles if waits_for_memory else 0
    budget_valid = valid and supported and not unsupported and not blocked
    few_cycle = (
        budget_valid and not waits_for_memory and
        fixed <= c["POLY_CYCLE_FEW_CYCLE_LIMIT"]
    )
    return {
        "valid": budget_valid,
        "fixed": fixed,
        "variable": variable,
        "total": fixed + variable,
        "few_cycle": few_cycle,
        "waits_for_memory": waits_for_memory,
        "unsupported": unsupported,
        "invalid": invalid,
        "blocked": blocked,
    }


def main() -> int:
    c = parse_sv_localparams(RTL)
    text = RTL.read_text()
    assert c["POLY_CYCLE_FEW_CYCLE_LIMIT"] == 4
    assert "register_only_signature_i" in text
    assert "memory_response_cycles_i" in text

    pswitch = budget(
        True, c["POLY_CYCLE_OP_PSWITCH"], True, True, True, True, 0, c
    )
    assert pswitch["valid"] and pswitch["fixed"] == 3
    assert pswitch["total"] == 3 and pswitch["few_cycle"]
    assert not pswitch["waits_for_memory"]

    pcall = budget(
        True, c["POLY_CYCLE_OP_PCALL_REG"], True, True, True, True, 0, c
    )
    assert pcall["valid"] and pcall["fixed"] == 4
    assert pcall["total"] == 4 and pcall["few_cycle"]

    memory_shaped_pcall = budget(
        True, c["POLY_CYCLE_OP_PCALL_REG"], False, True, True, True, 0, c
    )
    assert memory_shaped_pcall["unsupported"]
    assert not memory_shaped_pcall["valid"]
    assert not memory_shaped_pcall["few_cycle"]

    bad_signature = budget(
        True, c["POLY_CYCLE_OP_PCALL_REG"], True, False, True, True, 0, c
    )
    assert bad_signature["blocked"] and not bad_signature["valid"]

    stack_not_ready = budget(
        True, c["POLY_CYCLE_OP_RETURN_COOKIE"], True, True, False, True, 0, c
    )
    assert stack_not_ready["blocked"] and not stack_not_ready["valid"]

    ret_cookie = budget(
        True, c["POLY_CYCLE_OP_RETURN_COOKIE"], True, True, True, True, 0, c
    )
    assert ret_cookie["valid"] and ret_cookie["fixed"] == 3
    assert ret_cookie["few_cycle"]

    trap_fast_memory = budget(
        True, c["POLY_CYCLE_OP_TRAP_PACKET"], True, True, True, True, 1, c
    )
    assert trap_fast_memory["valid"]
    assert trap_fast_memory["fixed"] == 2
    assert trap_fast_memory["variable"] == 1
    assert trap_fast_memory["total"] == 3
    assert trap_fast_memory["waits_for_memory"]
    assert not trap_fast_memory["few_cycle"]

    trap_slow_memory = budget(
        True, c["POLY_CYCLE_OP_TRAP_PACKET"], True, True, True, True, 17, c
    )
    assert trap_slow_memory["valid"]
    assert trap_slow_memory["total"] == 19
    assert trap_slow_memory["waits_for_memory"]

    packet_not_ready = budget(
        True, c["POLY_CYCLE_OP_TRAP_PACKET"], True, True, True, False, 0, c
    )
    assert packet_not_ready["blocked"] and not packet_not_ready["valid"]

    invalid = budget(True, 7, True, True, True, True, 0, c)
    assert invalid["invalid"] and not invalid["valid"]
    assert invalid["fixed"] == 0 and invalid["total"] == 0

    idle = budget(
        False, c["POLY_CYCLE_OP_PSWITCH"], True, True, True, True, 0, c
    )
    assert not idle["valid"] and not idle["invalid"]

    print("POLY_RTL_TRANSITION_CYCLE_BUDGET_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
