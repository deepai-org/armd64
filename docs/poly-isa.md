# Poly ISA

Poly lets existing x86_64, AArch64, and RISC-V64 userspace code run in one
x86_64 virtual address space. x86_64 remains the privileged/control ISA.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- x86_64 still owns privilege, paging, interrupts, faults, VM control, atomics,
  and TSO memory ordering.
- AArch64 and RISC-V64 are additional user-mode frontends, not coprocessors.
- Foreign frontends fetch native instructions directly from `RIP`; there is no
  per-instruction `#UD` envelope.
- AArch64 fetch is 4-byte aligned. RISC-V fetch supports 2-byte RVC alignment.
- Foreign architectural registers are XSAVE-style process/thread state.

## Poly Controls

- `PENTER frontend`
- `PSWITCH frontend, target`
- `PCALL frontend, target, sig`
- `PTRAPRET`
- `PLANDING`

Same-ISA returns stay native. Cross-frontend returns use ordinary native return
instructions plus transition-stack cookies.

## Hardware Boundary

Hardware handles frontend switching, call/return cookies, trap packets, XSAVE
state, and fixed-latency register-only ABI remapping.

Software handles syscalls, libcalls, linking, stack arguments, aggregates,
variadics, incompatible vectors, and memory-shaped ABI translation.

## Prototype Encodings

- x86_64: `0f 3a fc <subop>`
- AArch64: reserved HINT subspace
- RISC-V: custom-0 opcode family

Full rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
