// Hardware transition stack for Poly PCALL/native-return-cookie bring-up.
//
// The stack stores caller frontend, caller PC, caller SP, and flags. It is a
// fixed-depth architectural structure; overflow, underflow, and same-cycle
// push/pop conflicts are precise errors rather than hidden recovery paths.
module poly_transition_stack #(
    parameter int DEPTH = 8
) (
    input  logic        clk_i,
    input  logic        rst_ni,

    input  logic        push_i,
    input  logic [1:0]  push_frontend_i,
    input  logic [63:0] push_pc_i,
    input  logic [63:0] push_sp_i,
    input  logic [31:0] push_flags_i,

    input  logic        pop_i,
    output logic        pop_valid_o,
    output logic [1:0]  pop_frontend_o,
    output logic [63:0] pop_pc_o,
    output logic [63:0] pop_sp_o,
    output logic [31:0] pop_flags_o,

    output logic        peek_valid_o,
    output logic [1:0]  peek_frontend_o,
    output logic [63:0] peek_pc_o,
    output logic [63:0] peek_sp_o,
    output logic [31:0] peek_flags_o,

    output logic        empty_o,
    output logic        full_o,
    output logic        overflow_o,
    output logic        underflow_o,
    output logic        conflict_o,
    output logic [3:0]  depth_o
);
  localparam int DEPTH_BITS = 4;
  localparam int INDEX_BITS = DEPTH <= 2 ? 1 : $clog2(DEPTH);
  localparam logic [DEPTH_BITS-1:0] DEPTH_VALUE = DEPTH_BITS'(DEPTH);

  logic [1:0]  frontend_q [DEPTH];
  logic [63:0] pc_q       [DEPTH];
  logic [63:0] sp_q       [DEPTH];
  logic [31:0] flags_q    [DEPTH];
  logic [DEPTH_BITS-1:0] depth_q;
  logic [INDEX_BITS-1:0] top_index;
  logic [INDEX_BITS-1:0] push_index;

  assign empty_o = depth_q == '0;
  assign full_o = depth_q == DEPTH_VALUE;
  assign depth_o = depth_q;
  assign top_index = depth_q[INDEX_BITS-1:0] - {{(INDEX_BITS-1){1'b0}}, 1'b1};
  assign push_index = depth_q[INDEX_BITS-1:0];

  always_comb begin
    peek_valid_o = !empty_o;
    if (empty_o) begin
      peek_frontend_o = 2'd0;
      peek_pc_o = 64'd0;
      peek_sp_o = 64'd0;
      peek_flags_o = 32'd0;
    end
    else begin
      peek_frontend_o = frontend_q[top_index];
      peek_pc_o = pc_q[top_index];
      peek_sp_o = sp_q[top_index];
      peek_flags_o = flags_q[top_index];
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      depth_q <= '0;
      pop_valid_o <= 1'b0;
      pop_frontend_o <= 2'd0;
      pop_pc_o <= 64'd0;
      pop_sp_o <= 64'd0;
      pop_flags_o <= 32'd0;
      overflow_o <= 1'b0;
      underflow_o <= 1'b0;
      conflict_o <= 1'b0;
    end
    else begin
      pop_valid_o <= 1'b0;
      overflow_o <= 1'b0;
      underflow_o <= 1'b0;
      conflict_o <= 1'b0;

      if (push_i && pop_i) begin
        conflict_o <= 1'b1;
      end
      else if (push_i) begin
        if (full_o) begin
          overflow_o <= 1'b1;
        end
        else begin
          frontend_q[push_index] <= push_frontend_i;
          pc_q[push_index] <= push_pc_i;
          sp_q[push_index] <= push_sp_i;
          flags_q[push_index] <= push_flags_i;
          depth_q <= depth_q + 1'b1;
        end
      end
      else if (pop_i) begin
        if (empty_o) begin
          underflow_o <= 1'b1;
        end
        else begin
          pop_valid_o <= 1'b1;
          pop_frontend_o <= frontend_q[top_index];
          pop_pc_o <= pc_q[top_index];
          pop_sp_o <= sp_q[top_index];
          pop_flags_o <= flags_q[top_index];
          depth_q <= depth_q - 1'b1;
        end
      end
    end
  end
endmodule
