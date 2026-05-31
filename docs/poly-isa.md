# Poly ISA

Compact contract for running precompiled x86_64, AArch64, and RISC-V64 code in one x86_64 process. Design rationale lives in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Model

x86_64 is the system ISA: privilege, paging, interrupts, hard faults, syscalls, VM control, atomics, and TSO remain x86-owned.

AArch64 and RISC-V64 are user-mode frontends over the same virtual address space. They fetch real native instructions from `RIP`/`PC`.

Mode changes are decoded control-transfer instructions, not `#UD` envelopes. Extra architectural state is per-thread XSAVE-style state. Recoverable foreign exits are Ring 3 trap packets.

## Controls

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Op | Meaning |
| --- | --- |
| `PENTER f` | Enter frontend `f` at the current PC. |
| `PSWITCH f, target` | Tail-branch to `target` in frontend `f`. |
| `PCALL f, target, sig` | Cross-call using ABI signature slot `sig`. |
| `PTRAPRET` | Resume after a Ring 3 trap packet. |
| `PLANDING` | Validate an indirect cross-ISA landing pad. |

## Calls

Fast `PCALL` aliases registers only; it does not parse descriptors, touch user memory, repack stacks, or translate syscalls. Signature slots describe native ABI argument/result mappings and can be applied as hardware rename/RAT mappings.

Baseline exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.

Cross-ISA returns use a hardware transition stack plus reserved return cookies. Software thunks handle stack arguments, aggregates, variadics, lazy binding, syscall/libcall policy, and incompatible vector ABIs.

## Prototype Encodings

Temporary Bochs encodings; final silicon opcode allocation is still open.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | (subop << 5)` using reserved `HINT` space |
| RISC-V64 | `0x0000700b | (subop << 25)` using `custom-0` space |
