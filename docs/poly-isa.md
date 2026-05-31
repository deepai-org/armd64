# Poly ISA

Reference contract for running precompiled x86_64, AArch64, and RISC-V64 code in one x86_64 process. Rationale and open design tradeoffs are in [poly-isa-design-directions.md](poly-isa-design-directions.md).

## Architectural Model

- x86_64 is the system ISA. Privilege, paging, interrupts, hard faults, VM control, atomics, syscalls, and TSO stay x86-owned.
- AArch64 and RISC-V64 are user-mode frontends sharing the same virtual address space and fetching real native instructions from `RIP`/`PC`.
- Mode switches are decoded control transfers, not `#UD` envelopes or exception paths.
- Extra foreign state is per-thread XSAVE-style architectural state, not CR3-scoped hidden emulator state.
- Recoverable foreign exits produce Ring 3 trap packets. CPU hardware does not implement Linux, libc, libgcc, or syscall policy.

## Control Transfers

Frontend IDs: `0` x86_64, `1` AArch64, `2` RISC-V64.

| Op | Contract |
| --- | --- |
| `PENTER f` | Enter frontend `f` at the current PC. |
| `PSWITCH f, target` | Tail-branch to `target` in frontend `f`. |
| `PCALL f, target, sig` | Cross-call to `target` using ABI signature slot `sig`. |
| `PTRAPRET` | Resume from a Ring 3 trap packet. |
| `PLANDING` | Validate an indirect cross-ISA landing pad. |

## Calls And ABI

- Fast `PCALL` only aliases registers. It must not parse descriptors, read user memory, repack stacks, or translate syscalls.
- ABI signature slots describe native argument/result register mappings and are intended to compile into rename/RAT updates in hardware.
- Baseline exchange window: `RAX,RDX,RCX,RDI,RSI,R8,R9,R10` = `x0..x7` = `a0..a7`.
- Cross-ISA returns use a hardware transition stack and reserved return cookies.
- Software thunks handle stack arguments, aggregates, variadics, lazy binding, syscall/libcall policy, and incompatible vector ABIs.

## Temporary Bochs Encodings

These are prototype encodings, not final silicon opcode allocations.

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | (subop << 5)` in reserved `HINT` space |
| RISC-V64 | `0x0000700b | (subop << 25)` in `custom-0` space |
