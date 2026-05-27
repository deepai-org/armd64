# Bochs Polyglot CPU Harness

This repo boots x86_64 Linux under a modified Bochs and tests a prototype
polyglot CPU extension. x86_64 remains the boot ISA, kernel ISA, and default
userspace ISA. With `POLY_ENABLED=1`, selected x86 instructions enter raw
AArch64/RISC-V execution or call precompiled foreign functions.

The goal is compatibility with existing compiled code and libraries, not a new
compiler-only ABI.

## Run

```bash
# Build the container image. Re-run after changing bochs-prepoly-src/.
make image

# Baseline x86_64 boot with the poly feature hidden.
make boot

# Main poly gates.
make boot-poly
make boot-poly-arch-traps
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps

# Remove generated images/logs/staging files.
make clean
```

Other focused targets exist in the `Makefile`, but the commands above are the
normal gates to run while changing the ISA/runtime.

Important outputs:

- `out/serial.log`: guest serial log and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.

## ISA Differences

Normal x86_64 execution is unchanged. The extension is an opt-in facility
discovered through private CPUID leaves beginning at `0x40000000`.

New operations:

- `PENTER.A64` / `PENTER.RV64`: switch from x86_64 decode to raw foreign
  instruction fetch.
- `PEXIT`: switch back to x86_64.
- `PCALL.A64.SYSV` / `PCALL.RV64.SYSV`: call native-ABI foreign functions from
  an x86_64 SysV caller.

Bochs currently encodes these with an experimental x86 byte family:

```text
0f 24 <op> 50 4f 4c 59 21
```

The trailing bytes spell `POLY!`. Real hardware should use dedicated
non-exception opcodes.

Foreign execution is direct-fetch, not one x86 envelope per foreign
instruction. Native escapes:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch to AArch64.

Cross-ISA calls target ordinary native ABIs:

- x86_64 SysV integer and FP arguments are translated to AAPCS64 or RISC-V
  psABI argument registers.
- Stack arguments use the target ABI stack rules.
- Foreign `ret` returns through a native return cookie to x86_64.
- Integer results return in `RAX`; scalar FP results return in `XMM0`.
- The tested AArch64 path also supports fixed 128-bit SIMD through
  `XMM0`/`XMM1` and `v0`/`v1`.

Foreign traps are architectural exits, not hidden Linux or libc emulation. The
CPU records the trap, switches back to x86_64, and lets the runtime or OS decide
whether to translate, signal, thunk, emulate, or reject.

Foreign memory accesses use the same guest virtual-memory path as x86_64. The
prototype defines foreign memory ordering as x86 TSO.

## Current Limits

- AArch64 and RISC-V instruction coverage is still a tested subset.
- `polycall` is not a complete Linux dynamic linker.
- Foreign register state is still Bochs-managed prototype state, not a real
  XCR0/XSAVE OS component.
- Equal-speed execution is a design goal, not a demonstrated result.

See `docs/poly-isa.md` for the detailed ISA and CPUID contract.
