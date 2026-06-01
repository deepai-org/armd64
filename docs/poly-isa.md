# Poly ISA

Poly lets one x86_64 process execute precompiled x86_64, AArch64, and RISC-V64
code in one virtual address space. It is not a new compiler-only ISA and does
not use per-instruction `#UD` envelopes.

## Run

```sh
make image
make boot-poly-full-real-xsave-arch-traps
make boot-poly-exec-cross-arch-traps
make boot-poly-call-real-xsave-arch-traps
rg -a 'POLY.*OK|POLYCALL_OK|FAIL|Kernel panic|Oops' out/serial.log
```

## x86_64 Differences

- x86_64 owns boot, privilege, paging, interrupts, VM control, atomics, and TSO.
- AArch64 and RISC-V64 are user-mode frontends over the same virtual memory.
- Foreign fetch is direct: AArch64 is 4-byte aligned; RISC-V is 2-byte aligned
  with RVC.
- Non-x86 registers are explicit per-thread XSAVE-style state.
- Recoverable exits produce precise trap packets.

## ISA Controls

Frontend IDs are `0` x86_64, `1` AArch64, and `2` RISC-V64.

| Op | Purpose |
| --- | --- |
| `PENTER frontend` | Enter a frontend from trusted code. |
| `PSWITCH frontend, target` | Cross-ISA branch without return. |
| `PCALL frontend, target, sig` | Cross-ISA call using ABI signature slot `sig`. |
| `PLANDING` | Validate an indirect landing target. |
| `PTRAPRET` | Resume from a Poly trap packet. |

Temporary encodings: x86 `0f 3a fc <subop>`, AArch64 reserved HINT space, and
RISC-V custom-0. Constants live in `tools/include/polycpuid.h`.

## ABI Boundary

- Native ABIs: x86_64 SysV, AArch64 AAPCS64, RISC-V psABI.
- Register-only calls use ABI signature slots for hardware alias/RAT remapping.
- Stack arguments, variadics, aggregate repacking, vector mismatches, lazy
  binding, and loader policy stay in software thunks.
- Cross-ISA calls return with native returns: x86_64 `ret`, AArch64 `ret x30`,
  and RISC-V `ret`/`jalr x0, ra, 0`.

## More Detail

- `docs/poly-isa-design-directions.md`
- `tools/include/polycpuid.h`
