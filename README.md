# Bochs Polyglot CPU Harness

Boots x86_64 Linux in a modified Bochs and tests a prototype CPU extension for
running existing AArch64 and RISC-V code from x86_64 userspace.

The goal is precompiled-code compatibility and fast cross-ISA interop. This is
not a new compiler-only ABI.

## Run

```bash
# Build the Docker/Bochs image. Re-run after bochs-prepoly-src/ changes.
make image

# Boot x86_64 Linux with the poly feature hidden.
make boot

# Main real boot tests.
make boot-poly
make boot-poly-arch-traps
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps

# Remove generated images, logs, and staging files.
make clean
```

Primary outputs:

- `out/serial.log`: guest serial log and pass/fail markers.
- `out/bochs.log`: Bochs device/CPU log.
- `out/bochs-boot.iso`: generated boot ISO.

Useful success markers include `POLY_PROBE_OK`, `POLYAPP_OK`, `POLYEXEC_OK`,
`POLYCALL_OK`, `POLYTHREAD_OK`, `POLYSIGNAL_OK`, and `POLYBINFMT_OK`.

## ISA Differences From x86_64

x86_64 remains the boot ISA, kernel ISA, and default userspace ISA. Existing
x86_64 code should behave normally unless it opts into the polyglot extension.

| Area | Difference |
| --- | --- |
| Discovery | Private CPUID leaves starting at `0x40000000`. |
| Opcode | Prototype uses `0f 24 <op> 50 4f 4c 59 21` (`POLY!`). Real hardware should use dedicated non-exception opcodes. |
| Frontend | x86_64 uses normal variable-length decode. Foreign modes use raw 32-bit AArch64 or RISC-V fetch from the shared program counter. |
| Entry | Poly opcodes switch the frontend or perform `PCALL` into native foreign functions. |
| ABI | `PCALL` bridges x86_64 SysV callers to native AAPCS64 or RISC-V psABI callees. It is not a custom PolyFast ABI. |
| Return | Native foreign returns use a return cookie to resume the x86_64 caller. |
| Traps/syscalls | Foreign traps, syscalls, illegal instructions, and breakpoints exit as architectural records for x86_64-side runtime or OS policy. |
| Memory | Foreign code uses the same guest virtual memory path as x86_64. The prototype defines foreign memory ordering as x86 TSO. |
| State | Bochs currently stores extra foreign state internally. A hardware design should expose it through CPUID/XCR0/XSAVE-like OS state. |

Native foreign escape instructions:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch to AArch64.

Current limits:

- AArch64 and RISC-V instruction coverage is still a tested subset.
- `polycall` is a compatibility loader/runtime, not a complete Linux dynamic linker.
- Equal-speed execution is a design goal, not a measured result.

Detailed architecture: `docs/poly-isa.md`.
