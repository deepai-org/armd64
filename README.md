# Bochs Polyglot CPU Harness

This repo boots x86_64 Linux under a modified Bochs and tests a prototype
polyglot CPU extension. x86_64 remains the boot ISA, kernel ISA, and default
userspace ISA. When enabled, the CPU can enter raw AArch64 or RISC-V execution
and can call precompiled foreign-ISA functions from x86_64 code.

The goal is compatibility with existing compiled code and libraries, not a new
compiler-only ABI.

## How To Run

```bash
# Build the Docker image. Re-run after changing bochs-prepoly-src/.
make image

# Baseline boot with the poly feature hidden.
make boot

# Main poly test gates.
make boot-poly
make boot-poly-arch-traps
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps

# Clean generated images, logs, and staging files.
make clean
```

Useful outputs:

- `out/serial.log`: guest serial log and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.

## How This ISA Differs From x86_64

Plain x86_64 behavior is unchanged. Polyglot execution is opt-in and discovered
through private CPUID leaves starting at `0x40000000`.

The prototype adds these architectural ideas:

- Frontend switches from x86_64 decode into raw fixed-width AArch64 or RISC-V
  fetch.
- Direct foreign fetch replaces the old idea of wrapping every foreign
  instruction in an x86 `#UD` envelope.
- `PCALL` bridges an x86_64 SysV caller to native AAPCS64 or RISC-V psABI
  callees.
- Native foreign return instructions return through a cookie back to x86_64.
- Foreign traps, syscalls, illegal instructions, and breakpoints exit back to
  x86_64 as architectural trap records; the runtime or OS decides policy.
- Foreign memory accesses use the same guest virtual-memory path as x86_64.
- The prototype defines foreign memory ordering as x86 TSO.

Current Bochs opcode placeholder:

```text
0f 24 <op> 50 4f 4c 59 21
```

The trailing bytes spell `POLY!`. Real hardware should use dedicated
non-exception opcodes rather than this experimental encoding.

Native foreign exits:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch to AArch64.

## Current Limits

- AArch64 and RISC-V instruction coverage is still a tested subset.
- `polycall` is not a complete Linux dynamic linker.
- Foreign register state is still Bochs-managed prototype state, not a real
  XCR0/XSAVE OS component.
- Equal-speed execution is a design goal, not a demonstrated result.

See `docs/poly-isa.md` for the detailed ISA and CPUID contract.
