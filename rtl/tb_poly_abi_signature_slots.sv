`timescale 1ns/1ps

module tb_poly_abi_signature_slots;
  localparam int SLOT_COUNT = 13;
  localparam logic [7:0] KIND_LAST_VALID = 8'd28;
  localparam logic [6:0] MAP_LAST_VALID = 7'd27;
  localparam logic [31:0] MAP_FLAG_TLS_BASE = 32'h80000000;

  logic clk;
  logic rst_n;
  logic set;
  logic [3:0] set_slot;
  logic [7:0] set_kind;
  logic [31:0] set_map;
  logic set_ok;
  logic set_error;
  logic [3:0] select_slot;
  logic select_valid;
  logic [7:0] select_kind;
  logic [6:0] select_map;
  logic select_tls_base;

  poly_abi_signature_slots #(
    .SLOT_COUNT(SLOT_COUNT)
  ) dut (
    .clk_i(clk),
    .rst_ni(rst_n),
    .set_i(set),
    .set_slot_i(set_slot),
    .set_kind_i(set_kind),
    .set_map_i(set_map),
    .set_ok_o(set_ok),
    .set_error_o(set_error),
    .select_slot_i(select_slot),
    .select_valid_o(select_valid),
    .select_kind_o(select_kind),
    .select_map_o(select_map),
    .select_tls_base_o(select_tls_base)
  );

  always #5 clk = ~clk;

  function automatic logic [7:0] default_kind(input int slot);
    begin
      case (slot)
        0: default_kind = 8'd0;
        1: default_kind = 8'd2;
        2: default_kind = 8'd3;
        3: default_kind = 8'd4;
        4: default_kind = 8'd5;
        5: default_kind = 8'd6;
        6: default_kind = 8'd7;
        7: default_kind = 8'd8;
        8: default_kind = 8'd9;
        9: default_kind = 8'd10;
        10: default_kind = 8'd11;
        11: default_kind = 8'd12;
        default: default_kind = 8'd26;
      endcase
    end
  endfunction

  function automatic logic [6:0] default_map(input int slot);
    begin
      case (slot)
        0: default_map = 7'd0;
        1: default_map = 7'd1;
        2: default_map = 7'd2;
        3: default_map = 7'd3;
        4: default_map = 7'd4;
        5: default_map = 7'd5;
        6: default_map = 7'd6;
        7: default_map = 7'd7;
        8: default_map = 7'd8;
        9: default_map = 7'd9;
        10: default_map = 7'd10;
        11: default_map = 7'd11;
        default: default_map = 7'd25;
      endcase
    end
  endfunction

  task automatic clear_inputs;
    begin
      set = 1'b0;
      set_slot = 4'd0;
      set_kind = 8'd0;
      set_map = 32'd0;
      select_slot = 4'd0;
    end
  endtask

  task automatic check(input logic condition, input [1023:0] message);
    begin
      if (!condition) begin
        $display("FAIL: %0s", message);
        $fatal;
      end
    end
  endtask

  task automatic select_and_check(
      input int slot,
      input logic valid,
      input logic [7:0] kind,
      input logic [6:0] map,
      input logic tls_base
  );
    begin
      select_slot = 4'(slot);
      #1;
      check(select_valid == valid, "select valid");
      check(select_kind == kind, "select kind");
      check(select_map == map, "select map");
      check(select_tls_base == tls_base, "select tls base");
    end
  endtask

  task automatic set_and_check(
      input int slot,
      input logic [7:0] kind,
      input logic [31:0] map,
      input logic expect_ok,
      input logic expect_error
  );
    begin
      @(negedge clk);
      set = 1'b1;
      set_slot = 4'(slot);
      set_kind = kind;
      set_map = map;
      @(posedge clk);
      #1;
      check(set_ok == expect_ok, "set ok");
      check(set_error == expect_error, "set error");
      set = 1'b0;
    end
  endtask

  initial begin
    clk = 1'b0;
    rst_n = 1'b1;
    clear_inputs();

    #1 rst_n = 1'b0;
    #2;
    check(!set_ok && !set_error, "reset clears set status");

    #12 rst_n = 1'b1;
    @(negedge clk);

    for (int i = 0; i < SLOT_COUNT; i++) begin
      select_and_check(i, 1'b1, default_kind(i), default_map(i), 1'b0);
    end
    select_and_check(SLOT_COUNT, 1'b0, 8'd0, 7'd0, 1'b0);

    set_and_check(3, 8'd4, 32'd3 | MAP_FLAG_TLS_BASE, 1'b1, 1'b0);
    select_and_check(3, 1'b1, 8'd4, 7'd3, 1'b1);

    set_and_check(4, KIND_LAST_VALID, MAP_LAST_VALID, 1'b1, 1'b0);
    select_and_check(4, 1'b1, KIND_LAST_VALID, MAP_LAST_VALID, 1'b0);

    set_and_check(SLOT_COUNT, 8'd0, 32'd0, 1'b0, 1'b1);
    select_and_check(3, 1'b1, 8'd4, 7'd3, 1'b1);

    set_and_check(3, KIND_LAST_VALID + 8'd1, 32'd0, 1'b0, 1'b1);
    select_and_check(3, 1'b1, 8'd4, 7'd3, 1'b1);

    set_and_check(3, 8'd0, 32'h00000080, 1'b0, 1'b1);
    select_and_check(3, 1'b1, 8'd4, 7'd3, 1'b1);

    set_and_check(3, 8'd0, 32'd28, 1'b0, 1'b1);
    select_and_check(3, 1'b1, 8'd4, 7'd3, 1'b1);

    set_and_check(3, 8'd0, MAP_FLAG_TLS_BASE | 32'd2, 1'b1, 1'b0);
    select_and_check(3, 1'b1, 8'd0, 7'd2, 1'b1);

    $display("POLY_RTL_ABI_SIGNATURE_SLOTS_SIM_OK");
    $finish;
  end
endmodule
