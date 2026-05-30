# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to x86_64. The goal is to
link and run existing precompiled objects from all three ISAs in one x86_64
process without making the CPU or OS understand libc, Linux ABIs, or loader
policy.

## Run Tests

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

```bash
make boot-poly-binfmt-arch-traps
```

## x86_64 Differences

- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 remains the system ISA: boot, paging, privilege, interrupts, faults,
  atomics, VM control, and TSO memory ordering are x86-owned.
- Foreign frontends fetch aligned native 32-bit instructions from the same
  linear address space. `RIP` is the shared frontend PC.
- Poly transitions are decoded control instructions, not `#UD` envelopes.
- Hardware may switch frontends, alias registers, track cross-ISA returns, and
  report precise traps. It must not parse user-memory call descriptors, repack
  stacks, marshal aggregates, or implement OS/libc policy.
- Runtime thunks handle stack arguments, aggregates, variadics, lazy binding,
  syscalls, libcalls, and incompatible vector layouts.
- Register-only calls use cached ABI signature slots for few-cycle handoff.
- Cross-ISA returns use native return instructions plus a hardware transition
  stack/return cookie. Same-ISA returns remain ordinary returns.

## Encodings

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Main subops include enter/switch frontend, call frontend, call with ABI
signature, landing pad, trap return, and ABI signature set/get. Numeric
assignments live in `tools/include/polycpuid.h`.

## State And Traps

- Poly register state is XSAVE-style architectural state. Current explicit
  state import layout version: `9`.
- Foreign traps produce OS-neutral packets. Packets include the first eight
  native foreign ABI argument registers so userspace can translate syscalls,
  libcalls, and lazy binding without CPU OS knowledge.
- The OS saves/restores Poly state; userspace runtime/monitor owns policy.

## References

- Rationale: `docs/poly-isa-design-directions.md`
- Constants: `tools/include/polycpuid.h`
- Bochs prototype: `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`
