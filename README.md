# Bochs Polyglot CPU Boot Harness

This repo boots a small x86_64 Linux guest under a modified Bochs and tests a
prototype polyglot CPU extension. x86_64 remains the boot ISA, kernel ISA, and
default userspace ISA. When `POLY_ENABLED=1`, the Bochs fork exposes CPUID-gated
instructions for entering raw AArch64/RISC-V execution and for calling
precompiled foreign functions from x86_64 userspace.

The project goal is compatibility with real existing binaries and libraries,
not a new compiler-only ABI.

## Run

Build the Docker image:

```bash
make image
```

Run a baseline x86_64 boot with the poly feature hidden:

```bash
make boot
```

Run the main poly gates:

```bash
make boot-poly
make boot-poly-arch-traps
make boot-poly-call-arch-traps
make boot-poly-full-arch-traps
```

Useful additional focused gates:

```bash
make boot-poly-probe-arch-traps
make boot-poly-apps-arch-traps
make boot-poly-binfmt-arch-traps
make boot-poly-bench-arch-traps
```

Re-run `make image` after changing `bochs-prepoly-src/`; boot targets use the
Bochs binary baked into the Docker image.

Primary outputs:

- `out/serial.log`: guest serial output and pass/fail markers.
- `out/bochs.log`: Bochs CPU/device log.
- `out/bochs-boot.iso`: generated boot ISO.
- `tmp/`: generated initramfs and staging files.

Clean generated files:

```bash
make clean
```

## What Changes Versus x86_64

Normal x86_64 execution is unchanged. The extension adds a separate polyglot
facility discovered through private CPUID leaves:

- `PENTER.A64` switches fetch/decode from x86_64 to raw fixed-width AArch64.
- `PENTER.RV64` switches fetch/decode from x86_64 to raw RISC-V, including the
  tested compressed 16-bit subset.
- `PEXIT` switches raw foreign execution back to x86_64.
- `PCALL.A64.SYSV` calls an AArch64 AAPCS64 function from an x86_64 SysV
  caller.
- `PCALL.RV64.SYSV` calls a RISC-V psABI function from an x86_64 SysV caller.
- Foreign traps are reported as guest-visible architectural trap packets; the
  CPU does not hide Linux syscalls or libcalls inside the emulator.

The current Bochs prototype uses the placeholder x86 byte family
`0f 24 <op> 50 4f 4c 59 21` where the final bytes spell `POLY!`. A hardware or
FPGA implementation should use dedicated non-exception opcodes.

Raw foreign modes execute native instruction words directly. Native escape
instructions are:

- AArch64 `brk #0x7fff`: exit to x86_64.
- AArch64 `brk #0x7ffe`: switch directly to RISC-V.
- RISC-V custom-0 `0x0000000b`: exit to x86_64.
- RISC-V custom-1 `0x0000002b`: switch directly to AArch64.

Cross-ISA calls use native return state, so ordinary AArch64 `ret` and RISC-V
`ret` can cross the mode boundary when entered through the supported call path.

## ABI And State

The bridge targets ordinary native ABIs:

- x86_64 SysV integer arguments are translated to AArch64 AAPCS64 or RISC-V
  psABI argument registers.
- x86_64 stack arguments are presented using the target ABI stack rules.
- x86_64 `RAX` receives integer return values from AArch64 `x0` or RISC-V `a0`.
- x86_64 `XMM0`-`XMM7` bridge to AArch64 `v0`-`v7` and RISC-V `fa0`-`fa7` for
  scalar FP; the tested AArch64 path also preserves fixed 128-bit SIMD
  arguments and returns through `XMM0`/`XMM1` and `v0`/`v1`.
- Foreign memory operations use the same virtual-memory path as x86_64.
- The mixed memory model is defined as x86 TSO for this prototype.

Bochs currently manages non-aliased foreign register state internally. A real
hardware contract needs CPUID plus an OS-visible save/restore mechanism such as
an XCR0/XSAVE component.

## Key Files

- `bochs-prepoly-src/bochs/cpu/proc_ctrl.cc`: Bochs CPU-side implementation.
- `scripts/boot.sh`: guest image construction and Bochs launch script.
- `tools/polyprobe.c`: raw-mode and trap-vector probe.
- `tools/polyapp.c`: manifest-backed raw payload runner.
- `tools/polyexec.c`: direct foreign ELF runner.
- `tools/polycall.c`: foreign ELF loader/runtime using `PCALL`.
- `tools/polythread.c`: pthread foreign-state isolation test.
- `tools/polysignal.c`: interrupted raw-mode signal/resume test.
- `docs/poly-isa.md`: detailed ISA, CPUID, ABI, and hardware notes.

## Current Limits

- AArch64 and RISC-V instruction coverage is still a tested subset.
- Equal-speed execution is a design target, not a demonstrated result.
- The opcode family is a Bochs placeholder, not a finalized silicon encoding.
- Foreign state is not yet a real XSAVE/XCR0 component.
- `polycall` is not a complete Linux dynamic linker.
