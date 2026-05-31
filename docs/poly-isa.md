# Poly ISA

Poly is an x86_64 CPU extension that lets user-mode x86_64, AArch64, and
RISC-V64 code run in one process address space. x86_64 remains the system ISA;
the foreign ISAs are additional user-mode frontends.

Detailed rationale is in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Difference From x86_64

- x86_64 still owns boot, privilege, paging, interrupts, syscalls, atomics, and
  the memory model.
- AArch64 and RISC-V64 fetch native 32-bit instructions directly from `RIP`.
- All frontends share one x86 virtual address space and page tables.
- Foreign modes inherit x86 TSO.
- Extra foreign registers are architectural XSAVE-style state, not hidden
  emulator state.
- Fast-path interop is register remapping plus a frontend switch. Stack
  reshaping, aggregate marshalling, variadics, lazy binding, and libc policy are
  runtime work.

## Frontends

Frontend IDs are `0` for x86_64, `1` for AArch64, `2` for RISC-V64, and
`3..255` reserved.

## Control Instructions

Prototype encodings live in `tools/include/polycpuid.h`. Hardware should use
real decoded opcodes, not `#UD` traps, for fast frontend changes.

- `PENTER frontend`: enter a frontend from trusted runtime/system code.
- `PSWITCH frontend, target`: tail-branch to another frontend.
- `PCALL frontend, target, sig`: cross-frontend call using ABI signature slot
  `sig`.
- `PTRAPRET`: resume after a precise Poly trap.
- `PLANDING`: validate an indirect cross-frontend target when enabled.

## ABI And Traps

Poly targets existing precompiled code, not a new source ABI. Register-only calls
can use cached ABI signature slots; complex calls use software thunks.

Native return instructions stay valid. Cross-ISA returns use a hardware
transition stack plus reserved return cookies.

Foreign syscalls, breakpoints, illegal instructions, unsupported instructions,
unresolved imports, and recoverable exits produce OS-neutral trap packets. A
Ring 3 Poly monitor may handle them; the kernel still owns hard page faults,
interrupts, scheduling, signals, and real syscalls issued by the monitor.

## Non-Goals

- Legacy single-instruction `#UD` envelopes.
- Hidden register banks keyed only by `CR3`.
- Hardware parsing of user-memory call descriptors.
- Hardware stack repacking, struct marshalling, libcalls, or Linux-specific
  syscall emulation.
