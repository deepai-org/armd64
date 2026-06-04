IMAGE ?= armd64-bochs
POLY_XCR0_MODULE ?= out/poly_xcr0.ko
BOOT_TIMEOUT_SECONDS ?= 300
BOOT_FOCUSED_TIMEOUT_SECONDS ?= 900
BOOT_DETAIL_ASSERTS ?= 1
BOOT_DOCKER_ENV = -e BOOT_TIMEOUT_SECONDS=$(BOOT_TIMEOUT_SECONDS) -e BOOT_DETAIL_ASSERTS=$(BOOT_DETAIL_ASSERTS)
POLY_RTL_TOP ?= poly_frontend_fpga_top
POLY_RTL_XDC ?= rtl/poly_frontend_fpga_top.xdc
POLY_RTL_FPGA_OUT ?= out/rtl
POLY_RTL_FPGA_EDIF = $(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).edif
POLY_RTL_FPGA_XDC = $(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).xdc
POLY_RTL_FPGA_MANIFEST = $(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).manifest
POLY_RTL_VERILATOR_FLAGS = --lint-only --Wall \
	-Wno-PINCONNECTEMPTY -Wno-UNUSEDSIGNAL -Wno-UNUSEDPARAM \
	--top-module $(POLY_RTL_TOP)
POLY_RTL_SV = \
	rtl/poly_abi_signature_slots.sv \
	rtl/poly_cpuid_rom.sv \
	rtl/poly_ctrl_decode.sv \
	rtl/poly_frontend_core.sv \
	rtl/poly_frontend_decode_dispatch.sv \
	rtl/poly_frontend_fetch_decode_pipeline.sv \
	rtl/poly_frontend_fetch_issue.sv \
	rtl/poly_frontend_fpga_top.sv \
	rtl/poly_frontend_handoff.sv \
	rtl/poly_frontend_memory_retire.sv \
	rtl/poly_frontend_predecoded_retire.sv \
	rtl/poly_frontend_retire.sv \
	rtl/poly_frontend_state.sv \
	rtl/poly_frontend_stateful_core.sv \
	rtl/poly_frontend_step.sv \
	rtl/poly_interrupt_boundary.sv \
	rtl/poly_memory_order.sv \
	rtl/poly_raw_data_mem_request.sv \
	rtl/poly_raw_data_mem_response_stage.sv \
	rtl/poly_raw_insn_decode.sv \
	rtl/poly_raw_fetch_plan.sv \
	rtl/poly_raw_fetch_request.sv \
	rtl/poly_raw_fetch_response_stage.sv \
	rtl/poly_raw_fetch_stage.sv \
	rtl/poly_return_cookie_recover.sv \
	rtl/poly_transition_cycle_budget.sv \
	rtl/poly_transition_stack.sv \
	rtl/poly_trap_packet_encode.sv \
	rtl/poly_trap_packet_stage.sv \
	rtl/poly_x86_fetch_stage.sv

.PHONY: image poly-xcr0-module poly-rtl-fpga-artifacts check-poly-import-ids check-poly-isa-readiness check-poly-arch-contract check-poly-cpuid-contract check-poly-state-layout check-poly-abi-legacy-bridge check-poly-bridge-ir check-poly-contracts check-poly-rtl check-poly-rtl-sim check-poly-rtl-formal check-poly-rtl-constraints check-poly-rtl-fpga-artifacts check-poly-rtl-verilator check-poly-rtl-yosys check-poly-rtl-synth check-poly-rtl-fpga check-poly-rtl-fpga-resources check-poly-rtl-hdl boot boot-poly boot-poly-network-smoke boot-poly-arch-traps boot-poly-nativecheck-arch-traps boot-poly-real-xsave-arch-traps boot-poly-probe-arch-traps boot-poly-apps-arch-traps boot-poly-neutral-arch-traps boot-poly-exec-arch-traps boot-poly-exec-cross-arch-traps boot-poly-exec-syscall-arch-traps boot-poly-call-arch-traps boot-poly-call-real-xsave-arch-traps boot-poly-thread-arch-traps boot-poly-bench-arch-traps boot-poly-binfmt-arch-traps boot-poly-focused-validation boot-poly-full-arch-traps boot-poly-full-real-xsave-arch-traps boot-poly-full clean

image:
	docker build --platform=linux/arm64 -t $(IMAGE) .

