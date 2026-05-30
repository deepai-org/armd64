# Poly ISA

Poly is an x86_64 extension that lets user code execute AArch64 and RISC-V64
basic blocks in the same virtual address space. x86_64 remains the system ISA:
boot, privilege, paging, interrupts, atomics, scheduling, syscalls, and memory
ordering are still x86-owned.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Smaller targets: `boot-poly`, `boot-poly-call-arch-traps`,
`boot-poly-binfmt-arch-traps`, and `boot-poly-neutral-arch-traps`.

## How It Differs From x86_64

- Frontends: `0` x86_64, `1` AArch64, `2` RISC-V64.
- Fetch: x86_64 uses normal variable-length decode. AArch64 fetches aligned
  32-bit words from `RIP`. RISC-V64 fetches native 16-bit or 32-bit instructions
  from `RIP`.
- Memory: all frontends share the x86_64 virtual address space, page tables, page
  permissions, and TSO ordering.
- State: non-x86 architectural state is explicit XSAVE-style state, not hidden
  emulator state.
- Calls: cross-frontend calls use register-only ABI signature slots. Software
  thunks handle stack arguments, aggregates, variadics, lazy binding, and policy.
- Traps: foreign syscalls, breakpoints, unsupported instructions, and import
  misses report OS-neutral trap packets. The ISA does not implement Linux, libc,
  libgcc, libatomic, or dynamic-linker behavior.

## Prototype Control Ops

These Bochs encodings are temporary and should become real vendor-allocated
opcodes before hardware work.

| Op | x86_64 | AArch64 | RISC-V64 |
| --- | --- | --- | --- |
| Base | `0f 3a fc <subop>` | `0xd503201f | (subop << 5)` | `0x0000700b | (subop << 25)` |
| `PENTER` | `0x03` | n/a | n/a |
| `PSWITCH` | `0x04` | `0x78` | `8` |
| `PCALL` | `0x2d` | `0x7a` | `10` |
| `PCALL_SIG_IMM` | `0x2e <slot>` | `0x60 + slot` | `16 + slot` |
| `PTRAPRET` | `0x62` | `0x76` | `6` |
| `PLANDING` | `0x05` | `0x7b` | `11` |

Long-form rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
