// OS-neutral Poly trap-packet delivery stage.
//
// The encoder validates architectural packet geometry and builds the fixed
// packet image. This stage adds memory-response ordering: a recoverable foreign
// exit is delivered only after the monitor-packet write completes without a
// memory fault. Translation, permissions, and page-fault classification remain
// owned by the normal memory subsystem.
module poly_trap_packet_stage (
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

    input  logic        mem_write_resp_valid_i,
    input  logic        mem_write_fault_i,

    output logic        mem_write_valid_o,
    output logic [63:0] mem_write_addr_o,
    output logic [7:0]  mem_write_bytes_o,
    output logic [63:0] mem_write_qword0_o,
    output logic [63:0] mem_write_qword1_o,
    output logic [63:0] mem_write_qword2_o,
    output logic [63:0] mem_write_qword3_o,
    output logic [63:0] mem_write_qword4_o,
    output logic [63:0] mem_write_qword5_o,
    output logic [63:0] mem_write_qword6_o,
    output logic [63:0] mem_write_qword7_o,
    output logic [63:0] mem_write_qword8_o,
    output logic [63:0] mem_write_qword9_o,
    output logic [63:0] mem_write_qword10_o,
    output logic [63:0] mem_write_qword11_o,
    output logic [63:0] mem_write_qword12_o,
    output logic [63:0] mem_write_qword13_o,
    output logic [63:0] mem_write_qword14_o,
    output logic [63:0] mem_write_qword15_o,

    output logic        wait_response_o,
    output logic        packet_delivered_o,
    output logic        fault_o,
    output logic        encode_error_o,
    output logic        packet_mem_fault_o,
    output logic        monitor_disabled_o,
    output logic        noncanonical_packet_o,
    output logic        packet_align_fault_o,
    output logic        packet_range_fault_o,
    output logic        invalid_reason_o,
    output logic        invalid_source_mode_o
);
  logic packet_valid;

  poly_trap_packet_encode packet_encode (
    .valid_i(valid_i),
    .monitor_enabled_i(monitor_enabled_i),
    .monitor_packet_addr_i(monitor_packet_addr_i),
    .reason_i(reason_i),
    .source_mode_i(source_mode_i),
    .number_i(number_i),
    .selector_i(selector_i),
    .trap_pc_i(trap_pc_i),
    .resume_pc_i(resume_pc_i),
    .arg0_i(arg0_i),
    .arg1_i(arg1_i),
    .arg2_i(arg2_i),
    .arg3_i(arg3_i),
    .arg4_i(arg4_i),
    .arg5_i(arg5_i),
    .arg6_i(arg6_i),
    .arg7_i(arg7_i),
    .packet_valid_o(packet_valid),
    .packet_addr_o(mem_write_addr_o),
    .packet_bytes_o(mem_write_bytes_o),
    .packet_qword0_o(mem_write_qword0_o),
    .packet_qword1_o(mem_write_qword1_o),
    .packet_qword2_o(mem_write_qword2_o),
    .packet_qword3_o(mem_write_qword3_o),
    .packet_qword4_o(mem_write_qword4_o),
    .packet_qword5_o(mem_write_qword5_o),
    .packet_qword6_o(mem_write_qword6_o),
    .packet_qword7_o(mem_write_qword7_o),
    .packet_qword8_o(mem_write_qword8_o),
    .packet_qword9_o(mem_write_qword9_o),
    .packet_qword10_o(mem_write_qword10_o),
    .packet_qword11_o(mem_write_qword11_o),
    .packet_qword12_o(mem_write_qword12_o),
    .packet_qword13_o(mem_write_qword13_o),
    .packet_qword14_o(mem_write_qword14_o),
    .packet_qword15_o(mem_write_qword15_o),
    .error_o(encode_error_o),
    .monitor_disabled_o(monitor_disabled_o),
    .noncanonical_packet_o(noncanonical_packet_o),
    .packet_align_fault_o(packet_align_fault_o),
    .packet_range_fault_o(packet_range_fault_o),
    .invalid_reason_o(invalid_reason_o),
    .invalid_source_mode_o(invalid_source_mode_o)
  );

  always_comb begin
    mem_write_valid_o = packet_valid;
    wait_response_o = packet_valid && !mem_write_resp_valid_i;
    packet_mem_fault_o = packet_valid && mem_write_resp_valid_i && mem_write_fault_i;
    packet_delivered_o =
      packet_valid && mem_write_resp_valid_i && !mem_write_fault_i;
    fault_o = encode_error_o || packet_mem_fault_o;
  end
endmodule
