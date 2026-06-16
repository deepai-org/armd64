// Poly transition cycle-budget model.
//
// This is not a timing closure result. It is a hardware-facing budget model
// for the fast paths that must stay fixed-latency and free of user-memory
// descriptor parsing: PSWITCH, register-only PCALL, native return-cookie
// recovery, and event-frame delivery.
module poly_transition_cycle_budget (
    input  logic       valid_i,
    input  logic [2:0] op_i,
    input  logic       register_only_signature_i,
    input  logic       signature_slot_valid_i,
    input  logic       transition_stack_ready_i,
    input  logic       event_frame_ready_i,
    input  logic [7:0] memory_response_cycles_i,

    output logic       budget_valid_o,
    output logic [7:0] fixed_cycles_o,
    output logic [7:0] variable_cycles_o,
    output logic [8:0] total_cycles_o,
    output logic       few_cycle_fast_path_o,
    output logic       waits_for_memory_o,
    output logic       unsupported_o,
    output logic       invalid_op_o,
    output logic       blocked_o
);
  localparam logic [2:0] POLY_CYCLE_OP_PSWITCH      = 3'd1;
  localparam logic [2:0] POLY_CYCLE_OP_PCALL_REG    = 3'd2;
  localparam logic [2:0] POLY_CYCLE_OP_RETURN_COOKIE = 3'd3;
  localparam logic [2:0] POLY_CYCLE_OP_EVENT_FRAME  = 3'd4;

  localparam logic [7:0] POLY_CYCLE_FEW_CYCLE_LIMIT = 8'd4;

  logic op_supported;
  logic requires_signature;
  logic requires_transition_stack;
  logic requires_event_frame;

  always_comb begin
    op_supported =
      op_i == POLY_CYCLE_OP_PSWITCH ||
      op_i == POLY_CYCLE_OP_PCALL_REG ||
      op_i == POLY_CYCLE_OP_RETURN_COOKIE ||
      op_i == POLY_CYCLE_OP_EVENT_FRAME;
    requires_signature = op_i == POLY_CYCLE_OP_PCALL_REG;
    requires_transition_stack =
      op_i == POLY_CYCLE_OP_PCALL_REG ||
      op_i == POLY_CYCLE_OP_RETURN_COOKIE;
    requires_event_frame = op_i == POLY_CYCLE_OP_EVENT_FRAME;

    invalid_op_o = valid_i && !op_supported;
    unsupported_o =
      valid_i && requires_signature && !register_only_signature_i;
    blocked_o =
      valid_i && op_supported && !unsupported_o &&
      ((requires_signature && !signature_slot_valid_i) ||
       (requires_transition_stack && !transition_stack_ready_i) ||
       (requires_event_frame && !event_frame_ready_i));

    unique case (op_i)
      POLY_CYCLE_OP_PSWITCH: fixed_cycles_o = 8'd3;
      POLY_CYCLE_OP_PCALL_REG: fixed_cycles_o = 8'd4;
      POLY_CYCLE_OP_RETURN_COOKIE: fixed_cycles_o = 8'd3;
      POLY_CYCLE_OP_EVENT_FRAME: fixed_cycles_o = 8'd2;
      default: fixed_cycles_o = 8'd0;
    endcase

    waits_for_memory_o =
      valid_i && op_i == POLY_CYCLE_OP_EVENT_FRAME && !invalid_op_o;
    variable_cycles_o = waits_for_memory_o ? memory_response_cycles_i : 8'd0;
    total_cycles_o = {1'b0, fixed_cycles_o} + {1'b0, variable_cycles_o};
    budget_valid_o = valid_i && op_supported && !unsupported_o && !blocked_o;
    few_cycle_fast_path_o =
      budget_valid_o && !waits_for_memory_o &&
      fixed_cycles_o <= POLY_CYCLE_FEW_CYCLE_LIMIT;
  end
endmodule
