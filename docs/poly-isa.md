# Poly ISA

Poly is an x86_64 extension that can fetch/decode AArch64 and RISC-V64
user-mode code in the same virtual address space. The goal is compatibility
with existing compiled objects and cross-ISA shared libraries, not a new ABI.

## Run

```sh
make image
make boot-poly-focused-validation
rg -a 'POLY.*OK|NATIVE.*OK|FAIL|Kernel panic|Oops' out/serial.log
```

Use `make boot-poly-full-real-xsave-arch-traps` as the broad regression gate;
it is intentionally stricter and may expose incomplete compatibility work.

## Differences From x86_64

- x86_64 still owns boot, paging, privilege, interrupts, syscalls, atomics, and
  the memory model.
- AArch64 and RISC-V64 are extra ring-3 frontends over x86_64 virtual memory.
- Mode switches use explicit opcodes, not per-instruction `#UD` envelopes.
- Foreign register state is per-thread XSAVE-style state.
- Cross-ISA calls keep native ABIs. Fast register-only cases use ABI signature
  slots; stack args, aggregates, variadics, lazy binding, libc, and syscall
  policy stay in software/runtime code.
- Foreign traps produce OS-neutral trap packets for a runtime or OS handler.

Frontend IDs: `0=x86_64`, `1=AArch64`, `2=RISC-V64`.

## x86 Controls

- `PENTER` `0f 3a fc 03`: enter frontend in `R15`, TLS/state key in `R13`
- `PSWITCH` `0f 3a fc 04`: switch to `RBX` target in frontend `R15`
- `PLANDING` `0f 3a fc 05`: indirect-call landing marker
- `PCALL` `0f 3a fc 2d`: call `RBX`, return `R11`, signature `R12`
- `PCALL_IMM` `0f 3a fc 30..`: same, with immediate signature slot
- `PTRAPRET` `0f 3a fc 62`: resume from a trap packet

Foreign controls use reserved AArch64 HINT and RISC-V custom-0 encodings. The
full rationale is in `docs/poly-isa-design-directions.md`.