poly-xcr0-module: $(POLY_XCR0_MODULE)

$(POLY_XCR0_MODULE): tools/kernel/poly_xcr0.c scripts/build_poly_xcr0_module.sh
	./scripts/build_poly_xcr0_module.sh

check-poly-import-ids:
	./scripts/checks/check_poly_import_ids.sh

check-poly-isa-readiness:
	./scripts/checks/check_poly_isa_readiness.sh

check-poly-arch-contract:
	./scripts/checks/check_poly_arch_contract.sh

check-poly-cpuid-contract:
	./scripts/checks/check_poly_cpuid_contract.sh

check-poly-state-layout:
	tmp_dir=$$(mktemp -d); \
	trap 'rm -rf "$$tmp_dir"' EXIT; \
	$(CC) -std=gnu11 -Wall -Wextra -Werror -Itools/include \
		tools/programs/polylayout.c -o "$$tmp_dir/polylayout"; \
	"$$tmp_dir/polylayout" --check

check-poly-abi-legacy-bridge:
	tmp_dir=$$(mktemp -d); \
	trap 'rm -rf "$$tmp_dir"' EXIT; \
	$(CC) -std=gnu11 -Wall -Wextra -Werror \
		-Itools/include -Itools/runtime \
		tools/tests/poly_abi_legacy_bridge_test.c \
		tools/runtime/abi/poly_abi_legacy_bridge.c \
		-o "$$tmp_dir/poly_abi_legacy_bridge_test"; \
	"$$tmp_dir/poly_abi_legacy_bridge_test"

check-poly-bridge-ir:
	tmp_dir=$$(mktemp -d); \
	trap 'rm -rf "$$tmp_dir"' EXIT; \
	$(CC) -std=gnu11 -Wall -Wextra -Werror \
		-Itools/include -Itools/runtime \
		tools/tests/poly_bridge_ir_test.c \
		tools/runtime/bridge/poly_bridge_plan.c \
		-o "$$tmp_dir/poly_bridge_ir_test"; \
	"$$tmp_dir/poly_bridge_ir_test"

check-poly-contracts: check-poly-import-ids check-poly-isa-readiness check-poly-arch-contract check-poly-cpuid-contract check-poly-state-layout check-poly-abi-legacy-bridge check-poly-bridge-ir

check-poly-rtl:
	python3 rtl/test_poly_ctrl_decode.py
	python3 rtl/test_poly_frontend_handoff.py
	python3 rtl/test_poly_frontend_decode_dispatch.py
	python3 rtl/test_poly_frontend_fetch_decode_pipeline.py
	python3 rtl/test_poly_frontend_step.py
	python3 rtl/test_poly_frontend_retire.py
	python3 rtl/test_poly_frontend_predecoded_retire.py
	python3 rtl/test_poly_frontend_fetch_issue.py
	python3 rtl/test_poly_frontend_memory_retire.py
	python3 rtl/test_poly_frontend_core.py
	python3 rtl/test_poly_frontend_state.py
	python3 rtl/test_poly_frontend_stateful_core.py
	python3 rtl/test_poly_frontend_fpga_top.py
	python3 rtl/test_poly_interrupt_boundary.py
	python3 rtl/test_poly_transition_stack.py
	python3 rtl/test_poly_abi_signature_slots.py
	python3 rtl/test_poly_cpuid_rom.py
	python3 rtl/test_poly_memory_order.py
	python3 rtl/test_poly_memory_order_formal.py
	python3 rtl/test_poly_memory_order_litmus.py
	python3 rtl/test_poly_raw_data_mem_request.py
	python3 rtl/test_poly_raw_data_mem_response_stage.py
	python3 rtl/test_poly_raw_insn_decode.py
	python3 rtl/test_poly_x86_fetch_stage.py
	python3 rtl/test_poly_raw_fetch_request.py
	python3 rtl/test_poly_raw_fetch_response_stage.py
	python3 rtl/test_poly_raw_fetch_stage.py
	python3 rtl/test_poly_raw_fetch_plan.py
	python3 rtl/test_poly_return_cookie_recover.py
	python3 rtl/test_poly_transition_cycle_budget.py
	python3 rtl/test_poly_trap_packet_encode.py
	python3 rtl/test_poly_trap_packet_stage.py
	$(MAKE) check-poly-rtl-formal
	$(MAKE) check-poly-rtl-sim

