# Bochs Polyglot CPU Harness

This boots x86_64 Linux in a modified Bochs and tests a prototype CPU extension
for running precompiled AArch64 and RISC-V code from x86_64 userspace.

The project goal is compatibility with existing binaries and fast cross-ISA
interop, not a new compiler-only ABI.

## Requirements

- Docker with `linux/arm64` support.
- The checked-in `bochs-prepoly-src/` subdirectory; rebuild the image after
  changing it.

## Run

```bash
# Build the Docker/Bochs image.
make image

# Boot x86_64 Linux with the poly feature disabled.
make boot

# Main boot gates.
make boot-poly
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps

# Remove generated images, logs, and staging files.
make clean
```

Useful outputs:

- `out/serial.log`: guest console log and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.

Expected success markers include `POLY_PROBE_OK`, `POLYAPP_OK`,
`POLYCALL_OK`, `POLYTHREAD_OK`, `POLYSIGNAL_OK`, and `POLYBINFMT_OK`.

## ISA Differences From x86_64

x86_64 is still the boot ISA, kernel ISA, and default userspace ISA. Normal
x86_64 code runs unchanged unless it executes the polyglot extension.

- Discovery uses private CPUID leaves starting at `0x40000000`.
- The Bochs prototype uses `0f 24 <op> 50 4f 4c 59 21` (`POLY!`) opcodes.
- A hardware version should use dedicated non-exception opcodes, not `UD2`.
- Foreign execution swaps the frontend from x86 variable-length decode to raw
  AArch64 or RISC-V instruction fetch from the shared program counter.
- `PCALL` bridges x86_64 SysV callers into native AAPCS64 or RISC-V psABI
  callees. There is no custom "PolyFast" ABI in the compatibility path.
- Native foreign returns use a return cookie to resume the x86_64 caller.
- Foreign traps, syscalls, illegal instructions, and breakpoints exit as
  architectural records for x86_64-side runtime or OS policy.
- Foreign memory accesses use the same guest virtual memory path as x86_64; the
  prototype defines foreign memory ordering as x86 TSO.
- Extra foreign architectural state is explicit in the ISA design. Bochs stores
  it internally today; hardware should expose it through CPUID/XCR0/XSAVE-like
  OS state.

Native foreign escape instructions:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch to AArch64.

Current limits:

- AArch64 and RISC-V instruction coverage is still a tested subset.
- `polycall` is a compatibility loader/runtime, not a complete Linux dynamic
  linker.
- Equal-speed execution is a design goal, not a measured result.

Detailed architecture lives in `docs/poly-isa.md`.
