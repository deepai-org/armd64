#!/usr/bin/env python3
"""Validate generated FPGA handoff artifacts for poly_frontend_fpga_top."""

from pathlib import Path
import hashlib


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "out/rtl"
MANIFEST = OUT / "poly_frontend_fpga_top.manifest"
YOSYS_LOG = OUT / "poly_frontend_fpga_top.yosys.log"


def read_manifest(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for line in path.read_text().splitlines():
        if not line.strip():
            continue
        key, sep, value = line.partition("=")
        if sep != "=" or not key or not value:
            raise AssertionError(f"malformed manifest line: {line!r}")
        if key in data:
            raise AssertionError(f"duplicate manifest key: {key}")
        data[key] = value
    return data


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def yosys_resource(log: str, label: str) -> str:
    value = None
    for line in log.splitlines():
        if label in line:
            value = line.split()[-1]
    if value is None:
        raise AssertionError(f"missing Yosys resource label: {label}")
    return value


def main() -> int:
    manifest = read_manifest(MANIFEST)
    required = {
        "top",
        "edif",
        "xdc",
        "cells",
        "estimated_lcs",
        "edif_sha256",
        "xdc_sha256",
        "timing_closure",
    }
    if set(manifest) != required:
        raise AssertionError(f"unexpected manifest keys: {sorted(manifest)}")

    assert manifest["top"] == "poly_frontend_fpga_top"
    assert manifest["timing_closure"] == "not_run"

    edif = ROOT / manifest["edif"]
    xdc = ROOT / manifest["xdc"]
    assert edif.is_file() and edif.stat().st_size > 0
    assert xdc.is_file() and xdc.stat().st_size > 0
    assert manifest["edif_sha256"] == sha256(edif)
    assert manifest["xdc_sha256"] == sha256(xdc)

    xdc_text = xdc.read_text()
    assert "create_clock -name poly_core_clk" in xdc_text
    assert "set_false_path -from [get_ports rst_ni]" in xdc_text

    edif_prefix = edif.read_text(errors="ignore")[:4096]
    assert "poly_frontend_fpga_top" in edif_prefix
    assert "(edif" in edif_prefix

    log = YOSYS_LOG.read_text()
    assert manifest["cells"] == yosys_resource(log, "Number of cells:")
    assert manifest["estimated_lcs"] == yosys_resource(log, "Estimated number of LCs:")

    print("POLY_RTL_FPGA_ARTIFACTS_OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
