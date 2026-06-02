// Formal harness for transition-stack and return-cookie ordering.
//
// The transition stack is the hardware return state for cross-ISA PCALL. Native
// return-cookie recovery must resume from the current stack top and must not
// synthesize a resume on empty, invalid, or non-cookie targets.
module poly_transition_stack_return_formal (
    input logic clk
);
`ifdef FORMAL
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;
  localparam logic [63:0] POLY_RETURN_COOKIE = 64'hfffffffffffff000;
  localparam logic [3:0] FORMAL_DEPTH = 4'd2;

  (* anyseq *) logic        rst_ni;
  (* anyseq *) logic        push;
  (* anyseq *) logic [1:0]  push_frontend;
  (* anyseq *) logic [63:0] push_pc;
  (* anyseq *) logic [63:0] push_sp;
  (* anyseq *) logic [31:0] push_flags;
  (* anyseq *) logic        pop;
  (* anyseq *) logic        return_valid;
  (* anyseq *) logic [1:0]  current_frontend;
  (* anyseq *) logic [63:0] return_target_pc;

  logic past_valid;
  logic pop_valid;
  logic [1:0] pop_frontend;
  logic [63:0] pop_pc;
  logic [63:0] pop_sp;
  logic [31:0] pop_flags;
  logic peek_valid;
  logic [1:0] peek_frontend;
  logic [63:0] peek_pc;
  logic [63:0] peek_sp;
  logic [31:0] peek_flags;
  logic empty;
  logic full;
  logic overflow;
  logic underflow;
  logic conflict;
  logic [3:0] depth;

  logic cookie_hit;
  logic pop_transition;
  logic resume;
  logic [1:0] resume_frontend;
  logic [63:0] resume_pc;
  logic [63:0] resume_sp;
  logic [31:0] resume_flags;
  logic recover_error;
  logic invalid_frontend;
  logic missing_transition;

  logic current_frontend_valid;
  logic peek_frontend_valid;

  poly_transition_stack #(
    .DEPTH(2)
  ) transition_stack (
    .clk_i(clk),
    .rst_ni(rst_ni),
    .push_i(push),
    .push_frontend_i(push_frontend),
    .push_pc_i(push_pc),
    .push_sp_i(push_sp),
    .push_flags_i(push_flags),
    .pop_i(pop),
    .pop_valid_o(pop_valid),
    .pop_frontend_o(pop_frontend),
    .pop_pc_o(pop_pc),
    .pop_sp_o(pop_sp),
    .pop_flags_o(pop_flags),
    .peek_valid_o(peek_valid),
    .peek_frontend_o(peek_frontend),
    .peek_pc_o(peek_pc),
    .peek_sp_o(peek_sp),
    .peek_flags_o(peek_flags),
    .empty_o(empty),
    .full_o(full),
    .overflow_o(overflow),
    .underflow_o(underflow),
    .conflict_o(conflict),
    .depth_o(depth)
  );

  poly_return_cookie_recover return_cookie_recover (
    .valid_i(return_valid),
    .current_frontend_i(current_frontend),
    .return_target_pc_i(return_target_pc),
    .transition_empty_i(!peek_valid),
    .pop_frontend_i(peek_frontend),
    .pop_pc_i(peek_pc),
    .pop_sp_i(peek_sp),
    .pop_flags_i(peek_flags),
    .cookie_hit_o(cookie_hit),
    .pop_transition_o(pop_transition),
    .resume_o(resume),
    .resume_frontend_o(resume_frontend),
    .resume_pc_o(resume_pc),
    .resume_sp_o(resume_sp),
    .resume_flags_o(resume_flags),
    .error_o(recover_error),
    .invalid_frontend_o(invalid_frontend),
    .missing_transition_o(missing_transition)
  );

  always_comb begin
    current_frontend_valid =
      current_frontend == POLY_FRONTEND_X86 ||
      current_frontend == POLY_FRONTEND_AARCH64 ||
      current_frontend == POLY_FRONTEND_RISCV;
    peek_frontend_valid =
      peek_frontend == POLY_FRONTEND_X86 ||
      peek_frontend == POLY_FRONTEND_AARCH64 ||
      peek_frontend == POLY_FRONTEND_RISCV;
  end

  initial past_valid = 1'b0;

  always_ff @(posedge clk) begin
    past_valid <= 1'b1;

    if (!past_valid)
      assume (!rst_ni);
    else
      assume (rst_ni);

    if (rst_ni) begin
      assert (depth <= FORMAL_DEPTH);
      assert (empty == (depth == 4'd0));
      assert (full == (depth == FORMAL_DEPTH));
      assert (peek_valid == !empty);
      assert (!(overflow && underflow));
      assert (!(conflict && (overflow || underflow || pop_valid)));

      assert (cookie_hit ==
        (return_valid && return_target_pc == POLY_RETURN_COOKIE));
      assert (missing_transition == (cookie_hit && !peek_valid));
      assert (invalid_frontend ==
        (cookie_hit && (!current_frontend_valid || !peek_frontend_valid)));
      assert (recover_error == (missing_transition || invalid_frontend));
      assert (pop_transition == (cookie_hit && !recover_error));
      assert (resume == pop_transition);

      if (!cookie_hit) begin
        assert (!pop_transition && !resume && !recover_error);
        assert (resume_frontend == current_frontend);
        assert (resume_pc == return_target_pc);
        assert (resume_sp == 64'd0);
        assert (resume_flags == 32'd0);
      end

      if (cookie_hit && !recover_error) begin
        assert (peek_valid);
        assert (current_frontend_valid && peek_frontend_valid);
        assert (resume_frontend == peek_frontend);
        assert (resume_pc == peek_pc);
        assert (resume_sp == peek_sp);
        assert (resume_flags == peek_flags);
      end

      if (recover_error)
        assert (!pop_transition && !resume);
    end

    if (past_valid && $past(rst_ni) && rst_ni) begin
      if ($past(push && pop)) begin
        assert (conflict);
        assert (!pop_valid && !overflow && !underflow);
        assert (depth == $past(depth));
      end
      else if ($past(push && !pop && full)) begin
        assert (overflow);
        assert (!pop_valid && !underflow && !conflict);
        assert (depth == $past(depth));
      end
      else if ($past(push && !pop && !full)) begin
        assert (!overflow && !underflow && !conflict && !pop_valid);
        assert (depth == $past(depth) + 4'd1);
        assert (peek_valid);
        assert (peek_frontend == $past(push_frontend));
        assert (peek_pc == $past(push_pc));
        assert (peek_sp == $past(push_sp));
        assert (peek_flags == $past(push_flags));
      end
      else if ($past(pop && !push && empty)) begin
        assert (underflow);
        assert (!pop_valid && !overflow && !conflict);
        assert (depth == $past(depth));
      end
      else if ($past(pop && !push && !empty)) begin
        assert (pop_valid);
        assert (!overflow && !underflow && !conflict);
        assert (depth == $past(depth) - 4'd1);
        assert (pop_frontend == $past(peek_frontend));
        assert (pop_pc == $past(peek_pc));
        assert (pop_sp == $past(peek_sp));
        assert (pop_flags == $past(peek_flags));
      end
      else if ($past(!push && !pop)) begin
        assert (!pop_valid && !overflow && !underflow && !conflict);
        assert (depth == $past(depth));
      end
    end
  end
`endif
endmodule
