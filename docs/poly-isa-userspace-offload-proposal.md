# Poly ISA Userspace Offload Proposal

Status: proposal

This document proposes a small Poly ISA change set that lets hardware carry
more architectural bookkeeping while keeping all operating-system policy in
userspace. The goal is not to make hardware understand Linux, ELF, BPF,
io_uring, or POSIX signals. The goal is to publish precise, stable execution
facts and provide atomic state transforms so `polyexec` can stop acting like a
fragile hypervisor.

## Summary

Add a required Poly ISA v2 monitor-offload feature group:

```c
#define POLY_FEAT_V2_MEM_PROBE        (1ull << 0)
#define POLY_FEAT_V2_STATE_DERIVE     (1ull << 1)
#define POLY_FEAT_V2_EVENT_COMPLETE   (1ull << 2)
#define POLY_FEAT_V2_DUMP_STATE       (1ull << 3)
#define POLY_FEAT_V2_SHARED_FENCE     (1ull << 4)
```

The feature group contains five OS-neutral primitives:

1. `PMEM_PROBE_RANGE`: non-faulting address-range validation.
2. `PDERIVE_STATE`: atomic state derive/import for clone, signal, and return
   paths.
3. `PCOMPLETE_EVENT`: atomic result and resume metadata application.
4. `PDUMP_STATE`: dense register/debug-note export.
5. `PFENCE`: explicit shared-memory ordering between frontend code, monitor
   code, and host-visible memory.

These primitives do not allocate memory, install kernel filters, create
threads, write ELF files, evaluate BPF, or interpret Linux syscalls.

## Why This Belongs In The ISA

`polyexec` currently has to infer facts that hardware already knows:

- whether a frontend access can safely touch a guest range;
- whether a trapped instruction completed, should be retried, or should resume
  at the next frontend PC;
- which frontend register receives a syscall result;
- how to copy frontend architectural state into a child thread or signal frame;
- how to export a coherent register image after fatal failure;
- which shared-memory writes must become visible before a syscall or wakeup.

Those are architectural facts, not OS policy decisions. Hardware can expose
them without knowing whether userspace will use them to implement Linux,
Windows, a unikernel ABI, a debugger, or a test harness.

## Non-Goals

- No Linux syscall numbers in hardware.
- No BPF or seccomp evaluator in hardware.
- No ELF, DWARF, GDB, minidump, or core-file format in hardware.
- No io_uring-specific instruction.
- No POSIX signal semantics in hardware.
- No thread creation, process creation, or scheduler policy in hardware.
- No ABI bridge planner in hardware.

Userspace remains responsible for policy and file formats. Hardware only owns
precise state facts and atomic state movement.

## 1. `PMEM_PROBE_RANGE`

Purpose: replace `/proc/self/maps` parsing, speculative prefaulting, and
intentional fault-driven probing with a non-faulting architectural query.

### Operation

```c
RAX = guest_va_start
RDX = byte_length
RCX = flags
PCTRL PMEM_PROBE_RANGE

RAX = status
RDX = first_failed_guest_va
RCX = probe_metadata
```

### Flags

```c
#define POLY_MEM_READ        (1u << 0)
#define POLY_MEM_WRITE       (1u << 1)
#define POLY_MEM_EXEC        (1u << 2)
#define POLY_MEM_FRONTEND    (1u << 3)
#define POLY_MEM_MONITOR     (1u << 4)
#define POLY_MEM_ATOMIC      (1u << 5)
```

`POLY_MEM_FRONTEND` probes the range as the active guest frontend would see it.
`POLY_MEM_MONITOR` probes the range as the monitor would see it. Exactly one
view flag must be set.

### Status Codes

```c
#define POLY_PROBE_OK              0
#define POLY_PROBE_UNMAPPED        1
#define POLY_PROBE_PERMISSION      2
#define POLY_PROBE_NONCANONICAL    3
#define POLY_PROBE_OVERFLOW        4
#define POLY_PROBE_UNSUPPORTED     5
```

### Required Guarantees

