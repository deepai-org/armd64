# Poly ISA Quick Reference

Bochs prototype for running existing precompiled x86_64, AArch64, and RISC-V64
user code in one x86_64 virtual address space. Design rationale lives in
[`poly-isa-design-directions.md`](poly-isa-design-directions.md). The frozen v1
contract lives in [`poly-isa-v1.md`](poly-isa-v1.md).

```bash
make image
make check-poly-contracts
```

## Contract

- x86_64 is still the system ISA: boot, privilege, paging, interrupts, faults,
  atomics, syscalls, VM control, and TSO memory ordering.
- AArch64 and RISC-V64 are user-mode decode frontends in the same address space.
- ISA switches are decoded control instructions, not `#UD` traps.
- AArch64 uses 32-bit aligned fetch. RISC-V64 supports 16/32-bit fetch.
- `PCALL` switches ISA and can apply cached native-ABI register aliases.
- Foreign state is an explicit 8KB user-owned spill image; an unmodified OS is
  not required to save a custom Poly xstate component.
- Recoverable foreign traps write OS-neutral trap records.
- Foreign syscall numbers and import selectors are opaque trap fields; hardware
  does not translate them into OS calls or libc/helper functions.
- Import trap selectors are opaque CPU values; named libc/libgcc bindings are
  runtime policy, not CPUID/ISA features.
- Software handles stack arguments, aggregates, variadics, lazy binding,
  syscalls, libcalls, and debugger policy.

## Control Opcode Allocations

| Frontend | Encoding |
| --- | --- |
| x86_64 | `0f 3a fc <subop>` |
| AArch64 | `0xd503201f | ((subop & 0x7f) << 5)` |
| RISC-V64 | `0x0000700b | ((subop & 0x7f) << 25)` |

Subops cover `PENTER`, `PSWITCH`, `PCALL`, signature-slot calls, setup/query,
`PLANDING`, `PTRAPRET`, `PSET_SPILL_PTR`, and `PRESTORE`. ABI signature setup
writes a register-only signature slot as `kind | (register_map << 32)`. Query
returns the same encoded value, so runtimes and hardware can verify the exact
RAT/register-map policy instead of inferring it from the kind alone.

These are fixed-latency decoded control operations: they do not read user
descriptors, repack stacks, or enter exception delivery for normal frontend
switching. The x86_64 byte family is the Bochs allocation for this prototype;
silicon/FPGA implementations can allocate a different dedicated opcode family,
but must expose its geometry through Poly CPUID so runtimes do not infer it
from documentation.

CPUID `POLY_CPUID_BASE + 2, subleaf 32` reports the active x86 byte geometry.
Subleaf 33 reports the opcode contract flags: the current Bochs encoding is a
vendor/prototype dedicated decode path, not a `#UD` trap envelope, and software
must discover it through CPUID instead of hard-coding it as a production x86
allocation.

## FPGA/Silicon ISA Readiness Boundary

The ISA contract is ready for FPGA or silicon implementation when hardware can
implement the architecture below without inheriting emulator or runtime policy:

- x86_64 remains the system ISA for boot, privilege, paging, interrupts,
  faults, atomics, syscalls, VM control, and global TSO memory ordering.
- AArch64 and RISC-V64 are user-mode native fetch frontends in the same virtual
  address space, using their native instruction-width and alignment rules.
- Poly control transfers are fixed-latency decoded control operations, not
  `#UD` trap envelopes or software descriptor calls.
- The active x86 control opcode family is CPUID-discovered. The current
  `0f 3a fc <subop>` allocation is a vendor/prototype dedicated decode page,
  not a production x86 allocation; silicon may reassign it behind the same
  discovery contract.
- `PCALL` fast paths use register-only ABI signature slots. Hardware may apply
  those slots in rename/RAT logic; stack arguments, aggregates, variadics,
  lazy binding, syscall translation, libcalls, debugger policy, and helper
  imports remain userspace runtime policy.
- Foreign architectural state is an explicit 8KB user-owned spill/import image
  with fixed offsets, size, alignment, and feature leaves. The OS is not in the
  Poly state-management loop.
- `PSET_SPILL_PTR` registers the per-thread spill image and x86 monitor
  trampoline. On raw-mode interrupt or fault, hardware writes the 8KB image,
  records the Poly PC and spill reason, switches back to x86, and makes the OS
  see the trampoline RIP. `PRESTORE` imports the image before `PENTER` resumes
  raw Poly code.
- Recoverable exits publish OS-neutral trap packets before monitor-vector
  redirect. Failed packet writes or invalid packet addresses prevent the
  redirect and report precise faults.
- Native returns use ordinary frontend return instructions and a hardware
  transition-stack return cookie; same-ISA returns remain normal.
- Invalid frontend IDs, non-canonical targets, frontend alignment violations,
  invalid ABI slots, transition-stack overflow/underflow, and trap-restore
  conflicts are rejected before mutating architectural frontend/PC state.
- Foreign barriers/fences are explicit x86-TSO no-ops, and foreign memory
  operations cannot expose weak reordering in the shared x86 address space.

Final opcode ownership, reserved-bit policy, CPUID leaves, spill-image layout,
trap packet format, error precedence, zero-kernel OS contract, and conformance
evidence are frozen in `docs/poly-isa-v1.md`. This boundary deliberately says nothing about
FPGA fabric, Verilog structure, or timing closure. Those are implementation and
productization tasks. The ISA readiness requirement is that the hardware
contract is explicit, discoverable, fixed-latency where required, and free of
OS/libc/runtime policy.
