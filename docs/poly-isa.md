# Poly ISA

Poly keeps x86_64 as the system ISA and adds direct-fetch AArch64 and RISC-V64
user frontends in the same virtual address space. The target is compatibility
with existing precompiled code and cross-ISA shared libraries, not a new
compiler-only ABI.

## Run It

```sh
make image
make boot-poly-binfmt-arch-traps
make boot-poly-full-arch-traps
rg -n "POLY.*(OK|FAIL)|NATIVE_CHECK|Kernel panic|Segmentation fault|BUG:" \
  out/serial.log out/bochs*.log
```

Useful shorter targets: `make boot`, `make boot-poly-arch-traps`,
`make boot-poly-call-arch-traps`.

## Difference From x86_64

| Area | Poly behavior |
| --- | --- |
| System ISA | x86_64 still owns boot, paging, privilege, interrupts, faults, syscalls, and TSO memory ordering. |
| User frontends | Frontend IDs are `0=x86_64`, `1=AArch64`, `2=RISC-V64`. |
| Fetch | AArch64 fetches aligned 4-byte instructions from `RIP`; RISC-V64 fetches 2-byte or 4-byte instructions from `RIP`. |
| Memory | All frontends use the same x86_64 virtual addresses, TLBs, page permissions, and faults. |
| Switching | `PENTER`, `PSWITCH`, `PCALL`, and `PIRET` are decoded control-flow operations, not `#UD` envelopes. |
| State | Non-x86 state is explicit XSAVE-style architectural state, currently component `20` in the prototype. |
| ABI | Hardware accelerates register-only handoff; software thunks handle stack args, aggregates, variadics, PLT/GOT, lazy binding, and incompatible vectors. |

## Prototype Encodings

- x86_64 controls: `0f 3a fc <subop>`
- AArch64 controls: reserved `HINT`
- RISC-V64 controls: `custom-0`

## References

- Design rationale: `docs/poly-isa-design-directions.md`
- Public constants: `tools/include/polycpuid.h`
