# Poly ISA

Quick reference for the x86_64 extension that lets one userspace address space
execute existing x86_64, AArch64, and RISC-V64 code. Rationale:
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-exec-cross-arch-traps
make boot-poly-full-real-xsave-arch-traps
```

## Contract

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, VM control, and the global TSO memory model.
- AArch64 and RISC-V64 are user-mode decode frontends over the same memory.
- Fetch is frontend-specific: x86_64 variable-length, AArch64 32-bit aligned,
  and RISC-V64 16-bit RVC plus 32-bit.
- Cross-frontend control uses real decoded Poly instructions, not `#UD`.
- Non-x86 state is explicit per-thread XSAVE-style architectural state.
- Fast calls may use hardware register-alias signature slots.
- Stack arguments, aggregates, variadics, syscall translation, and loader
  policy stay in software.

## Frontends

- `0`: x86_64, variable-length fetch
- `1`: AArch64, 32-bit aligned fetch
- `2`: RISC-V64, 16-bit aligned RVC plus 32-bit fetch

## Prototype Control Encodings

Bochs/test encodings only; not vendor allocations.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | (subop << 5)` reserved HINT subspace |
| RISC-V64 | `0x0000700b | (subop << 25)` custom-0 family |

Constants are in `tools/include/polycpuid.h`.
