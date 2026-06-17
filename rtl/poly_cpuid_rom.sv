// Poly CPUID discovery ROM for FPGA/silicon bring-up.
//
// This is not a full x86 CPUID implementation. It only returns the Poly vendor
// leaves that describe fixed hardware contracts: feature bits, XSAVE geometry,
// trap-packet format, interrupt/memory rules, transitions, and ABI signatures.
module poly_cpuid_rom (
    input  logic        valid_i,
    input  logic [31:0] leaf_i,
    input  logic [31:0] subleaf_i,
    output logic        hit_o,
    output logic [31:0] eax_o,
    output logic [31:0] ebx_o,
    output logic [31:0] ecx_o,
    output logic [31:0] edx_o
);
  localparam logic [31:0] POLY_CPUID_BASE = 32'h40000000;
  localparam logic [31:0] POLY_CPUID_MAX  = 32'h4000000a;

  localparam logic [31:0] POLY_VENDOR_EBX = 32'h796c6f50; // "Poly"
  localparam logic [31:0] POLY_VENDOR_EDX = 32'h746f6c67; // "glot"
  localparam logic [31:0] POLY_VENDOR_ECX = 32'h21555043; // "CPU!"

  localparam logic [31:0] POLY_FEATURE_MASK = 32'hbe3fffff;
  localparam logic [31:0] POLY_STATE_MASK = 32'h00ffbf61;
  localparam logic [31:0] POLY_MODE_MASK = 32'h00000007;
  localparam logic [31:0] POLY_RAW_MODE_MASK = 32'h00000006;
  localparam logic [31:0] POLY_FRONTEND_MASK = 32'h00000007;

  localparam logic [31:0] POLY_STATE_XSAVE_COMPONENT_ARCH = 32'd20;
  localparam logic [31:0] POLY_STATE_XSAVE_BYTES_ARCH = 32'h00002000;
  localparam logic [31:0] POLY_STATE_XSAVE_LAYOUT_VERSION = 32'd12;
  localparam logic [31:0] POLY_STATE_XSAVE_ALIGN_ARCH = 32'd64;
  localparam logic [31:0] POLY_STATE_XSAVE_LAYOUT_ECX = 32'h0040000c;
  localparam logic [31:0] POLY_STATE_XSAVE_FLAGS = 32'h0001ff7c;

  localparam logic [31:0] POLY_STATE_XSAVE_HEADER_OFFSET = 32'h00000000;
  localparam logic [31:0] POLY_STATE_XSAVE_HEADER_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_MAGIC = 32'h31594c50;
  localparam logic [31:0] POLY_STATE_XSAVE_EVENT_RECORD_OFFSET = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_EVENT_RECORD_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_EVENT_ARGS_OFFSET = 32'h00000080;
  localparam logic [31:0] POLY_STATE_XSAVE_EVENT_ARGS_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_TRANSITION_OFFSET = 32'h000000c0;
  localparam logic [31:0] POLY_STATE_XSAVE_TRANSITION_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_AARCH64_GPR_OFFSET = 32'h00000100;
  localparam logic [31:0] POLY_STATE_XSAVE_AARCH64_GPR_BYTES = 32'h00000100;
  localparam logic [31:0] POLY_STATE_XSAVE_AARCH64_FP_OFFSET = 32'h00000200;
  localparam logic [31:0] POLY_STATE_XSAVE_AARCH64_FP_BYTES = 32'h00000200;
  localparam logic [31:0] POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET = 32'h00000400;
  localparam logic [31:0] POLY_STATE_XSAVE_AARCH64_STATUS_BYTES = 32'h00000080;
  localparam logic [31:0] POLY_STATE_XSAVE_RISCV_GPR_OFFSET = 32'h00000480;
  localparam logic [31:0] POLY_STATE_XSAVE_RISCV_GPR_BYTES = 32'h00000100;
  localparam logic [31:0] POLY_STATE_XSAVE_RISCV_FP_OFFSET = 32'h00000580;
  localparam logic [31:0] POLY_STATE_XSAVE_RISCV_FP_BYTES = 32'h00000200;
  localparam logic [31:0] POLY_STATE_XSAVE_RISCV_STATUS_OFFSET = 32'h00000780;
  localparam logic [31:0] POLY_STATE_XSAVE_RISCV_STATUS_BYTES = 32'h00000080;
  localparam logic [31:0] POLY_STATE_XSAVE_IMPORT_RETURN_OFFSET = 32'h00000800;
  localparam logic [31:0] POLY_STATE_XSAVE_IMPORT_RETURN_BYTES = 32'h00000500;
  localparam logic [31:0] POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH = 32'd8;
  localparam logic [31:0] POLY_STATE_XSAVE_IMPORT_RETURN_FRAME_BYTES = 32'h00000080;
  localparam logic [31:0] POLY_STATE_XSAVE_ABI_SIGNATURE_OFFSET = 32'h00000d00;
  localparam logic [31:0] POLY_STATE_XSAVE_ABI_SIGNATURE_BYTES = 32'h00000080;
  localparam logic [31:0] POLY_ABI_SIGNATURE_SLOT_COUNT = 32'd13;
  localparam logic [31:0] POLY_STATE_XSAVE_CROSS_RETURN_OFFSET = 32'h00000d80;
  localparam logic [31:0] POLY_STATE_XSAVE_CROSS_RETURN_BYTES = 32'h00000120;
  localparam logic [31:0] POLY_STATE_XSAVE_CROSS_RETURN_DEPTH = 32'd8;
  localparam logic [31:0] POLY_STATE_XSAVE_CROSS_RETURN_FRAME_BYTES = 32'h00000020;
  localparam logic [31:0] POLY_STATE_XSAVE_FRONTEND_TLS_OFFSET = 32'h00000ea0;
  localparam logic [31:0] POLY_STATE_XSAVE_FRONTEND_TLS_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_LANDING_POLICY_OFFSET = 32'h00000ee0;
  localparam logic [31:0] POLY_STATE_XSAVE_LANDING_POLICY_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_STATE_KEY_OFFSET = 32'h00000f20;
  localparam logic [31:0] POLY_STATE_XSAVE_STATE_KEY_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_TRAP_RESTORE_OFFSET = 32'h00001000;
  localparam logic [31:0] POLY_STATE_XSAVE_TRAP_RESTORE_BYTES = 32'h00000800;
  localparam logic [31:0] POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET = 32'h00001800;
  localparam logic [31:0] POLY_STATE_XSAVE_NATIVE_RETURN_BYTES = 32'h00000280;
  localparam logic [31:0] POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH = 32'd8;
  localparam logic [31:0] POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_STATE_XSAVE_RESERVED_OFFSET = 32'h00001a80;
  localparam logic [31:0] POLY_STATE_XSAVE_RESERVED_BYTES = 32'h00000580;

  localparam logic [31:0] POLY_EVENT_RECORD_LAYOUT_VERSION = 32'd2;
  localparam logic [31:0] POLY_EVENT_RECORD_ARG_COUNT = 32'd8;
  localparam logic [31:0] POLY_EVENT_RECORD_FLAGS = 32'h0000005f;

  localparam logic [31:0] POLY_INTERRUPT_ABI_VERSION = 32'd1;
  localparam logic [31:0] POLY_INTERRUPT_FLAGS = 32'h0000001f;
  localparam logic [31:0] POLY_INTERRUPT_RETURN_FLAGS = 32'h0000000f;

  localparam logic [31:0] POLY_MEMORY_ABI_VERSION = 32'd1;
  localparam logic [31:0] POLY_MEMORY_MODEL_X86_TSO = 32'd1;
  localparam logic [31:0] POLY_MEMORY_FLAGS = 32'h0000001f;

  localparam logic [31:0] POLY_TRANSITION_ABI_VERSION = 32'd1;
  localparam logic [31:0] POLY_TRANSITION_FLAGS = 32'h00000fff;
  localparam logic [31:0] POLY_TRANSITION_ALIGN_ECX = 32'h00020004;
  localparam logic [31:0] POLY_TRANSITION_FRAME_BYTES = 32'h00000020;

  localparam logic [31:0] POLY_FRONTEND_X86 = 32'd0;
  localparam logic [31:0] POLY_FRONTEND_AARCH64 = 32'd1;
  localparam logic [31:0] POLY_FRONTEND_RISCV = 32'd2;

  localparam logic [31:0] POLY_ABI_BRIDGE_ABI_VERSION = 32'd1;
  localparam logic [31:0] POLY_ABI_BRIDGE_FLAGS = 32'h00007e9f;
  localparam logic [31:0] POLY_ABI_BRIDGE_COUNTS_ALIGN = 32'h00100808;

  localparam logic [31:0] POLY_CPUID_V2_ABI_VERSION = 32'd2;
  localparam logic [31:0] POLY_CPUID_V2_FEATURES = 32'h0000005d;
  localparam logic [31:0] POLY_CPUID_V2_REQUIRED_FEATURES = 32'h00000001;
  localparam logic [31:0] POLY_V2_EVENT_BYTES = 32'd512;
  localparam logic [31:0] POLY_V2_SIZE_EDX = 32'h00000200;

  localparam logic [31:0] POLY_X86_CTRL_FOREIGN_BREAK_COUNT_STATUS = 32'h00000044;
  localparam logic [31:0] POLY_X86_CTRL_FOREIGN_IMPORT_COUNT_STATUS = 32'h00000045;
  localparam logic [31:0] POLY_X86_CTRL_EVENT_PTR_SET = 32'h00000071;
  localparam logic [31:0] POLY_X86_CTRL_PRESTORE = 32'h00000070;
  localparam logic [31:0] POLY_X86_OPCODE_GEOMETRY_EAX = 32'h00fc3a0f;
  localparam logic [31:0] POLY_X86_CTRL_PREFIX_BYTES = 32'd3;
  localparam logic [31:0] POLY_X86_CTRL_TOTAL_BYTES = 32'd4;
  localparam logic [31:0] POLY_X86_CTRL_SUBOP_OFFSET = 32'd3;
  localparam logic [31:0] POLY_X86_OPCODE_CONTRACT_VERSION = 32'd1;
  localparam logic [31:0] POLY_X86_OPCODE_FLAGS = 32'h0000003f;
  localparam logic [31:0] POLY_X86_OPCODE_FAMILY_VENDOR_PROTOTYPE = 32'd1;

  localparam logic [31:0] POLY_U64_BYTES = 32'd8;
  localparam logic [31:0] POLY_U128_BYTES = 32'd16;
  localparam logic [31:0] POLY_AARCH64_STATUS_BYTES = 32'h00000080;
  localparam logic [31:0] POLY_RISCV_STATUS_BYTES = 32'h00000080;
  localparam logic [31:0] POLY_ABI_SIGNATURE_SLOT_BYTES = 32'd8;
  localparam logic [31:0] POLY_FRONTEND_TLS_STATE_BYTES = 32'h00000040;
  localparam logic [31:0] POLY_LANDING_POLICY_SUPPORTED = 32'h00000003;
  localparam logic [31:0] POLY_STATE_KEY_FLAG_EXPLICIT = 32'h00000001;
  localparam logic [31:0] POLY_TRAP_RESTORE_FLAGS = 32'h00000007;

  always_comb begin
    hit_o = 1'b0;
    eax_o = 32'd0;
    ebx_o = 32'd0;
    ecx_o = 32'd0;
    edx_o = 32'd0;

    if (valid_i) begin
      unique case (leaf_i)
        POLY_CPUID_BASE: begin
          hit_o = 1'b1;
          eax_o = POLY_CPUID_MAX;
          ebx_o = POLY_VENDOR_EBX;
          ecx_o = POLY_VENDOR_ECX;
          edx_o = POLY_VENDOR_EDX;
        end
        POLY_CPUID_BASE + 32'd1: begin
          hit_o = 1'b1;
          eax_o = 32'd1;
          ebx_o = POLY_MODE_MASK;
          ecx_o = POLY_FEATURE_MASK;
          edx_o = POLY_STATE_XSAVE_COMPONENT_ARCH;
        end
        POLY_CPUID_BASE + 32'd2: begin
          unique case (subleaf_i)
            32'd31: begin
              hit_o = 1'b1;
              eax_o = POLY_X86_CTRL_FOREIGN_BREAK_COUNT_STATUS;
              ebx_o = POLY_X86_CTRL_FOREIGN_IMPORT_COUNT_STATUS;
              ecx_o = POLY_X86_CTRL_EVENT_PTR_SET;
              edx_o = 32'd0;
            end
            32'd32: begin
              hit_o = 1'b1;
              eax_o = POLY_X86_OPCODE_GEOMETRY_EAX;
              ebx_o = POLY_X86_CTRL_PREFIX_BYTES;
              ecx_o = POLY_X86_CTRL_TOTAL_BYTES;
              edx_o = POLY_X86_CTRL_SUBOP_OFFSET;
            end
            32'd33: begin
              hit_o = 1'b1;
              eax_o = POLY_X86_OPCODE_CONTRACT_VERSION;
              ebx_o = POLY_X86_OPCODE_FLAGS;
              ecx_o = POLY_X86_OPCODE_FAMILY_VENDOR_PROTOTYPE;
              edx_o = 32'd0;
            end
            default: begin
              hit_o = 1'b0;
            end
          endcase
        end
        POLY_CPUID_BASE + 32'd3: begin
          hit_o = 1'b1;
          eax_o = POLY_STATE_MASK;
          ebx_o = 32'd0;
          ecx_o = POLY_STATE_XSAVE_COMPONENT_ARCH;
          edx_o = POLY_STATE_XSAVE_BYTES_ARCH;
        end
        POLY_CPUID_BASE + 32'd4: begin
          unique case (subleaf_i)
            32'd0: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_COMPONENT_ARCH;
              ebx_o = POLY_STATE_XSAVE_BYTES_ARCH;
              ecx_o = POLY_STATE_XSAVE_LAYOUT_ECX;
              edx_o = POLY_STATE_XSAVE_FLAGS;
            end
            32'd1: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_HEADER_OFFSET;
              ebx_o = POLY_STATE_XSAVE_HEADER_BYTES;
              ecx_o = POLY_STATE_XSAVE_MAGIC;
              edx_o = POLY_STATE_XSAVE_LAYOUT_VERSION;
            end
            32'd2: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_EVENT_RECORD_OFFSET;
              ebx_o = POLY_STATE_XSAVE_EVENT_RECORD_BYTES;
              ecx_o = POLY_STATE_XSAVE_EVENT_ARGS_OFFSET;
              edx_o = POLY_STATE_XSAVE_EVENT_ARGS_BYTES;
            end
            32'd3: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_AARCH64_GPR_OFFSET;
              ebx_o = POLY_STATE_XSAVE_AARCH64_GPR_BYTES;
              ecx_o = 32'd32;
              edx_o = POLY_U64_BYTES;
            end
            32'd4: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_AARCH64_FP_OFFSET;
              ebx_o = POLY_STATE_XSAVE_AARCH64_FP_BYTES;
              ecx_o = 32'd32;
              edx_o = POLY_U128_BYTES;
            end
            32'd5: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_AARCH64_STATUS_OFFSET;
              ebx_o = POLY_STATE_XSAVE_AARCH64_STATUS_BYTES;
              ecx_o = POLY_AARCH64_STATUS_BYTES;
              edx_o = 32'd0;
            end
            32'd6: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_RISCV_GPR_OFFSET;
              ebx_o = POLY_STATE_XSAVE_RISCV_GPR_BYTES;
              ecx_o = 32'd32;
              edx_o = POLY_U64_BYTES;
            end
            32'd7: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_RISCV_FP_OFFSET;
              ebx_o = POLY_STATE_XSAVE_RISCV_FP_BYTES;
              ecx_o = 32'd32;
              edx_o = POLY_U128_BYTES;
            end
            32'd8: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_RISCV_STATUS_OFFSET;
              ebx_o = POLY_STATE_XSAVE_RISCV_STATUS_BYTES;
              ecx_o = POLY_RISCV_STATUS_BYTES;
              edx_o = 32'd0;
            end
            32'd9: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_ABI_SIGNATURE_OFFSET;
              ebx_o = POLY_STATE_XSAVE_ABI_SIGNATURE_BYTES;
              ecx_o = POLY_ABI_SIGNATURE_SLOT_COUNT;
              edx_o = POLY_ABI_SIGNATURE_SLOT_BYTES;
            end
            32'd10: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_FRONTEND_TLS_OFFSET;
              ebx_o = POLY_STATE_XSAVE_FRONTEND_TLS_BYTES;
              ecx_o = POLY_FRONTEND_TLS_STATE_BYTES;
              edx_o = 32'd0;
            end
            32'd11: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_LANDING_POLICY_OFFSET;
              ebx_o = POLY_STATE_XSAVE_LANDING_POLICY_BYTES;
              ecx_o = POLY_LANDING_POLICY_SUPPORTED;
              edx_o = 32'd0;
            end
            32'd12: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_STATE_KEY_OFFSET;
              ebx_o = POLY_STATE_XSAVE_STATE_KEY_BYTES;
              ecx_o = POLY_STATE_KEY_FLAG_EXPLICIT;
              edx_o = 32'd0;
            end
            32'd13: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_TRAP_RESTORE_OFFSET;
              ebx_o = POLY_STATE_XSAVE_TRAP_RESTORE_BYTES;
              ecx_o = POLY_TRAP_RESTORE_FLAGS;
              edx_o = 32'd0;
            end
            32'd14: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET;
              ebx_o = POLY_STATE_XSAVE_NATIVE_RETURN_BYTES;
              ecx_o = POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH;
              edx_o = POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES;
            end
            32'd15: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_RESERVED_OFFSET;
              ebx_o = POLY_STATE_XSAVE_RESERVED_BYTES;
              ecx_o = 32'd0;
              edx_o = 32'd0;
            end
            default: begin
              hit_o = 1'b0;
            end
          endcase
        end
        POLY_CPUID_BASE + 32'd5: begin
          hit_o = 1'b1;
          eax_o = POLY_EVENT_RECORD_LAYOUT_VERSION;
          ebx_o = POLY_STATE_XSAVE_EVENT_RECORD_BYTES;
          ecx_o = POLY_EVENT_RECORD_ARG_COUNT;
          edx_o = POLY_EVENT_RECORD_FLAGS;
        end
        POLY_CPUID_BASE + 32'd6: begin
          hit_o = 1'b1;
          eax_o = POLY_INTERRUPT_ABI_VERSION;
          ebx_o = POLY_INTERRUPT_FLAGS;
          ecx_o = POLY_INTERRUPT_RETURN_FLAGS;
          edx_o = POLY_RAW_MODE_MASK;
        end
        POLY_CPUID_BASE + 32'd7: begin
          hit_o = 1'b1;
          eax_o = POLY_MEMORY_ABI_VERSION;
          ebx_o = POLY_MEMORY_MODEL_X86_TSO;
          ecx_o = POLY_MEMORY_FLAGS;
          edx_o = POLY_RAW_MODE_MASK;
        end
        POLY_CPUID_BASE + 32'd8: begin
          unique case (subleaf_i)
            32'd0: begin
              hit_o = 1'b1;
              eax_o = POLY_TRANSITION_ABI_VERSION;
              ebx_o = POLY_TRANSITION_FLAGS;
              ecx_o = POLY_TRANSITION_ALIGN_ECX;
              edx_o = POLY_MODE_MASK;
            end
            32'd1: begin
              hit_o = 1'b1;
              eax_o = POLY_FRONTEND_X86;
              ebx_o = POLY_FRONTEND_AARCH64;
              ecx_o = POLY_FRONTEND_RISCV;
              edx_o = POLY_FRONTEND_MASK;
            end
            32'd2: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_TRANSITION_OFFSET;
              ebx_o = POLY_STATE_XSAVE_TRANSITION_BYTES;
              ecx_o = POLY_TRANSITION_FRAME_BYTES;
              edx_o = 32'd0;
            end
            32'd3: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_CROSS_RETURN_OFFSET;
              ebx_o = POLY_STATE_XSAVE_CROSS_RETURN_BYTES;
              ecx_o = POLY_STATE_XSAVE_CROSS_RETURN_DEPTH;
              edx_o = POLY_STATE_XSAVE_CROSS_RETURN_FRAME_BYTES;
            end
            32'd4: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_IMPORT_RETURN_OFFSET;
              ebx_o = POLY_STATE_XSAVE_IMPORT_RETURN_BYTES;
              ecx_o = POLY_STATE_XSAVE_IMPORT_RETURN_DEPTH;
              edx_o = POLY_STATE_XSAVE_IMPORT_RETURN_FRAME_BYTES;
            end
            32'd5: begin
              hit_o = 1'b1;
              eax_o = POLY_STATE_XSAVE_NATIVE_RETURN_OFFSET;
              ebx_o = POLY_STATE_XSAVE_NATIVE_RETURN_BYTES;
              ecx_o = POLY_STATE_XSAVE_NATIVE_RETURN_DEPTH;
              edx_o = POLY_STATE_XSAVE_NATIVE_RETURN_FRAME_BYTES;
            end
            default: begin
              hit_o = 1'b0;
            end
          endcase
        end
        POLY_CPUID_BASE + 32'd9: begin
          hit_o = 1'b1;
          eax_o = POLY_ABI_BRIDGE_ABI_VERSION;
          ebx_o = POLY_ABI_BRIDGE_FLAGS;
          ecx_o = POLY_ABI_BRIDGE_COUNTS_ALIGN;
          edx_o = 32'd0;
        end
        POLY_CPUID_BASE + 32'd10: begin
          hit_o = 1'b1;
          eax_o = POLY_CPUID_V2_ABI_VERSION;
          ebx_o = POLY_CPUID_V2_FEATURES;
          ecx_o = POLY_CPUID_V2_REQUIRED_FEATURES;
          edx_o = POLY_V2_SIZE_EDX;
        end
        default: begin
          hit_o = 1'b0;
        end
      endcase
    end
  end
endmodule
