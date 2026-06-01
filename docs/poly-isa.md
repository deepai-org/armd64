# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine. Code
shares the x86_64 virtual address space and keeps each ISA's native ABI; the
goal is running existing compiled objects and cross-ISA shared libraries.

## Run

```sh
make image
make boot-poly-focused-validation
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `make boot-poly-full-real-xsave-arch-traps` for the broad regression gate.

## ISA Contract

- Frontends: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.
- x86_64 owns boot, paging, privilege, interrupts, syscalls, atomics, and the
  memory model.
- AArch64 and RISC-V64 execute as ring-3 frontends over x86_64 memory.
- Switching uses explicit control opcodes, not one `#UD` envelope per foreign
  instruction.
- Foreign register state is per-thread XSAVE-style architectural state.
- Cross-ISA calls preserve native ABIs. Register-only calls use ABI signature
  slots; stack arguments, aggregates, variadics, lazy binding, libc policy, and
  syscall policy stay in runtime software.
- Foreign traps produce OS-neutral trap packets for a runtime or OS handler.

## x86 Control Opcodes

- `PENTER` `0f 3a fc 03`: enter frontend in `R15`, TLS/state key in `R13`.
- `PSWITCH` `0f 3a fc 04`: switch to `RBX` target in frontend `R15`.
- `PLANDING` `0f 3a fc 05`: indirect-call landing marker.
- `PCALL` `0f 3a fc 2d`: call `RBX`, return `R11`, signature `R12`.
- `PCALL_IMM` `0f 3a fc 30..`: call using an immediate signature slot.
- `PTRAPRET` `0f 3a fc 62`: resume from a trap packet.

AArch64 controls use reserved HINT encodings. RISC-V controls use custom-0
encodings. Design rationale lives in `docs/poly-isa-design-directions.md`.
