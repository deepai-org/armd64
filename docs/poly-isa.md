# Poly ISA

Poly is an x86_64 extension that adds user-mode AArch64 and RISC-V64
frontends in the same virtual address space. The target is compatibility with
existing native ABI code, not a new compiler-only ISA.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Contract

- Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.
- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, and global TSO memory ordering.
- AArch64 and RISC-V execute native instruction streams directly from `RIP`.
  AArch64 uses 4-byte fetch; RISC-V supports the normal 16/32-bit instruction
  stream. There are no per-instruction `#UD` envelopes.
- Foreign code shares the x86_64 virtual address space and page permissions.
- Foreign register state is explicit XSAVE-style architectural state. It is not
  hidden CR3-keyed emulator state.
- Hardware provides fixed-latency frontend switches, calls, returns, trap
  packets, and register-only ABI signature mapping.
- Software handles linking policy, syscalls, libcalls, stack arguments,
  by-value aggregates, variadics, lazy binding, and other memory-shaped ABI
  translation.

## Control Instructions

| Instruction | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from runtime/system code. |
| `PSWITCH frontend, target` | Tail-branch to another frontend. |
| `PCALL frontend, target, sig` | Call another frontend using ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a precise Poly trap packet. |
| `PLANDING` | Mark or validate an indirect cross-frontend landing pad. |

Cross-ISA calls return through ordinary native returns: x86_64 `ret`, AArch64
`ret`, and RISC-V `ret`. The hardware transition stack and return cookie handle
the frontend restore; same-ISA returns stay on the normal path.

## ABI And Traps

- Fast calls use small register-only ABI signature slots. A hardware
  implementation can apply them in rename/RAT logic without moving data through
  execution units.
- Complex ABI cases use loader/runtime thunks, then perform a normal `PCALL` or
  `PSWITCH`.
- Foreign `svc`, `ecall`, breakpoints, illegal instructions, unresolved imports,
  and recoverable exits produce OS-neutral trap packets. Runtime or OS policy
  decides how to service them.

## Prototype Encodings

The Bochs prototype currently models Poly controls as decoded instructions:

- x86_64: `0f 3a fc <subop>` Poly control opcode page.
- AArch64: reserved HINT subspace.
- RISC-V: custom-0 opcode family.

These are temporary decoded opcodes, not fast-path `#UD` traps. Real hardware
should allocate normal frontend opcodes.

Design rationale: [poly-isa-design-directions.md](poly-isa-design-directions.md).
