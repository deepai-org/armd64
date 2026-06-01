# Poly ISA

Poly adds AArch64 and RISC-V64 user-mode frontends to an x86_64 machine so
existing compiled objects and cross-ISA shared libraries can share one virtual
address space.

## Run It

```sh
make image
make boot-poly-focused-validation
make boot-poly-full-real-xsave-arch-traps
rg -a 'POLY.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

## Model

- x86_64 owns boot, privilege, paging, interrupts, exceptions, syscalls,
  atomics, and memory ordering.
- AArch64/RISC-V64 are extra user-mode fetch/decode frontends over that same
  address space.
- Mode switches use explicit opcodes, never per-instruction `#UD` envelopes.
- Cross-ISA calls preserve native ABIs. ABI signature slots accelerate
  register-only calls; stack args, aggregates, variadics, lazy binding, libc,
  and syscall policy stay in software/runtime code.
- Foreign register state is per-thread XSAVE-style state. Foreign traps produce
  OS-neutral trap packets for a runtime or OS handler.

## x86 Controls

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

- `PENTER` `0f 3a fc 03`: `R15=frontend`, `R13=TLS`
- `PSWITCH` `0f 3a fc 04`: `R15=frontend`, `RBX=target`, `R13=TLS`
- `PLANDING` `0f 3a fc 05`: indirect-call landing marker
- `PCALL` `0f 3a fc 2d`: `R15=frontend`, `RBX=target`, `R11=return`, `R12=sig`
- `PCALL_IMM` `0f 3a fc 30..`: `sig=op - 0x30`
- `PTRAPRET` `0f 3a fc 62`: resume from trap packet

Foreign controls use reserved AArch64 HINT and RISC-V custom-0 encodings. See
`docs/poly-isa-design-directions.md` for rationale and hardware direction.
