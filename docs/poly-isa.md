# Poly ISA

Poly is a multi-frontend CPU extension. x86_64 remains the system ISA for boot,
privilege, paging, interrupts, faults, atomics, and the effective TSO memory
model. AArch64 and RISC-V64 are user-mode frontends that fetch real 32-bit
instructions from the same x86_64 virtual address space.

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

## What Changes From x86_64

- x86_64 owns all privileged state; foreign frontends are user-mode only.
- Foreign code uses the same addresses, page permissions, stack memory, and TSO
  ordering as x86_64 code.
- Foreign fetch is direct native fetch. There is no per-instruction `#UD`
  envelope.
- Mode switches use decoded Poly control operations, not exception side effects.
- Compatibility targets ordinary precompiled SysV x86_64, AAPCS64, and RISC-V
  psABI objects, not a new compiler-only ABI.
- Fast cross-ISA calls are register-only. Stack arguments, aggregates,
  variadics, lazy binding, and complex ABI reshaping stay in software thunks.

## Control Operations

- `PENTER frontend`: enter a frontend from runtime/system code.
- `PSWITCH frontend,target`: switch frontend and branch without a return.
- `PCALL frontend,target,sig`: switch frontend, branch, and apply a cached
  register ABI signature slot.
- `PTRAPRET`: resume after a precise Poly trap.

Prototype encodings are temporary: x86_64 uses `0f 3a fc <subop>`, AArch64
uses a reserved `HINT` subspace, RISC-V uses `custom-0`, and Poly architectural
state is exposed as XSAVE component `20`.

## ABI Boundary

`PCALL` is intentionally small: it changes frontend, target PC, return state,
and optionally applies a register-only ABI signature. Hardware must not parse
user-memory call descriptors or repack stack layouts.

Signature slots can map native ABI register lanes, for example SysV
`RDI,RSI,RDX,RCX,R8,R9` to AAPCS64 `x0..x5` or RISC-V `a0..a5`. The first
eight native foreign ABI argument registers are preserved in Poly trap/import
packets, so runtimes can handle full AAPCS64 `x0..x7` and RISC-V `a0..a7`
boundaries.

## State And Traps

Foreign-only registers, ABI signature slots, trap packets, transition-stack
state, and monitor controls are explicit XSAVE-style architectural state. The
prototype's explicit state import layout version is `3`.

Foreign `svc`/`ecall`, breakpoints, illegal instructions, unresolved imports,
and recoverable faults produce OS-neutral trap packets. The CPU reports facts;
runtime or OS policy decides whether to translate syscalls, bind symbols, call
helpers, deliver signals, or terminate.

Detailed rationale and future hardware direction live in
`docs/poly-isa-design-directions.md`.