- The instruction never raises a guest memory fault for the probed range.
- The instruction does not mark pages accessed or dirty.
- The instruction does not allocate, fault in, or populate pages.
- On failure, `RDX` contains the first failing guest virtual address when the
  implementation can identify one. Otherwise it returns `guest_va_start`.
- Permission checks use the same frontend state that would be used by an actual
  load, store, or execute.

### Userspace Impact

`polyexec` can replace slow and racy range checks such as
`poly_guest_range_is_mapped()` and prefault walkers with a single architectural
query. Prefault avoidance comes from trusting precise event frames on real
faults instead of touching memory early to discover what might happen.

## 2. `PDERIVE_STATE`

Purpose: copy one frontend architectural state into another and apply a small,
validated set of register edits atomically.

This is the hardware primitive behind clone-child setup, signal delivery,
sigreturn-like restore, and monitor-to-frontend resume. It does not create a
thread and does not know what `clone(2)` or a POSIX signal is.

### Operation

```c
RAX = dst_state_ptr
RDX = src_state_ptr
RCX = derive_desc_ptr
R8  = derive_desc_bytes
PCTRL PDERIVE_STATE

RAX = status
RDX = applied_mask
RCX = dst_generation
```

### Descriptor

```c
#define POLY_DERIVE_MAGIC 0x44525632594c4f50ull /* "POLYV2DR" */

struct poly_v2_state_derive_desc {
    uint64_t magic;
    uint32_t size;
    uint16_t version;
    uint16_t frontend;
    uint64_t flags;

    uint64_t new_pc;
    uint64_t new_sp;
    uint64_t new_tls;
    uint64_t new_state_key;

    uint64_t result_reg0_value;
    uint64_t result_reg1_value;
    uint32_t result_reg0_index;
    uint32_t result_reg1_index;

    uint64_t clear_event_mask;
    uint64_t import_state_mask;
    uint64_t reserved[8];
};
```

### Flags

```c
#define POLY_DERIVE_SET_PC          (1ull << 0)
#define POLY_DERIVE_SET_SP          (1ull << 1)
#define POLY_DERIVE_SET_TLS         (1ull << 2)
#define POLY_DERIVE_SET_STATE_KEY   (1ull << 3)
#define POLY_DERIVE_SET_RESULT0     (1ull << 4)
#define POLY_DERIVE_SET_RESULT1     (1ull << 5)
#define POLY_DERIVE_CLEAR_EVENT     (1ull << 6)
#define POLY_DERIVE_IMPORT_GPR      (1ull << 7)
#define POLY_DERIVE_IMPORT_FP       (1ull << 8)
#define POLY_DERIVE_IMPORT_STATUS   (1ull << 9)
```

### Required Guarantees

- The destination state is never partially updated. Success is all-or-nothing.
- The implementation validates descriptor size, version, frontend, alignment,
  and reserved-zero fields.
- Unsupported register indexes fail with `POLY_STATE_BAD_REGISTER`.
- The operation cannot change host privilege, host page tables, native signal
  masks, or kernel thread identity.
- On success, the destination state receives a new generation value.

### Userspace Impact

`polyexec` can stop manually copying and patching frontend state across clone,
signal, and return paths. Userspace still builds ABI-specific signal frames and
calls host thread primitives, but the architectural frontend state movement is
owned by hardware.

## 3. `PCOMPLETE_EVENT`

Purpose: let the monitor finish an architectural event by applying result
registers, resume PC, event-clearing, and completion metadata in one validated
operation.

This is intentionally separate from syscall policy. Userspace decides the
result. Hardware applies that result according to the frontend state layout.

### Operation

```c
RAX = state_ptr
RDX = event_frame_ptr
RCX = complete_desc_ptr
R8  = complete_desc_bytes
PCTRL PCOMPLETE_EVENT

RAX = status
RDX = applied_mask
RCX = new_state_generation
```

### Descriptor

```c
#define POLY_COMPLETE_MAGIC 0x434d5032594c4f50ull /* "POLYV2MP" */

struct poly_v2_event_complete_desc {
    uint64_t magic;
    uint32_t size;
    uint16_t version;
    uint16_t frontend;
    uint64_t flags;

    uint64_t resume_pc;
    uint64_t result0;
    uint64_t result1;
    uint64_t errno_value;

    uint64_t event_generation;
    uint64_t clear_event_mask;
    uint64_t reserved[8];
};
```

