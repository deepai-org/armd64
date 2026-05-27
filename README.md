# Bochs Polyglot CPU Harness

Boots x86_64 Linux in modified Bochs and tests a prototype CPU extension for
running precompiled AArch64 and RISC-V code from x86_64 userspace.

The goal is real binary compatibility and fast cross-ISA interop, not a new
compiler-only ABI.

## Requirements

- Docker with `linux/arm64` support.
- The checked-in `bochs-prepoly-src/` tree.

## Run

```bash
make image                  # Rebuild after Bochs source changes.
make boot                   # Baseline x86_64 Linux boot.
make boot-poly              # Poly ISA smoke tests.
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps
make clean
```

Read results in:

- `out/serial.log`: guest console and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.

Expected markers include `POLY_PROBE_OK`, `POLYAPP_OK`, `POLYCALL_OK`,
`POLYTHREAD_OK`, `POLYSIGNAL_OK`, and `POLYBINFMT_OK`.

## ISA Delta From x86_64

x86_64 remains the boot ISA, kernel ISA, and default userspace ISA. Normal
x86_64 code is unchanged unless it executes the polyglot extension.

- Discovery: private CPUID leaves starting at `0x40000000`.
- Prototype opcodes: `0f 24 <op> 50 4f 4c 59 21` (`POLY!`).
- Hardware target: dedicated non-exception opcodes, not `UD2` envelopes.
- Execution: switch the frontend from x86 variable-length decode to raw
  32-bit AArch64 or RISC-V fetch from the shared program counter.
- Calls: `PCALL` bridges x86_64 SysV callers into native AAPCS64 or RISC-V
  psABI callees; no custom "PolyFast" ABI is used for compatibility.
- Returns: native foreign returns use a cookie to resume the x86_64 caller.
- Traps/syscalls: foreign traps, syscalls, illegal instructions, and
  breakpoints exit as architectural records for x86_64-side runtime or OS
  policy.
- Memory: foreign accesses use the same guest virtual memory path as x86_64;
  the prototype defines foreign memory ordering as x86 TSO.
- State: extra foreign architectural state is internal in Bochs today; hardware
  should expose it through CPUID/XCR0/XSAVE-like OS state.

Foreign escape instructions:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch to AArch64.

Current limits: AArch64/RISC-V instruction coverage is still a tested subset,
`polycall` is not a complete Linux dynamic linker, and equal-speed execution is
a design goal rather than a measured result.

Detailed architecture: `docs/poly-isa.md`.
