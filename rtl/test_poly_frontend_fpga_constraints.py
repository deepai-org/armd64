#!/usr/bin/env python3
"""Static checks for FPGA timing constraints on poly_frontend_fpga_top."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
RTL = ROOT / "rtl/poly_frontend_fpga_top.sv"
XDC = ROOT / "rtl/poly_frontend_fpga_top.xdc"
MAKEFILE = ROOT / "Makefile"


def parse_ports(text: str) -> tuple[set[str], set[str]]:
    header = text.split(");", 1)[0]
    inputs: set[str] = set()
    outputs: set[str] = set()
    pattern = re.compile(
        r"^\s*(input|output)\s+logic(?:\s+\[[^\]]+\])?\s+([A-Za-z_][A-Za-z0-9_]*)",
        re.MULTILINE,
    )
    for direction, name in pattern.findall(header):
        if direction == "input":
            inputs.add(name)
        else:
            outputs.add(name)
    return inputs, outputs


def extract_float(pattern: str, text: str, name: str) -> float:
    match = re.search(pattern, text)
    if not match:
        raise AssertionError(f"missing {name}")
    return float(match.group(1))


def main() -> int:
    rtl = RTL.read_text()
    xdc = XDC.read_text()
    makefile = MAKEFILE.read_text()
    inputs, outputs = parse_ports(rtl)

    assert "clk_i" in inputs, "missing FPGA top clock input"
    assert "rst_ni" in inputs, "missing FPGA top reset input"
    assert outputs, "FPGA top must expose output ports"
    assert "poly_frontend_fpga_top" in xdc

    period = extract_float(
        r"create_clock\s+-name\s+poly_core_clk\s+-period\s+([0-9.]+)\s+\[get_ports clk_i\]",
        xdc,
        "poly_core_clk create_clock",
    )
    uncertainty = extract_float(
        r"set_clock_uncertainty\s+([0-9.]+)\s+\[get_clocks poly_core_clk\]",
        xdc,
        "clock uncertainty",
    )
    input_delay = extract_float(
        r"set_input_delay\s+-clock\s+poly_core_clk\s+([0-9.]+)\s+\$poly_data_inputs",
        xdc,
        "input delay",
    )
    output_delay = extract_float(
        r"set_output_delay\s+-clock\s+poly_core_clk\s+([0-9.]+)\s+\[all_outputs\]",
        xdc,
        "output delay",
    )

    assert 0.0 < uncertainty < period
    assert 0.0 <= input_delay < period
    assert 0.0 <= output_delay < period
    assert input_delay + output_delay + uncertainty < period

    required_fragments = [
        "set_false_path -from [get_ports rst_ni]",
        "set poly_data_inputs [remove_from_collection [all_inputs] [get_ports {clk_i rst_ni}]]",
        "set_input_delay -clock poly_core_clk",
        "set_output_delay -clock poly_core_clk",
        "[all_outputs]",
    ]
    for fragment in required_fragments:
        if fragment not in xdc:
            raise AssertionError(f"missing XDC fragment: {fragment}")

    forbidden_fragments = [
        "set_false_path -from [all_inputs]",
        "set_false_path -to [all_outputs]",
        "set_clock_groups -asynchronous",
        "set_disable_timing",
    ]
    for fragment in forbidden_fragments:
        if fragment in xdc:
            raise AssertionError(f"over-broad timing exception: {fragment}")

    assert "POLY_RTL_XDC ?= rtl/poly_frontend_fpga_top.xdc" in makefile
    assert "POLY_RTL_FPGA_OUT ?= out/rtl" in makefile
    assert "POLY_RTL_FPGA_MANIFEST = $(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).manifest" in makefile
    assert "check-poly-rtl-constraints" in makefile
    assert "check-poly-rtl-fpga-artifacts" in makefile
    assert "poly-rtl-fpga-artifacts: check-poly-rtl-constraints" in makefile
    assert "python3 rtl/test_poly_frontend_fpga_artifacts.py" in makefile
    assert "timing_closure=not_run" in makefile
    assert "python3 rtl/test_poly_frontend_fpga_constraints.py" in makefile
    assert "check-poly-rtl-hdl: check-poly-rtl-constraints" in makefile

    print("POLY_RTL_FPGA_CONSTRAINTS_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