### Flags

```c
#define POLY_COMPLETE_SET_RESUME_PC     (1ull << 0)
#define POLY_COMPLETE_USE_NEXT_PC       (1ull << 1)
#define POLY_COMPLETE_RETRY_PC          (1ull << 2)
#define POLY_COMPLETE_SET_RESULT0       (1ull << 3)
#define POLY_COMPLETE_SET_RESULT1       (1ull << 4)
#define POLY_COMPLETE_SET_ERRNO_STYLE   (1ull << 5)
#define POLY_COMPLETE_CLEAR_EVENT       (1ull << 6)
#define POLY_COMPLETE_REQUIRE_GENERATION (1ull << 7)
```

`POLY_COMPLETE_REQUIRE_GENERATION` makes the operation fail if the event frame
generation does not match `event_generation`. This prevents stale monitor
decisions from being applied to a newer trap.

### Required Guarantees

- Result registers and resume metadata are applied atomically.
- `USE_NEXT_PC` and `RETRY_PC` use hardware-provided event-frame fields, not
  monitor-guessed instruction lengths.
- The operation fails if the event frame and state frontend do not match.
- The operation does not issue syscalls and does not decide whether an error is
  Linux `errno`, Windows `NTSTATUS`, or any other OS concept.

### Userspace Impact

`polyexec` can replace ad hoc return-register mutation, next-PC calculation,
and event sanitization in trap return paths with one operation. Seccomp and
syscall translation stay in userspace; hardware only applies the result after
userspace has chosen it.

## 4. `PDUMP_STATE`

Purpose: export a dense, versioned, frontend-neutral register/debug blob that
userspace can wrap in any debugger or crash-reporting format.

### Operation

```c
RAX = state_ptr
RDX = output_buffer
RCX = output_buffer_bytes
R8  = selector_mask
PCTRL PDUMP_STATE

RAX = status
RDX = bytes_written
RCX = bytes_required
```

### Selector Mask

```c
#define POLY_DUMP_GPR          (1ull << 0)
#define POLY_DUMP_FP           (1ull << 1)
#define POLY_DUMP_STATUS       (1ull << 2)
#define POLY_DUMP_EVENT        (1ull << 3)
#define POLY_DUMP_TLS          (1ull << 4)
#define POLY_DUMP_DEBUG        (1ull << 5)
#define POLY_DUMP_LAYOUT_ONLY  (1ull << 63)
```

### Required Guarantees

- Output starts with a size-tagged header containing magic, version, frontend,
  endian, state generation, and record count.
- Each record is size-tagged and 16-byte aligned.
- The blob is not ELF, DWARF, minidump, or a Linux note.
- If the buffer is too small, `RCX` reports the required size without partial
  state exposure beyond the header.
- Dumping a state does not mutate that state.

### Userspace Impact

Core dump support becomes a format-wrapping problem. `polyexec` can ask
hardware for a coherent AArch64 or RISC-V register image, then emit ELF notes,
minidump streams, JSON test artifacts, or debugger-specific records in
userspace.

## 5. `PFENCE`

Purpose: make shared-memory ring and monitor handoff ordering explicit without
teaching hardware about io_uring or any other API.

### Operation

```c
RAX = fence_scope
RDX = flags
PCTRL PFENCE

RAX = status
```

### Scopes

```c
#define POLY_FENCE_FRONTEND_RELEASE      1
#define POLY_FENCE_FRONTEND_ACQUIRE      2
#define POLY_FENCE_MONITOR_RELEASE       3
#define POLY_FENCE_MONITOR_ACQUIRE       4
#define POLY_FENCE_FRONTEND_MONITOR_FULL 5
#define POLY_FENCE_HOST_SHARED_RELEASE   6
#define POLY_FENCE_HOST_SHARED_ACQUIRE   7
#define POLY_FENCE_HOST_SHARED_FULL      8
```

### Required Guarantees

- Frontend writes before a release fence are visible to the monitor before a
  subsequent monitor acquire observes the event.