check-poly-rtl-formal:
	yosys -q -p 'read_verilog -formal -sv -D FORMAL rtl/poly_memory_order.sv rtl/poly_memory_order_formal.sv; prep -top poly_memory_order_formal -flatten; sat -seq 2 -prove-asserts -set-init-zero'
	@echo POLY_RTL_MEMORY_ORDER_FORMAL_PROOF_OK
	yosys -q -p 'read_verilog -formal -sv -D FORMAL rtl/poly_transition_stack.sv rtl/poly_return_cookie_recover.sv rtl/poly_transition_stack_return_formal.sv; prep -top poly_transition_stack_return_formal -flatten; async2sync; dffunmap; sat -seq 4 -prove-asserts -tempinduct -set-init-zero'
	@echo POLY_RTL_TRANSITION_STACK_RETURN_FORMAL_PROOF_OK

check-poly-rtl-sim:
	tmp_dir=$$(mktemp -d); \
	trap 'rm -rf "$$tmp_dir"' EXIT; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_ctrl_decode" \
		rtl/poly_ctrl_decode.sv rtl/tb_poly_ctrl_decode.sv; \
	vvp "$$tmp_dir/tb_poly_ctrl_decode"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_raw_fetch_path" \
		rtl/poly_raw_fetch_plan.sv rtl/poly_raw_fetch_request.sv \
		rtl/poly_raw_fetch_response_stage.sv rtl/tb_poly_raw_fetch_path.sv; \
	vvp "$$tmp_dir/tb_poly_raw_fetch_path"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_raw_data_mem_request" \
		rtl/poly_raw_data_mem_request.sv rtl/tb_poly_raw_data_mem_request.sv; \
	vvp "$$tmp_dir/tb_poly_raw_data_mem_request"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_raw_data_mem_response_stage" \
		rtl/poly_raw_data_mem_response_stage.sv \
		rtl/tb_poly_raw_data_mem_response_stage.sv; \
	vvp "$$tmp_dir/tb_poly_raw_data_mem_response_stage"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_fetch_decode_pipeline" \
		rtl/poly_ctrl_decode.sv rtl/poly_raw_fetch_plan.sv \
		rtl/poly_raw_insn_decode.sv \
		rtl/poly_raw_fetch_request.sv rtl/poly_raw_fetch_response_stage.sv \
		rtl/poly_frontend_fetch_issue.sv rtl/poly_x86_fetch_stage.sv \
		rtl/poly_frontend_decode_dispatch.sv \
		rtl/poly_frontend_fetch_decode_pipeline.sv \
		rtl/tb_poly_frontend_fetch_decode_pipeline.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_fetch_decode_pipeline"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_cpuid_rom" \
		rtl/poly_cpuid_rom.sv rtl/tb_poly_cpuid_rom.sv; \
	vvp "$$tmp_dir/tb_poly_cpuid_rom"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_retire" \
		rtl/poly_ctrl_decode.sv rtl/poly_raw_fetch_plan.sv \
		rtl/poly_raw_insn_decode.sv \
		rtl/poly_frontend_decode_dispatch.sv rtl/poly_frontend_handoff.sv \
		rtl/poly_frontend_step.sv rtl/poly_frontend_retire.sv \
		rtl/tb_poly_frontend_retire.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_retire"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_predecoded_retire" \
		rtl/poly_frontend_handoff.sv rtl/poly_frontend_predecoded_retire.sv \
		rtl/tb_poly_frontend_predecoded_retire.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_predecoded_retire"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_memory_retire" \
		rtl/poly_ctrl_decode.sv rtl/poly_raw_fetch_plan.sv \
		rtl/poly_raw_insn_decode.sv \
		rtl/poly_raw_fetch_request.sv rtl/poly_raw_fetch_response_stage.sv \
		rtl/poly_frontend_fetch_issue.sv rtl/poly_x86_fetch_stage.sv \
		rtl/poly_frontend_decode_dispatch.sv \
		rtl/poly_frontend_fetch_decode_pipeline.sv \
		rtl/poly_frontend_handoff.sv rtl/poly_frontend_predecoded_retire.sv \
		rtl/poly_frontend_memory_retire.sv \
		rtl/tb_poly_frontend_memory_retire.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_memory_retire"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_core" \
		rtl/poly_ctrl_decode.sv rtl/poly_raw_fetch_plan.sv \
		rtl/poly_raw_insn_decode.sv \
		rtl/poly_raw_fetch_request.sv rtl/poly_raw_fetch_response_stage.sv \
		rtl/poly_frontend_fetch_issue.sv rtl/poly_x86_fetch_stage.sv \
		rtl/poly_frontend_decode_dispatch.sv \
		rtl/poly_frontend_fetch_decode_pipeline.sv \
		rtl/poly_frontend_handoff.sv rtl/poly_frontend_predecoded_retire.sv \
		rtl/poly_frontend_memory_retire.sv rtl/poly_memory_order.sv \
		rtl/poly_interrupt_boundary.sv rtl/poly_trap_packet_encode.sv \
		rtl/poly_trap_packet_stage.sv rtl/poly_abi_signature_slots.sv \
		rtl/poly_cpuid_rom.sv rtl/poly_transition_cycle_budget.sv \
		rtl/poly_transition_stack.sv rtl/poly_return_cookie_recover.sv \
		rtl/poly_frontend_core.sv rtl/tb_poly_frontend_core.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_core"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_stateful_core" \
		rtl/poly_ctrl_decode.sv rtl/poly_raw_fetch_plan.sv \
		rtl/poly_raw_insn_decode.sv \
		rtl/poly_raw_fetch_request.sv rtl/poly_raw_fetch_response_stage.sv \
		rtl/poly_frontend_fetch_issue.sv rtl/poly_x86_fetch_stage.sv \
		rtl/poly_frontend_decode_dispatch.sv \
		rtl/poly_frontend_fetch_decode_pipeline.sv \
		rtl/poly_frontend_handoff.sv rtl/poly_frontend_predecoded_retire.sv \
		rtl/poly_frontend_memory_retire.sv rtl/poly_memory_order.sv \
		rtl/poly_interrupt_boundary.sv rtl/poly_trap_packet_encode.sv \
		rtl/poly_trap_packet_stage.sv rtl/poly_abi_signature_slots.sv \
		rtl/poly_cpuid_rom.sv rtl/poly_transition_cycle_budget.sv \
		rtl/poly_transition_stack.sv rtl/poly_return_cookie_recover.sv \
		rtl/poly_frontend_core.sv rtl/poly_frontend_state.sv \
		rtl/poly_frontend_stateful_core.sv \
		rtl/tb_poly_frontend_stateful_core.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_stateful_core"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_fpga_top" \
		rtl/poly_ctrl_decode.sv rtl/poly_raw_fetch_plan.sv \
		rtl/poly_raw_insn_decode.sv \
		rtl/poly_raw_fetch_request.sv rtl/poly_raw_fetch_response_stage.sv \
		rtl/poly_frontend_fetch_issue.sv rtl/poly_x86_fetch_stage.sv \
		rtl/poly_frontend_decode_dispatch.sv \
		rtl/poly_frontend_fetch_decode_pipeline.sv \
		rtl/poly_frontend_handoff.sv rtl/poly_frontend_predecoded_retire.sv \
		rtl/poly_frontend_memory_retire.sv rtl/poly_memory_order.sv \
		rtl/poly_interrupt_boundary.sv rtl/poly_trap_packet_encode.sv \
		rtl/poly_trap_packet_stage.sv rtl/poly_abi_signature_slots.sv \
		rtl/poly_cpuid_rom.sv rtl/poly_transition_cycle_budget.sv \
		rtl/poly_transition_stack.sv rtl/poly_return_cookie_recover.sv \
		rtl/poly_frontend_core.sv rtl/poly_frontend_state.sv \
		rtl/poly_frontend_stateful_core.sv rtl/poly_frontend_fpga_top.sv \
		rtl/poly_raw_data_mem_request.sv \
		rtl/poly_raw_data_mem_response_stage.sv \
		rtl/tb_poly_frontend_fpga_top.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_fpga_top"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_frontend_state" \
		rtl/poly_frontend_state.sv rtl/tb_poly_frontend_state.sv; \
	vvp "$$tmp_dir/tb_poly_frontend_state"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_transition_stack" \
		rtl/poly_transition_stack.sv rtl/tb_poly_transition_stack.sv; \
	vvp "$$tmp_dir/tb_poly_transition_stack"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_abi_signature_slots" \
		rtl/poly_abi_signature_slots.sv rtl/tb_poly_abi_signature_slots.sv; \
	vvp "$$tmp_dir/tb_poly_abi_signature_slots"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_interrupt_boundary" \
		rtl/poly_interrupt_boundary.sv rtl/tb_poly_interrupt_boundary.sv; \
	vvp "$$tmp_dir/tb_poly_interrupt_boundary"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_return_cookie_recover" \
		rtl/poly_return_cookie_recover.sv rtl/tb_poly_return_cookie_recover.sv; \
	vvp "$$tmp_dir/tb_poly_return_cookie_recover"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_trap_packet_stage" \
		rtl/poly_trap_packet_encode.sv rtl/poly_trap_packet_stage.sv \
		rtl/tb_poly_trap_packet_stage.sv; \
	vvp "$$tmp_dir/tb_poly_trap_packet_stage"; \
	iverilog -g2012 -o "$$tmp_dir/tb_poly_memory_order" \
		rtl/poly_memory_order.sv rtl/tb_poly_memory_order.sv; \
	vvp "$$tmp_dir/tb_poly_memory_order"

