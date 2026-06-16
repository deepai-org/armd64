// OS-neutral Poly v2 event-frame encoder.
//
// This block turns a recoverable frontend exit into the canonical 512-byte
// event frame described by docs/poly-isa-v2-draft.md. It validates only
// architectural address geometry and event metadata. OS policy, syscall
// translation, signal construction, and debugger file formats remain in
// userspace.
module poly_event_frame_encode (
    input  logic        valid_i,
    input  logic        event_enabled_i,
    input  logic [63:0] event_frame_addr_i,
    input  logic [31:0] event_kind_i,
    input  logic [31:0] source_frontend_i,
    input  logic [63:0] number_i,
    input  logic [63:0] selector_i,
    input  logic [63:0] insn_pc_i,
    input  logic [63:0] resume_pc_i,
    input  logic [63:0] arg0_i,
    input  logic [63:0] arg1_i,
    input  logic [63:0] arg2_i,
    input  logic [63:0] arg3_i,
    input  logic [63:0] arg4_i,
    input  logic [63:0] arg5_i,
    input  logic [63:0] arg6_i,
    input  logic [63:0] arg7_i,

    output logic        frame_valid_o,
    output logic [63:0] frame_addr_o,
    output logic [9:0]  frame_bytes_o,
    output logic [4095:0] frame_data_o,

    output logic        error_o,
    output logic        event_disabled_o,
    output logic        noncanonical_frame_o,
    output logic        frame_align_fault_o,
    output logic        frame_range_fault_o,
    output logic        invalid_event_kind_o,
    output logic        invalid_source_frontend_o
);
  localparam logic [31:0] POLY_FRONTEND_RAW_AARCH64 = 32'd1;
  localparam logic [31:0] POLY_FRONTEND_RAW_RISCV   = 32'd2;

  localparam logic [31:0] POLY_EVENT_SYSCALL = 32'd1;
  localparam logic [31:0] POLY_EVENT_BREAK   = 32'd2;
  localparam logic [31:0] POLY_EVENT_IMPORT  = 32'd3;
  localparam logic [31:0] POLY_EVENT_ILLEGAL = 32'd4;

  localparam logic [63:0] POLY_V2_EVENT_MAGIC = 64'h32545645594c4f50;
  localparam logic [31:0] POLY_V2_EVENT_BYTES = 32'd512;
  localparam logic [15:0] POLY_V2_EVENT_VERSION = 16'd2;
  localparam logic [15:0] POLY_V2_EVENT_HEADER_BYTES = 16'd408;
  localparam logic [15:0] POLY_V2_EVENT_ARG_COUNT = 16'd8;
  localparam logic [63:0] POLY_V2_EVENT_ALIGN_MASK = 64'd63;
  localparam logic [63:0] POLY_V2_EVENT_LAST_OFFSET = 64'd511;

  logic [63:0] frame_last_addr;
  logic range_wrap;
  logic frame_start_canonical;
  logic frame_end_canonical;
  logic [63:0] qword [0:63];
  integer i;

  function automatic logic canonical64(input logic [63:0] addr);
    begin
      canonical64 = addr[63:48] == {16{addr[47]}};
    end
  endfunction

  always_comb begin
    frame_last_addr = event_frame_addr_i + POLY_V2_EVENT_LAST_OFFSET;
    range_wrap = frame_last_addr < event_frame_addr_i;
    frame_start_canonical = canonical64(event_frame_addr_i);
    frame_end_canonical = canonical64(frame_last_addr);

    event_disabled_o = valid_i && !event_enabled_i;
    noncanonical_frame_o =
      valid_i && event_enabled_i &&
      (!frame_start_canonical || !frame_end_canonical);
    frame_align_fault_o =
      valid_i && event_enabled_i &&
      (event_frame_addr_i & POLY_V2_EVENT_ALIGN_MASK) != 64'd0;
    frame_range_fault_o =
      valid_i && event_enabled_i && range_wrap;
    invalid_event_kind_o =
      valid_i && event_enabled_i &&
      !(event_kind_i == POLY_EVENT_SYSCALL ||
        event_kind_i == POLY_EVENT_BREAK ||
        event_kind_i == POLY_EVENT_IMPORT ||
        event_kind_i == POLY_EVENT_ILLEGAL);
    invalid_source_frontend_o =
      valid_i && event_enabled_i &&
      !(source_frontend_i == POLY_FRONTEND_RAW_AARCH64 ||
        source_frontend_i == POLY_FRONTEND_RAW_RISCV);

    error_o =
      event_disabled_o ||
      noncanonical_frame_o ||
      frame_align_fault_o ||
      frame_range_fault_o ||
      invalid_event_kind_o ||
      invalid_source_frontend_o;
    frame_valid_o = valid_i && !error_o;
    frame_addr_o = event_frame_addr_i;
    frame_bytes_o = POLY_V2_EVENT_BYTES[9:0];

    for (i = 0; i < 64; i = i + 1) begin
      qword[i] = 64'd0;
    end

    qword[0] = POLY_V2_EVENT_MAGIC;
    qword[1] = {
      POLY_V2_EVENT_HEADER_BYTES,
      POLY_V2_EVENT_VERSION,
      POLY_V2_EVENT_BYTES
    };
    qword[2] = 64'd0;
    qword[3] = {16'd0, event_kind_i[15:0], source_frontend_i};
    qword[4] = 64'd0;
    qword[5] = {POLY_V2_EVENT_ARG_COUNT, 16'd0, 32'd0};
    qword[6] = insn_pc_i;
    qword[7] = resume_pc_i;
    qword[8] = resume_pc_i;
    qword[9] = 64'd0;
    qword[10] = 64'd0;
    qword[11] = 64'd0;
    qword[12] = 64'd0;
    qword[13] = 64'd0;
    qword[14] = selector_i;
    qword[15] = arg0_i;
    qword[16] = arg1_i;
    qword[17] = arg2_i;
    qword[18] = arg3_i;
    qword[19] = arg4_i;
    qword[20] = arg5_i;
    qword[21] = arg6_i;
    qword[22] = arg7_i;
    qword[23] = 64'd0;
    qword[24] = number_i;

    for (i = 0; i < 64; i = i + 1) begin
      frame_data_o[(i * 64) +: 64] = qword[i];
    end
  end
endmodule
