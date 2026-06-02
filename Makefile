IMAGE ?= armd64-bochs
POLY_XCR0_MODULE ?= out/poly_xcr0.ko
BOOT_TIMEOUT_SECONDS ?= 300
BOOT_FOCUSED_TIMEOUT_SECONDS ?= 900
BOOT_DETAIL_ASSERTS ?= 1
BOOT_DOCKER_ENV = -e BOOT_TIMEOUT_SECONDS=$(BOOT_TIMEOUT_SECONDS) -e BOOT_DETAIL_ASSERTS=$(BOOT_DETAIL_ASSERTS)

.PHONY: image poly-xcr0-module check-poly-import-ids check-poly-arch-contract check-poly-cpuid-contract check-poly-rtl boot boot-poly boot-poly-arch-traps boot-poly-nativecheck-arch-traps boot-poly-real-xsave-arch-traps boot-poly-probe-arch-traps boot-poly-apps-arch-traps boot-poly-neutral-arch-traps boot-poly-exec-arch-traps boot-poly-exec-cross-arch-traps boot-poly-exec-syscall-arch-traps boot-poly-call-arch-traps boot-poly-call-real-xsave-arch-traps boot-poly-thread-arch-traps boot-poly-bench-arch-traps boot-poly-binfmt-arch-traps boot-poly-focused-validation boot-poly-full-arch-traps boot-poly-full-real-xsave-arch-traps boot-poly-full clean

image:
	docker build --platform=linux/arm64 -t $(IMAGE) .

poly-xcr0-module: $(POLY_XCR0_MODULE)

$(POLY_XCR0_MODULE): tools/kernel/poly_xcr0.c scripts/build_poly_xcr0_module.sh
	./scripts/build_poly_xcr0_module.sh

check-poly-import-ids:
	./scripts/checks/check_poly_import_ids.sh

check-poly-arch-contract:
	./scripts/checks/check_poly_arch_contract.sh

check-poly-cpuid-contract:
	./scripts/checks/check_poly_cpuid_contract.sh

check-poly-rtl:
	python3 rtl/test_poly_ctrl_decode.py
	python3 rtl/test_poly_frontend_handoff.py
	python3 rtl/test_poly_frontend_step.py
	python3 rtl/test_poly_transition_stack.py
	python3 rtl/test_poly_abi_signature_slots.py
	python3 rtl/test_poly_cpuid_rom.py
	python3 rtl/test_poly_raw_fetch_plan.py
	python3 rtl/test_poly_trap_packet_encode.py

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