check-poly-rtl-verilator:
	verilator $(POLY_RTL_VERILATOR_FLAGS) $(POLY_RTL_SV)

check-poly-rtl-yosys:
	yosys -q -p 'read_verilog -sv $(POLY_RTL_SV); hierarchy -top $(POLY_RTL_TOP); proc; check'

check-poly-rtl-synth:
	yosys -q -p 'read_verilog -sv $(POLY_RTL_SV); hierarchy -top $(POLY_RTL_TOP); synth -top $(POLY_RTL_TOP); check'

check-poly-rtl-constraints:
	test -s $(POLY_RTL_XDC)
	python3 rtl/test_poly_frontend_fpga_constraints.py

check-poly-rtl-fpga:
	tmp_dir=$$(mktemp -d); \
	trap 'rm -rf "$$tmp_dir"' EXIT; \
	yosys -q -p "read_verilog -sv $(POLY_RTL_SV); synth_xilinx -family xc7 -top $(POLY_RTL_TOP) -noiopad -noclkbuf -edif $$tmp_dir/$(POLY_RTL_TOP).edif; stat"; \
	test -s "$$tmp_dir/$(POLY_RTL_TOP).edif"; \
	echo POLY_RTL_FPGA_SYNTH_OK

check-poly-rtl-fpga-resources:
	tmp_dir=$$(mktemp -d); \
	trap 'rm -rf "$$tmp_dir"' EXIT; \
	yosys -p "read_verilog -sv $(POLY_RTL_SV); synth_xilinx -family xc7 -top $(POLY_RTL_TOP) -noiopad -noclkbuf -edif $$tmp_dir/$(POLY_RTL_TOP).edif; stat -tech xilinx" > "$$tmp_dir/yosys.log"; \
	test -s "$$tmp_dir/$(POLY_RTL_TOP).edif"; \
	awk '/Number of cells:/ { cells = $$4 } /Estimated number of LCs:/ { lcs = $$5 } END { if (!cells || !lcs) exit 1; printf "POLY_RTL_FPGA_RESOURCES cells=%s estimated_lcs=%s\n", cells, lcs }' "$$tmp_dir/yosys.log"

