# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 system ISA.
The goal is to link and run existing precompiled objects from all three ISAs in
one process while keeping OS, libc, loader, and ABI policy in software.

## Run

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Use `make boot-poly-binfmt-arch-traps` for the binfmt-style loader path and
`make boot-poly-full-real-xsave-arch-traps` when validating OS XSAVE state.

## x86_64 Differences

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 owns boot, paging, privilege, interrupts, faults, atomics, VM control,
  and TSO memory ordering.
- Foreign frontends fetch aligned native 32-bit instructions from the same
  linear address space; the shared frontend PC is `RIP`.
- Transitions are decoded control instructions, not `#UD` envelopes.
- Register-only cross-ISA calls use cached ABI signature slots so hardware can
  remap argument registers in rename/dispatch without moving data.
- Stack arguments, aggregates, variadics, syscalls, libcalls, lazy binding, and
  incompatible vector layouts are software thunk/runtime work.
- Cross-ISA returns use native return instructions plus a hardware transition
  stack and return cookie. Same-ISA returns stay ordinary returns.

## Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover frontend switch/call, ABI-signature call, landing pad, trap return,
and ABI signature set/get. Numeric assignments live in
`tools/include/polycpuid.h`.

## State And Traps

- Poly register state is XSAVE-style architectural state. Current explicit
  state import layout version: `9`.
- Foreign traps produce OS-neutral packets with PC, frontend, trap cause, and
  the first eight native ABI argument registers.
- The OS saves/restores architectural state. Userspace runtime/monitor code owns
  syscall translation, libcalls, lazy binding, and ABI compatibility policy.

## Details

- Design rationale: `docs/poly-isa-design-directions.md`
- Public constants: `tools/include/polycpuid.h`
- Bochs prototype: `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`
