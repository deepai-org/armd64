# Poly ISA

## Run

```bash
make image
make boot-poly-binfmt-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYEXEC_RESULT|FAIL|Kernel panic|Oops' out/serial.log
```

Other targets: `make boot-poly-arch-traps`, `make boot-poly-neutral-arch-traps`,
`make boot-poly-call-arch-traps`, `make boot-poly-full-arch-traps`.

## What Changes From x86_64

Poly lets one x86_64 user process run existing x86_64, AArch64, and RISC-V64
code in one virtual address space. x86_64 remains the system ISA for boot,
privilege, paging, interrupts, faults, atomics, virtual memory, and TSO memory
ordering.

AArch64 and RISC-V64 are user-mode frontends. They fetch normal aligned 32-bit
instructions from the shared `RIP` stream and enter/exit through decoded Poly
control instructions, not per-instruction `#UD` envelopes.

Non-x86 state is explicit XSAVE-style architectural state. Syscalls,
breakpoints, illegal instructions, unresolved imports, and policy exits produce
user/runtime trap packets so hardware stays OS-neutral. ABI-simple calls can use
register alias/signature slots; stack/aggregate/variadic calls use software
thunks.

## Control And Encodings

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64. Operations: `PENTER`,
`PSWITCH`, `PCALL`, `PTRAPRET`, `PLANDING`.

- CPUID base leaf `0x40000000`; XSAVE component `20`; state layout `8`
- x86_64: `0f 3a fc <subop>`
- AArch64: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64: `0x0000700b | ((subop & 0x7f) << 25)`

Full design notes: `docs/poly-isa-design-directions.md`.