check-poly-rtl-hdl: check-poly-rtl-constraints check-poly-rtl-verilator check-poly-rtl-yosys check-poly-rtl-synth check-poly-rtl-fpga check-poly-rtl-fpga-resources

check-poly-rtl-fpga-artifacts:
	python3 rtl/test_poly_frontend_fpga_artifacts.py

poly-rtl-fpga-artifacts: check-poly-rtl-constraints
	mkdir -p $(POLY_RTL_FPGA_OUT)
	yosys -p "read_verilog -sv $(POLY_RTL_SV); synth_xilinx -family xc7 -top $(POLY_RTL_TOP) -noiopad -noclkbuf -edif $(POLY_RTL_FPGA_EDIF); stat -tech xilinx" > "$(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).yosys.log"
	test -s $(POLY_RTL_FPGA_EDIF)
	cp $(POLY_RTL_XDC) $(POLY_RTL_FPGA_XDC)
	cells=$$(awk '/Number of cells:/ { cells = $$4 } END { if (!cells) exit 1; print cells }' "$(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).yosys.log"); \
	lcs=$$(awk '/Estimated number of LCs:/ { lcs = $$5 } END { if (!lcs) exit 1; print lcs }' "$(POLY_RTL_FPGA_OUT)/$(POLY_RTL_TOP).yosys.log"); \
	sv_sha=$$(sha256sum $(POLY_RTL_SV) | sha256sum | awk '{ print $$1 }'); \
	edif_sha=$$(sha256sum $(POLY_RTL_FPGA_EDIF) | awk '{ print $$1 }'); \
	xdc_sha=$$(sha256sum $(POLY_RTL_FPGA_XDC) | awk '{ print $$1 }'); \
	{ \
	  printf "top=%s\n" "$(POLY_RTL_TOP)"; \
	  printf "rtl_sources_sha256=%s\n" "$$sv_sha"; \
	  printf "edif=%s\n" "$(POLY_RTL_FPGA_EDIF)"; \
	  printf "xdc=%s\n" "$(POLY_RTL_FPGA_XDC)"; \
	  printf "cells=%s\n" "$$cells"; \
	  printf "estimated_lcs=%s\n" "$$lcs"; \
	  printf "edif_sha256=%s\n" "$$edif_sha"; \
	  printf "xdc_sha256=%s\n" "$$xdc_sha"; \
	  printf "timing_closure=not_run\n"; \
	} > $(POLY_RTL_FPGA_MANIFEST)
	python3 rtl/test_poly_frontend_fpga_artifacts.py
	@echo POLY_RTL_FPGA_ARTIFACTS $(POLY_RTL_FPGA_EDIF) $(POLY_RTL_FPGA_XDC) $(POLY_RTL_FPGA_MANIFEST)

