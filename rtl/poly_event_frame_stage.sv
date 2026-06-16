// OS-neutral Poly v2 event-frame delivery stage.
//
// The encoder validates architectural event-frame geometry and builds the
// fixed frame image. This stage adds memory-response ordering: a recoverable
// foreign exit is delivered only after the frame write completes without a
// memory fault.
module poly_event_frame_stage (
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

    input  logic        mem_write_resp_valid_i,
    input  logic        mem_write_fault_i,

    output logic        mem_write_valid_o,
    output logic [63:0] mem_write_addr_o,
    output logic [9:0]  mem_write_bytes_o,
    output logic [4095:0] mem_write_data_o,

    output logic        wait_response_o,
    output logic        frame_delivered_o,
    output logic        fault_o,
    output logic        encode_error_o,
    output logic        frame_mem_fault_o,
    output logic        event_disabled_o,
    output logic        noncanonical_frame_o,
    output logic        frame_align_fault_o,
    output logic        frame_range_fault_o,
    output logic        invalid_event_kind_o,
    output logic        invalid_source_frontend_o
);
  logic frame_valid;

  poly_event_frame_encode frame_encode (
    .valid_i(valid_i),
    .event_enabled_i(event_enabled_i),
    .event_frame_addr_i(event_frame_addr_i),
    .event_kind_i(event_kind_i),
    .source_frontend_i(source_frontend_i),
    .number_i(number_i),
    .selector_i(selector_i),
    .insn_pc_i(insn_pc_i),
    .resume_pc_i(resume_pc_i),
    .arg0_i(arg0_i),
    .arg1_i(arg1_i),
    .arg2_i(arg2_i),
    .arg3_i(arg3_i),
    .arg4_i(arg4_i),
    .arg5_i(arg5_i),
    .arg6_i(arg6_i),
    .arg7_i(arg7_i),
    .frame_valid_o(frame_valid),
    .frame_addr_o(mem_write_addr_o),
    .frame_bytes_o(mem_write_bytes_o),
    .frame_data_o(mem_write_data_o),
    .error_o(encode_error_o),
    .event_disabled_o(event_disabled_o),
    .noncanonical_frame_o(noncanonical_frame_o),
    .frame_align_fault_o(frame_align_fault_o),
    .frame_range_fault_o(frame_range_fault_o),
    .invalid_event_kind_o(invalid_event_kind_o),
    .invalid_source_frontend_o(invalid_source_frontend_o)
  );

  always_comb begin
    mem_write_valid_o = frame_valid;
    wait_response_o = frame_valid && !mem_write_resp_valid_i;
    frame_mem_fault_o = frame_valid && mem_write_resp_valid_i && mem_write_fault_i;
    frame_delivered_o =
      frame_valid && mem_write_resp_valid_i && !mem_write_fault_i;
    fault_o = encode_error_o || frame_mem_fault_o;
  end
endmodule
