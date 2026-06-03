# FPGA bring-up timing constraints for poly_frontend_fpga_top.
#
# This file is an integration contract for downstream FPGA place-and-route. It
# is not a timing-closure result by itself; device, package, board pins, and a
# real P&R report are still required before silicon or board signoff.

create_clock -name poly_core_clk -period 10.000 [get_ports clk_i]
set_clock_uncertainty 0.250 [get_clocks poly_core_clk]

# rst_ni is an asynchronous board/reset-controller input. Synchronization and
# reset release policy are owned by the integration wrapper around this top.
set_false_path -from [get_ports rst_ni]

set poly_data_inputs [remove_from_collection [all_inputs] [get_ports {clk_i rst_ni}]]
set_input_delay -clock poly_core_clk 2.000 $poly_data_inputs
set_output_delay -clock poly_core_clk 2.000 [all_outputs]
