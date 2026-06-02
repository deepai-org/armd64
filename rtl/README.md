# Poly RTL Bring-Up

This directory contains minimal hardware-facing artifacts. The current module is
not a full CPU; it is the first fixed-latency decode block needed by an FPGA or
silicon prototype.

## Files

- `poly_ctrl_decode.sv`: synthesizable SystemVerilog decoder for x86_64,
  AArch64, and RISC-V64 Poly control instructions.
- `test_poly_ctrl_decode.py`: static and behavioral consistency test against
  `tools/include/polycpuid.h`.

## Run

```bash
python3 rtl/test_poly_ctrl_decode.py
```

Expected output:

```text
POLY_RTL_CTRL_DECODE_OK
```
