# Poly ISA

Bochs prototype quick reference. Poly targets existing x86_64, AArch64, and
RISC-V64 user-mode objects in one x86_64 process, not a new compiler-only ISA.
For rationale, see `docs/poly-isa-design-directions.md`.

## Running

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Other useful targets: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-neutral-arch-traps`, `make boot-poly-call-arch-traps`,
`make boot-poly-full-arch-traps`.

## Difference From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults, interrupts,
  atomics, virtual memory, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends over the same virtual address
  space, fetching normal fixed-width 32-bit instructions from `RIP`.
- Cross-ISA transfer uses decoded Poly control instructions, not one x86 `#UD`
  envelope per foreign instruction.
- Non-x86 register state is explicit XSAVE-style state, not hidden CR3-scoped
  emulator state.
- Foreign syscalls, breakpoints, illegal instructions, unresolved imports, and
  policy exits produce precise user/runtime trap packets.
- Hardware does not implement Linux, libc, dynamic linking, or memory-parsing
  ABI descriptors. Software handles ABI-complex cases.

## Control Instructions

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64. Core operations are
`PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`.

`PCALL` uses a hardware transition stack and native return cookie. ABI signature
slots may remap register names for register-only calls. Stack arguments,
aggregates, variadics, and other ABI-complex cases stay in software.

## Prototype Encodings

These are temporary Bochs encodings, not final silicon allocations:

- CPUID base leaf `0x40000000`; XSAVE component `20`; state layout `8`
- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`
