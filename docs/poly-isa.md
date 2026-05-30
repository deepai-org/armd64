# Poly ISA Quick Reference

Poly is a Bochs prototype for running precompiled x86_64, AArch64, and
RISC-V64 userspace code in one x86_64 virtual address space.

## Test

```bash
make image
make boot-poly-full-arch-traps
rg -a 'BOOT_OK|POLY.*OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Useful variants:

- `make boot-poly-binfmt-arch-traps`: loader/binfmt path.
- `make boot-poly-full-real-xsave-arch-traps`: OS XSAVE/XRSTOR path.

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, faults,
  interrupts, VM control, atomics, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode frontends that fetch native instructions
  from the same linear address space. AArch64 fetch is 4-byte aligned; RISC-V64
  fetch is 2-byte aligned so RVC is legal.
- Fast transitions are decoded control instructions, not `#UD` envelopes.
- Register-only cross-ISA calls use cached ABI signature slots. Complex ABI
  cases stay in loader/runtime thunks.
- Cross-ISA returns use native return instructions, a transition stack, and a
  return cookie. Same-ISA returns are unchanged.
- Foreign traps produce OS-neutral trap packets; syscall/libcall policy belongs
  to userspace runtime or OS code, not the frontend switch instruction.

## Temporary Encodings

| Frontend | Poly control encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover frontend switch/call, ABI-signature calls, landing pads, trap
return, and ABI signature set/get. Numeric assignments live in
[polycpuid.h](../tools/include/polycpuid.h).

## References

- Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md)
- Public constants: [polycpuid.h](../tools/include/polycpuid.h)
- Bochs prototype: [proc_ctrl.cc](../bochs-prepoly-src/bochs/cpu/proc_ctrl.cc)
