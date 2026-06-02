// Poly architectural frontend/PC state register.
//
// This is the state-update boundary for the frontend-switch prototype. It does
// not fetch or decode instructions; it validates and commits the next
// architectural frontend/PC selected by retirement, interrupt return, or native
// return-cookie recovery.
module poly_frontend_state (
    input  logic        clk_i,
    input  logic        rst_ni,

    input  logic        init_i,
    input  logic [1:0]  init_frontend_i,
    input  logic [63:0] init_pc_i,

    input  logic        commit_i,
    input  logic [1:0]  commit_frontend_i,
    input  logic [63:0] commit_pc_i,

    input  logic        interrupt_restore_i,
    input  logic [1:0]  interrupt_frontend_i,
    input  logic [63:0] interrupt_pc_i,

    input  logic        return_resume_i,
    input  logic [1:0]  return_frontend_i,
    input  logic [63:0] return_pc_i,

    input  logic        fault_i,
    input  logic        stall_i,

    output logic [1:0]  current_frontend_o,
    output logic [63:0] current_pc_o,
    output logic        redirect_valid_o,
    output logic [1:0]  redirect_frontend_o,
    output logic [63:0] redirect_pc_o,
    output logic [2:0]  redirect_reason_o,
    output logic        update_o,
    output logic        hold_o,
    output logic        conflict_o,
    output logic        invalid_frontend_o,
    output logic        invalid_pc_o,
    output logic        error_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;
  localparam logic [2:0] POLY_REDIRECT_NONE      = 3'd0;
  localparam logic [2:0] POLY_REDIRECT_INIT      = 3'd1;
  localparam logic [2:0] POLY_REDIRECT_COMMIT    = 3'd2;
  localparam logic [2:0] POLY_REDIRECT_INTERRUPT = 3'd3;
  localparam logic [2:0] POLY_REDIRECT_RETURN    = 3'd4;

  logic [1:0] frontend_q;
  logic [63:0] pc_q;
  logic [1:0] selected_frontend;
  logic [63:0] selected_pc;
  logic [2:0] selected_reason;
  logic request_valid;
  logic multiple_requests;

  assign current_frontend_o = frontend_q;
  assign current_pc_o = pc_q;

  function automatic logic canonical64(input logic [63:0] addr);
    begin
      canonical64 = addr[63:48] == {16{addr[47]}};
    end
  endfunction

  function automatic logic frontend_valid(input logic [1:0] frontend);
    begin
      frontend_valid =
        frontend == POLY_FRONTEND_X86 ||
        frontend == POLY_FRONTEND_AARCH64 ||
        frontend == POLY_FRONTEND_RISCV;
    end
  endfunction

  function automatic logic frontend_aligned(
      input logic [1:0] frontend,
      input logic [63:0] addr
  );
    begin
      unique case (frontend)
        POLY_FRONTEND_AARCH64: frontend_aligned = addr[1:0] == 2'b00;
        POLY_FRONTEND_RISCV: frontend_aligned = addr[0] == 1'b0;
        default: frontend_aligned = 1'b1;
      endcase
    end
  endfunction

  always_comb begin
    multiple_requests =
      (commit_i && interrupt_restore_i) ||
      (commit_i && return_resume_i) ||
      (interrupt_restore_i && return_resume_i);
    conflict_o = !init_i && multiple_requests;

    if (init_i) begin
      selected_frontend = init_frontend_i;
      selected_pc = init_pc_i;
      selected_reason = POLY_REDIRECT_INIT;
    end
    else if (return_resume_i) begin
      selected_frontend = return_frontend_i;
      selected_pc = return_pc_i;
      selected_reason = POLY_REDIRECT_RETURN;
    end
    else if (interrupt_restore_i) begin
      selected_frontend = interrupt_frontend_i;
      selected_pc = interrupt_pc_i;
      selected_reason = POLY_REDIRECT_INTERRUPT;
    end
    else if (commit_i) begin
      selected_frontend = commit_frontend_i;
      selected_pc = commit_pc_i;
      selected_reason = POLY_REDIRECT_COMMIT;
    end
    else begin
      selected_frontend = frontend_q;
      selected_pc = pc_q;
      selected_reason = POLY_REDIRECT_NONE;
    end

    request_valid = init_i || commit_i || interrupt_restore_i || return_resume_i;
    invalid_frontend_o =
      request_valid && !conflict_o && !frontend_valid(selected_frontend);
    invalid_pc_o =
      request_valid && !conflict_o &&
      (!canonical64(selected_pc) ||
        !frontend_aligned(selected_frontend, selected_pc));
    error_o = conflict_o || invalid_frontend_o || invalid_pc_o;

    update_o =
      request_valid && !fault_i && !stall_i && !error_o;
    hold_o = !update_o;
    redirect_valid_o = update_o;
    redirect_frontend_o = update_o ? selected_frontend : frontend_q;
    redirect_pc_o = update_o ? selected_pc : pc_q;
    redirect_reason_o = update_o ? selected_reason : POLY_REDIRECT_NONE;
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      frontend_q <= POLY_FRONTEND_X86;
      pc_q <= 64'd0;
    end
    else if (update_o) begin
      frontend_q <= selected_frontend;
      pc_q <= selected_pc;
    end
  end
endmodule