- Monitor writes before a release fence are visible to resumed frontend code
  before a subsequent frontend acquire.
- Host-shared fences order memory that is shared with host kernel interfaces or
  native host code.
- Implementations may map these scopes to stronger native barriers.

### Userspace Impact

io_uring, futex-like handoffs, shared queues, and host-kernel mmap rings can be
supported without API-specific hardware. `polyexec` places fences around event
delivery, syscall submission, ring publication, and resume.

## Polyexec Simplification Map

This proposal moves architectural mechanics out of `polyexec` but leaves policy
there:

| Current burden | ISA replacement | What remains in userspace |
| --- | --- | --- |
| `/proc/self/maps` range checks and prefault walkers | `PMEM_PROBE_RANGE` | Deciding whether to copy, reject, or deliver a guest fault |
| Clone child register/TLS/SP/result setup | `PDERIVE_STATE` | Calling host clone/thread APIs and mapping Linux clone semantics |
| Signal delivery and sigreturn state import | `PDERIVE_STATE` | Building ABI-specific signal frames |
| Trap return register mutation and next-PC handling | `PCOMPLETE_EVENT` | Syscall translation, seccomp decisions, errno policy |
| Fatal crash register reconstruction | `PDUMP_STATE` | ELF core, minidump, or debugger file wrapping |
| io_uring and shared mmap ordering assumptions | `PFENCE` | Syscall passthrough and ring API policy |

Seccomp is the clearest boundary test. Hardware should not evaluate BPF. The
monitor should keep evaluating guest seccomp filters against guest syscall
numbers, then use `PCOMPLETE_EVENT` to apply `EPERM`, `ENOSYS`, success, or
signal-like outcomes to the frontend state.

## Acceptance Tests

1. Memory probe:
   - Probe mapped, unmapped, read-only, write-only, and noncanonical ranges.
   - Verify no host signal is raised and no page is faulted in by the probe.

2. Clone derive:
   - Create a frontend child state with new SP, TLS, and child return value.
   - Verify parent and child state generations differ and no partial state is
     visible on descriptor failure.

3. Signal/return:
   - Deliver a guest signal using a userspace-built frame and `PDERIVE_STATE`.
   - Restore with imported GPR/FP/status state.
   - Verify interrupted PC and resumed PC match the event frame.

4. Event completion:
   - Complete a trapped syscall with success, failure, retry, and next-PC
     policies.
   - Verify stale event generation completion fails.

5. Debug export:
   - Crash an AArch64 program.
   - Use `PDUMP_STATE` to emit a register blob.
   - Wrap the blob in ELF core notes in userspace and verify `gdb-multiarch`
     shows the expected PC, SP, GPRs, FP state, and backtrace.

6. Shared-memory ordering:
   - Run an AArch64 `fio` io_uring workload through `polyexec`.
   - Place `PFENCE` around ring submission and completion handoff.
   - Verify no stale SQE/CQE observation under stress.

7. Seccomp boundary:
   - Run a strict container profile that blocks `mkdir`.
   - Verify `polyexec` evaluates the guest filter in userspace and uses
     `PCOMPLETE_EVENT` to return `EPERM` without installing the guest filter on
     the host kernel.

## Migration Plan

1. Add feature discovery and conformance tests for the five primitives.
2. Implement `PMEM_PROBE_RANGE`, `PDUMP_STATE`, and conservative `PFENCE`
   semantics first.
3. Implement `PDERIVE_STATE` for same-frontend state copy plus SP, TLS, PC, and
   result edits.
4. Implement `PCOMPLETE_EVENT` and convert trap return paths.
5. Replace `/proc/self/maps` probes, clone handoff mutation, signal restore
   mutation, and crash register reconstruction in `polyexec`.
6. Keep old userspace paths behind feature checks until the Bochs model and
   hardware implementation pass the acceptance suite.

## Design Rule

The line is simple:

Hardware owns architectural truth and atomic state movement. Userspace owns OS
meaning.

That line keeps the ISA useful beyond Linux while making the current Linux
runtime much smaller, less racy, and easier to debug.
