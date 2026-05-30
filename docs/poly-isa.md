# Poly ISA Quick Reference

Poly extends x86_64 with AArch64 and RISC-V64 user-mode frontends so existing
precompiled code from all three ISAs can run in one virtual address space.
x86_64 remains the system ISA: boot, privilege, paging, interrupts, faults,
atomics, VM control, and global TSO memory ordering stay x86-owned.

## Run

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Faster focused smoke test:

```bash
make boot-poly-binfmt-arch-traps
```

## Architectural Delta From x86_64

- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Foreign frontends fetch native aligned 32-bit instructions from the same
  linear address space; `RIP` is the shared frontend PC.
- Poly transitions are decoded control instructions, not `#UD` envelopes.
- Hardware only performs fixed-latency frontend switches, register aliasing, and
  precise trap/return bookkeeping.
- Hardware does not implement Linux, libc, dynamic-linker policy, user-memory
  call descriptors, stack repacking, or aggregate marshalling.
- Loader/runtime thunks handle stack arguments, by-value aggregates, variadics,
  lazy binding, syscalls, libcalls, and incompatible vector layouts.
- Register-only calls can use cached ABI signature slots for few-cycle native
  ABI handoff.
- Cross-ISA returns use native return instructions plus a hardware transition
  stack/return-cookie mechanism; same-ISA returns stay ordinary returns.

## Encodings

Prototype x86 control page:

```text
0f 3a fc <subop>
```

Current x86 subops: `PENTER=0x03`, `PSWITCH=0x04`, `PLANDING=0x05`,
`PCALL_SIG=0x2d`, `PCALL_SIG_IMM=0x2e`, `LANDING_POLICY_SET=0x6d`, and
`LANDING_POLICY_GET=0x6e`.

Foreign control instructions use native reserved spaces:

| Frontend | Encoding |
| --- | --- |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Key foreign subops: escape to x86, trap return, switch mode, call mode,
call-with-signature, landing pad, and set/get ABI signature. See
`tools/include/polycpuid.h` for the exact numeric assignments.

## State And Traps

- Poly state is XSAVE-style architectural state. The current explicit state import layout version is `8`.
- Foreign traps produce OS-neutral packets. A trap packet records the first eight native foreign ABI argument registers so a monitor or runtime can apply policy without the CPU knowing the host OS or C library.
- The OS saves/restores the Poly XSAVE component; userspace policy lives in the
  Poly runtime/monitor.

## References

- Design rationale: `docs/poly-isa-design-directions.md`
- Constants and userspace helper ABI: `tools/include/polycpuid.h`
- Prototype Bochs implementation: `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`
