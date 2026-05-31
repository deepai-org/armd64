# Poly ISA

Poly is a Bochs prototype of one x86_64 machine that can also execute existing
AArch64 and RISC-V64 userspace code in the same virtual address space.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
rg -a 'BOOT_OK|POLYBINFMT_OK|POLYTHREAD_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## x64 Changes

- x86_64 remains the system ISA for privilege, paging, faults, interrupts,
  atomics, VM control, and TSO memory ordering.
- Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.
- Foreign frontends fetch native instructions directly from `RIP`; the fast
  path does not use per-instruction `#UD` envelopes.
- Foreign register state is explicit XSAVE-style per-thread state; the state
  import layout version is `8`.
- Hardware is limited to frontend switching, register alias signatures, return
  cookies, transition-stack state, and trap packets.
- Software owns dynamic linking, syscall/libcall policy, stack arguments,
  aggregates, variadics, incompatible vectors, and loader/runtime thunks.

## Controls

- `PENTER frontend`: enter a frontend from trusted runtime code.
- `PSWITCH frontend, target`: branch to another frontend without return.
- `PCALL frontend, target, sig`: call another frontend using ABI signature
  slot `sig`.
- `PTRAPRET`: resume from a precise Poly trap.
- `PLANDING`: mark or validate indirect cross-frontend targets.

Same-ISA returns use normal native returns. Cross-frontend returns use normal
native returns to hardware return cookies.

## Prototype Encodings

- x86_64: decoded `0f 3a fc <subop>` Poly control page.
- AArch64: reserved HINT subspace.
- RISC-V64: custom-0 opcode family.

Rationale and future hardware direction:
[poly-isa-design-directions.md](poly-isa-design-directions.md).
