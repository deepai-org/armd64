# Poly ISA

Compact contract for running precompiled x86_64, AArch64, and RISC-V64 code in
one x86_64 process. Design rationale lives in
[poly-isa-design-directions.md](poly-isa-design-directions.md).

## Contract

- x86_64 remains the system ISA: privilege, paging, interrupts, hard faults,
  syscalls, VM control, atomics, and global TSO stay x86-owned.
- AArch64 and RISC-V64 are user-mode frontends that fetch real native
  instructions from the same virtual address space.
- Poly control transfers are decoded instructions, not `#UD` envelopes.
- Extra architectural state is per-thread XSAVE-style state.
- Recoverable foreign exits are delivered as Ring 3 trap packets.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Instruction | Meaning |
| --- | --- |
| `PENTER frontend` | Enter a frontend at the current PC. |
| `PSWITCH frontend, target` | Tail-branch across frontends. |
| `PCALL frontend, target, sig` | Cross-call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a Ring 3 trap packet. |
| `PLANDING` | Mark or validate an indirect cross-ISA landing pad. |

## ABI Boundary

- Fast `PCALL` only aliases registers; it never parses descriptors, reads user
  memory, or repacks stacks.
- Signature slots map native ABI argument/result registers, ideally through
  rename/RAT remapping.
- The null exchange window is `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` =
  `x0..x7` = `a0..a7`.
- Cross-ISA returns use a hardware transition stack plus reserved return
  cookies, so native `ret` instructions can return across frontends.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding,
  syscall/libcall policy, and incompatible vector ABIs.

## Temporary Encodings

Bochs prototype encodings only; final silicon opcode allocations are open.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | (subop << 5)` in reserved `HINT` space |
| RISC-V64 | `0x0000700b | (subop << 25)` in `custom-0` space |
