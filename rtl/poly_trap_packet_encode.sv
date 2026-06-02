// OS-neutral Poly trap packet encoder.
//
// This block turns a recoverable frontend exit into the fixed monitor-packet
// memory image. It validates only architectural address geometry and packet
// metadata. It does not translate syscall numbers, import selectors, libcalls,
// or OS policy.
module poly_trap_packet_encode (
    input  logic        valid_i,
    input  logic        monitor_enabled_i,
    input  logic [63:0] monitor_packet_addr_i,
    input  logic [31:0] reason_i,
    input  logic [31:0] source_mode_i,
    input  logic [63:0] number_i,
    input  logic [63:0] selector_i,
    input  logic [63:0] trap_pc_i,
    input  logic [63:0] resume_pc_i,
    input  logic [63:0] arg0_i,
    input  logic [63:0] arg1_i,
    input  logic [63:0] arg2_i,
    input  logic [63:0] arg3_i,
    input  logic [63:0] arg4_i,
    input  logic [63:0] arg5_i,
    input  logic [63:0] arg6_i,
    input  logic [63:0] arg7_i,

    output logic        packet_valid_o,
    output logic [63:0] packet_addr_o,
    output logic [7:0]  packet_bytes_o,
    output logic [63:0] packet_qword0_o,
    output logic [63:0] packet_qword1_o,
    output logic [63:0] packet_qword2_o,
    output logic [63:0] packet_qword3_o,
    output logic [63:0] packet_qword4_o,
    output logic [63:0] packet_qword5_o,
    output logic [63:0] packet_qword6_o,
    output logic [63:0] packet_qword7_o,
    output logic [63:0] packet_qword8_o,
    output logic [63:0] packet_qword9_o,
    output logic [63:0] packet_qword10_o,
    output logic [63:0] packet_qword11_o,
    output logic [63:0] packet_qword12_o,
    output logic [63:0] packet_qword13_o,
    output logic [63:0] packet_qword14_o,
    output logic [63:0] packet_qword15_o,

    output logic        error_o,
    output logic        monitor_disabled_o,
    output logic        noncanonical_packet_o,
    output logic        packet_align_fault_o,
    output logic        packet_range_fault_o,
    output logic        invalid_reason_o,
    output logic        invalid_source_mode_o
);
  localparam logic [31:0] POLY_MODE_RAW_AARCH64 = 32'd1;
  localparam logic [31:0] POLY_MODE_RAW_RISCV   = 32'd2;

  localparam logic [31:0] POLY_TRAP_SYSCALL = 32'd1;
  localparam logic [31:0] POLY_TRAP_BREAK   = 32'd2;
  localparam logic [31:0] POLY_TRAP_IMPORT  = 32'd3;
  localparam logic [31:0] POLY_TRAP_ILLEGAL = 32'd4;

  localparam logic [7:0]  POLY_MONITOR_PACKET_BYTES = 8'd128;
  localparam logic [63:0] POLY_MONITOR_PACKET_LAST_OFFSET = 64'd127;
  localparam logic [63:0] POLY_TRAP_PACKET_REQUIRED_FLAGS = 64'h000000000000007f;

  logic [63:0] packet_last_addr;
  logic range_wrap;
  logic packet_start_canonical;
  logic packet_end_canonical;

  function automatic logic canonical64(input logic [63:0] addr);
    return addr[63:48] == {16{addr[47]}};
  endfunction

  always_comb begin
    packet_last_addr = monitor_packet_addr_i + POLY_MONITOR_PACKET_LAST_OFFSET;
    range_wrap = packet_last_addr < monitor_packet_addr_i;
    packet_start_canonical = canonical64(monitor_packet_addr_i);
    packet_end_canonical = canonical64(packet_last_addr);

    monitor_disabled_o = valid_i && !monitor_enabled_i;
    noncanonical_packet_o =
      valid_i && monitor_enabled_i &&
      (!packet_start_canonical || !packet_end_canonical);
    packet_align_fault_o =
      valid_i && monitor_enabled_i && monitor_packet_addr_i[5:0] != 6'd0;
    packet_range_fault_o =
      valid_i && monitor_enabled_i && range_wrap;
    invalid_reason_o =
      valid_i && monitor_enabled_i &&
      !(reason_i == POLY_TRAP_SYSCALL ||
        reason_i == POLY_TRAP_BREAK ||
        reason_i == POLY_TRAP_IMPORT ||
        reason_i == POLY_TRAP_ILLEGAL);
    invalid_source_mode_o =
      valid_i && monitor_enabled_i &&
      !(source_mode_i == POLY_MODE_RAW_AARCH64 ||
        source_mode_i == POLY_MODE_RAW_RISCV);

    error_o =
      monitor_disabled_o ||
      noncanonical_packet_o ||
      packet_align_fault_o ||
      packet_range_fault_o ||
      invalid_reason_o ||
      invalid_source_mode_o;
    packet_valid_o = valid_i && !error_o;
    packet_addr_o = monitor_packet_addr_i;
    packet_bytes_o = POLY_MONITOR_PACKET_BYTES;

    packet_qword0_o = {source_mode_i, reason_i};
    packet_qword1_o = number_i;
    packet_qword2_o = selector_i;
    packet_qword3_o = trap_pc_i;
    packet_qword4_o = resume_pc_i;
    packet_qword5_o = POLY_TRAP_PACKET_REQUIRED_FLAGS;
    packet_qword6_o = 64'd0;
    packet_qword7_o = 64'd0;
    packet_qword8_o = arg0_i;
    packet_qword9_o = arg1_i;
    packet_qword10_o = arg2_i;
    packet_qword11_o = arg3_i;
    packet_qword12_o = arg4_i;
    packet_qword13_o = arg5_i;
    packet_qword14_o = arg6_i;
    packet_qword15_o = arg7_i;
  end
endmodule
