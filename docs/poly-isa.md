# Poly ISA

Poly is an x86_64 user-mode extension for running AArch64 and RISC-V64
instructions in the same process and virtual address space.

## Contract

- x86_64 remains the system ISA for privilege, paging, interrupts, VM control,
  atomics, and TSO ordering.
- AArch64 and RISC-V64 fetch native instructions from x86_64 virtual memory.
- Compatibility targets real SysV x86_64, AAPCS64, and RISC-V psABI objects.
- Non-x86 state is explicit per-thread XSAVE-style architectural state.
- Recoverable exits produce OS-neutral trap packets for a user runtime.
- Hardware does not implement OS, libc, linker, libgcc, or libatomic policy.

## Differences From x64

- One frontend is active: x86_64, AArch64, or RISC-V64.
- x86_64 fetch is variable length; AArch64 is 4-byte aligned; RISC-V is 2-byte
  aligned with RVC.
- `PCALL` switches frontend; ABI signature slots handle register-only calls.
- Software thunks handle stack args, aggregates, variadics, lazy binding, and
  incompatible vectors.
- Native returns cross ISAs through reserved return cookies and a hardware
  transition stack.
- Foreign syscalls trap to the user runtime; the CPU does not translate Linux
  calls.
- Trap packets preserve the first eight native foreign ABI argument registers.

## Control Ops

- `PENTER mode`: enter frontend `0` x86_64, `1` AArch64, or `2` RISC-V64.
- `PSWITCH mode,target`: branch to another frontend.
- `PCALL mode,target,sig`: cross-ISA call using ABI signature slot `sig`.
- `PTRAPRET`: resume from the user trap monitor.
- `PLANDING`: mark or validate indirect cross-ISA landing points.

Prototype encodings are real decoded controls, not `#UD` envelopes:
x86_64 `0f 3a fc <subop>`, AArch64 reserved `HINT`, RISC-V `custom-0`.

Use `README.md` for commands and `docs/poly-isa-design-directions.md` for
design rationale.
