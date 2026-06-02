// Native return-cookie recovery for Poly frontend transitions.
//
// Ordinary native return instructions stay untouched. If the return target is
// the reserved Poly return cookie, hardware must pop a transition frame and
// restore the caller frontend, PC, and SP. Result register remapping is handled
// elsewhere by ABI signature/return-kind logic.
module poly_return_cookie_recover (
    input  logic        valid_i,
    input  logic [1:0]  current_frontend_i,
    input  logic [63:0] return_target_pc_i,
    input  logic        transition_empty_i,
    input  logic [1:0]  pop_frontend_i,
    input  logic [63:0] pop_pc_i,
    input  logic [63:0] pop_sp_i,
    input  logic [31:0] pop_flags_i,

    output logic        cookie_hit_o,
    output logic        pop_transition_o,
    output logic        resume_o,
    output logic [1:0]  resume_frontend_o,
    output logic [63:0] resume_pc_o,
    output logic [63:0] resume_sp_o,
    output logic [31:0] resume_flags_o,

    output logic        error_o,
    output logic        invalid_frontend_o,
    output logic        missing_transition_o
);
  localparam logic [1:0] POLY_FRONTEND_X86     = 2'd0;
  localparam logic [1:0] POLY_FRONTEND_AARCH64 = 2'd1;
  localparam logic [1:0] POLY_FRONTEND_RISCV   = 2'd2;
  localparam logic [63:0] POLY_RETURN_COOKIE = 64'hfffffffffffff000;

  logic frontend_valid;
  logic pop_frontend_valid;

  always_comb begin
    frontend_valid =
      current_frontend_i == POLY_FRONTEND_X86 ||
      current_frontend_i == POLY_FRONTEND_AARCH64 ||
      current_frontend_i == POLY_FRONTEND_RISCV;
    pop_frontend_valid =
      pop_frontend_i == POLY_FRONTEND_X86 ||
      pop_frontend_i == POLY_FRONTEND_AARCH64 ||
      pop_frontend_i == POLY_FRONTEND_RISCV;

    cookie_hit_o = valid_i && return_target_pc_i == POLY_RETURN_COOKIE;
    invalid_frontend_o =
      cookie_hit_o && (!frontend_valid || !pop_frontend_valid);
    missing_transition_o = cookie_hit_o && transition_empty_i;
    error_o = invalid_frontend_o || missing_transition_o;

    pop_transition_o = cookie_hit_o && !error_o;
    resume_o = pop_transition_o;
    resume_frontend_o = resume_o ? pop_frontend_i : current_frontend_i;
    resume_pc_o = resume_o ? pop_pc_i : return_target_pc_i;
    resume_sp_o = resume_o ? pop_sp_i : 64'd0;
    resume_flags_o = resume_o ? pop_flags_i : 32'd0;
  end
endmodule
