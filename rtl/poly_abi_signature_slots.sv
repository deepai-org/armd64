// Cached ABI signature slots for register-only Poly PCALL.
//
// A slot contains only a signature kind and a register-map id. Hardware may use
// this metadata to steer rename/RAT aliasing. It must not parse user-memory
// descriptors, rewrite stacks, or perform memory-shaped ABI translation.
module poly_abi_signature_slots #(
    parameter int SLOT_COUNT = 13
) (
    input  logic        clk_i,
    input  logic        rst_ni,

    input  logic        set_i,
    input  logic [3:0]  set_slot_i,
    input  logic [7:0]  set_kind_i,
    input  logic [31:0] set_map_i,
    output logic        set_ok_o,
    output logic        set_error_o,

    input  logic [3:0]  select_slot_i,
    output logic        select_valid_o,
    output logic [7:0]  select_kind_o,
    output logic [6:0]  select_map_o,
    output logic        select_tls_base_o
);
  localparam logic [3:0] SLOT_COUNT_VALUE = 4'd13;
  localparam logic [7:0] POLY_ABI_SIGNATURE_KIND_LAST_VALID = 8'd28;
  localparam logic [6:0] POLY_ABI_REGISTER_MAP_LAST_VALID = 7'd27;
  localparam logic [31:0] POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE = 32'h80000000;

  logic [7:0] kind_q [SLOT_COUNT];
  logic [31:0] map_q [SLOT_COUNT];

  function automatic logic valid_slot(input logic [3:0] slot);
    begin
      valid_slot = slot < SLOT_COUNT_VALUE;
    end
  endfunction

  function automatic logic valid_kind(input logic [7:0] kind);
    begin
      valid_kind = kind <= POLY_ABI_SIGNATURE_KIND_LAST_VALID;
    end
  endfunction

  function automatic logic valid_map(input logic [31:0] map);
    logic [31:0] map_without_tls;
    begin
      map_without_tls = map & ~POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE;
      valid_map = map_without_tls[31:7] == 25'd0 &&
        map_without_tls[6:0] <= POLY_ABI_REGISTER_MAP_LAST_VALID;
    end
  endfunction

  always_comb begin
    select_valid_o = valid_slot(select_slot_i);
    if (select_valid_o) begin
      select_kind_o = kind_q[select_slot_i];
      select_map_o = map_q[select_slot_i][6:0];
      select_tls_base_o =
        (map_q[select_slot_i] & POLY_ABI_REGISTER_MAP_FLAG_TLS_BASE) != 32'd0;
    end
    else begin
      select_kind_o = 8'd0;
      select_map_o = 7'd0;
      select_tls_base_o = 1'b0;
    end
  end

  always_ff @(posedge clk_i or negedge rst_ni) begin
    if (!rst_ni) begin
      kind_q[0] <= 8'd0;
      map_q[0] <= 32'd0;
      kind_q[1] <= 8'd2;
      map_q[1] <= 32'd1;
      kind_q[2] <= 8'd3;
      map_q[2] <= 32'd2;
      kind_q[3] <= 8'd4;
      map_q[3] <= 32'd3;
      kind_q[4] <= 8'd5;
      map_q[4] <= 32'd4;
      kind_q[5] <= 8'd6;
      map_q[5] <= 32'd5;
      kind_q[6] <= 8'd7;
      map_q[6] <= 32'd6;
      kind_q[7] <= 8'd8;
      map_q[7] <= 32'd7;
      kind_q[8] <= 8'd9;
      map_q[8] <= 32'd8;
      kind_q[9] <= 8'd10;
      map_q[9] <= 32'd9;
      kind_q[10] <= 8'd11;
      map_q[10] <= 32'd10;
      kind_q[11] <= 8'd12;
      map_q[11] <= 32'd11;
      kind_q[12] <= 8'd26;
      map_q[12] <= 32'd25;
      set_ok_o <= 1'b0;
      set_error_o <= 1'b0;
    end
    else begin
      set_ok_o <= 1'b0;
      set_error_o <= 1'b0;
      if (set_i) begin
        if (!valid_slot(set_slot_i) ||
            !valid_kind(set_kind_i) ||
            !valid_map(set_map_i)) begin
          set_error_o <= 1'b1;
        end
        else begin
          kind_q[set_slot_i] <= set_kind_i;
          map_q[set_slot_i] <= set_map_i;
          set_ok_o <= 1'b1;
        end
      end
    end
  end
endmodule
