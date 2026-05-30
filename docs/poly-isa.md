# Poly ISA Quick Reference

Poly lets x86_64, AArch64, and RISC-V64 user code run in one x86_64 process
address space. The compatibility target is existing native objects and shared
libraries, not a new compiler-only ISA. See
`docs/poly-isa-design-directions.md` for the longer hardware/ABI rationale.

## Running

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Focused checks:

- `make boot`: plain x86_64 VM
- `make boot-poly-arch-traps`: raw AArch64/RISC-V execution and traps
- `make boot-poly-neutral-arch-traps`: direct AArch64<->RISC-V switching
- `make boot-poly-call-arch-traps`: calls, threads, and signals
- `make boot-poly-full-arch-traps`: broad regression run

## How It Differs From x86_64

- x86_64 remains the system ISA for boot, privilege, paging, faults, interrupts,
  atomics, virtual memory, and TSO ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch normal 32-bit
  instructions from the same virtual address space.
- Cross-ISA transfer uses decoded control instructions, not one `#UD` envelope
  per foreign instruction.
- Extra foreign register state is explicit XSAVE-style architectural state, not
  hidden CR3-scoped emulator state.
- Foreign syscalls, breakpoints, illegal instructions, unresolved imports, and
  policy exits become precise Poly trap packets for user/runtime software.
- Hardware does not implement Linux, libc, the dynamic linker, or memory-parsing
  ABI descriptors.

## Control Instructions

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64. The control
surface is `PENTER`, `PSWITCH`, `PCALL`, `PTRAPRET`, and `PLANDING`.

`PCALL` uses a hardware transition stack plus a native return cookie. ABI
signature slots may remap register names for register-only calls; memory and
ABI-complex cases stay in software.

## Prototype Encodings

Temporary Bochs encodings, not final silicon allocations:

- CPUID base leaf `0x40000000`; XSAVE component `20`; state layout `8`
- x86_64 control page: `0f 3a fc <subop>`
- AArch64 control page: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control page: `0x0000700b | ((subop & 0x7f) << 25)`