boot:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e RUN_NATIVE_CHECK=1 \
		-e EXPECT_POLY_CPUID=0 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_NEUTRAL=1 \
		-e RUN_POLY_EXEC=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-network-smoke:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_GUEST_NETWORK_SMOKE=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-nativecheck-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-real-xsave-arch-traps: $(POLY_XCR0_MODULE)
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e REQUIRE_POLY_REAL_XSAVE=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-probe-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_PROBE=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-apps-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_EXEC=0 \
		-e RUN_POLY_CALL=0 \
		-e RUN_POLY_THREAD=0 \
		-e RUN_POLY_SIGNAL=0 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-neutral-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_NEUTRAL=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-exec-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_EXEC=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-exec-cross-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_EXEC_CROSS=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-exec-syscall-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_EXEC_SYSCALL=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-call-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_CALL=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-call-real-xsave-arch-traps: $(POLY_XCR0_MODULE)
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_CALL=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=1 \
		-e REQUIRE_POLY_REAL_XSAVE=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-thread-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=0 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-bench-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_BENCH=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-binfmt-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-focused-validation:
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-probe-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-apps-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-neutral-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-exec-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-exec-cross-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-exec-syscall-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-call-real-xsave-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-binfmt-arch-traps
	$(MAKE) BOOT_TIMEOUT_SECONDS=$(BOOT_FOCUSED_TIMEOUT_SECONDS) boot-poly-bench-arch-traps

boot-poly-full-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_NEUTRAL=1 \
		-e RUN_POLY_EXEC_CROSS=1 \
		-e RUN_POLY_EXEC_SYSCALL=1 \
		-e RUN_POLY_EXEC=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e RUN_POLY_CALL=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=1 \
		-e RUN_POLY_BENCH=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-full-real-xsave-arch-traps: $(POLY_XCR0_MODULE)
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_NEUTRAL=1 \
		-e RUN_POLY_EXEC_CROSS=1 \
		-e RUN_POLY_EXEC_SYSCALL=1 \
		-e RUN_POLY_EXEC=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e RUN_POLY_CALL=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=1 \
		-e RUN_POLY_BENCH=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		-e REQUIRE_POLY_REAL_XSAVE=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-full:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(BOOT_DOCKER_ENV) \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_NEUTRAL=1 \
		-e RUN_POLY_EXEC_CROSS=1 \
		-e RUN_POLY_EXEC_SYSCALL=1 \
		-e RUN_POLY_EXEC=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e RUN_POLY_CALL=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=1 \
		-e RUN_POLY_BENCH=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

clean:
	rm -rf cache out tmp
