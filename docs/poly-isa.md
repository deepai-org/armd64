# Poly ISA Quick Reference

This is the short operational reference for the Bochs prototype. For build and
test details, see `README.md`. For the hardware/ABI rationale, see
`docs/poly-isa-design-directions.md`.

## Goal

Run precompiled x86_64, AArch64, and RISC-V64 userspace code in one virtual
address space, with native ABI compatibility as the priority. This is not a
new compiler-only ABI.

## Run

```bash
make image
make boot-poly-focused-validation
rg -a 'BOOT_OK|.*_OK|FAIL|Kernel panic|Oops' out/serial.log
```

Useful narrower targets:

- `make boot`: plain x86_64 VM sanity boot.
- `make boot-poly`: shorter Poly smoke/regression run.
- `make boot-poly-nativecheck-arch-traps`: native x86 plus Poly CPU controls.
- `make boot-poly-probe-arch-traps`: trap/control-plane probe coverage.
- `make boot-poly-call-real-xsave-arch-traps`: cross-ISA calls with real XSAVE
  state enabled.
- `make boot-poly-full-real-xsave-arch-traps`: broad silicon-facing regression.

Use `BOOT_TIMEOUT_SECONDS=900 make <target>` for long runs.

## How It Differs From x86_64

- x86_64 remains the system ISA: boot, privilege, paging, faults, interrupts,
  VM control, atomics, syscalls, and global TSO memory ordering.
- AArch64 and RISC-V64 are peer user-mode decode frontends fetched from the
  same address space.
- Frontend switches are decoded control instructions, not per-instruction
  `#UD` envelopes.
- AArch64 uses fixed 32-bit fetch. RISC-V64 supports 16/32-bit fetch,
  including RVC.
- Cross-ISA calls target real native ABIs: x86_64 SysV, AArch64 AAPCS64, and
  RISC-V psABI.
- Register-only calls may use ABI signature slots and hardware-style register
  aliasing. Stack arguments, aggregates, variadics, lazy binding, libc policy,
  and syscall policy stay in software.
- Foreign architectural state is per-thread XSAVE-style state. Hidden Bochs
  banks are prototype fallback machinery, not the hardware contract.
- Foreign `svc`, `ecall`, breakpoints, illegal instructions, and frontend
  faults produce OS-neutral trap packets for a runtime or OS handler.
- AArch64 and RISC-V64 can switch/call each other directly; they do not need to
  bounce through x86_64.

## Temporary Control Encodings

These are prototype encodings chosen to decode as normal instructions in Bochs.
They model dedicated hardware controls and are not the final silicon opcode
allocation.

- x86_64 control prefix: `0f 3a fc <subop>`
- AArch64 control form: `0xd503201f | ((subop & 0x7f) << 5)`
- RISC-V64 control form: `0x0000700b | ((subop & 0x7f) << 25)`

Core subops:

- `0x03`: `PENTER`
- `0x04`: `PSWITCH`
- `0x05`: `PLANDING`
- `0x2d`: `PCALL`
- `0x30..0x3c`: `PCALL_SLOT`
- `0x62`: `PTRAPRET`
- `0x65..0x6e`: setup/query controls

## Compatibility Boundary

Fast hardware support covers frontend switching, direct raw fetch/decode,
register exchange windows, ABI signature slots, trap-packet delivery, and
XSAVE-visible foreign state.

Software still owns anything that requires memory interpretation: stack layout,
large or split aggregates, variadic calls, dynamic linker thunks, libc behavior,
and syscall-number/policy translation.
