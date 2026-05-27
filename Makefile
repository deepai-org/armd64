IMAGE ?= armd64-bochs

.PHONY: image check-poly-import-ids check-poly-arch-contract check-poly-cpuid-contract boot boot-poly boot-poly-arch-traps boot-poly-probe-arch-traps boot-poly-apps-arch-traps boot-poly-call-arch-traps boot-poly-bench-arch-traps boot-poly-binfmt-arch-traps boot-poly-full-arch-traps boot-poly-full clean

image:
	docker build --platform=linux/arm64 -t $(IMAGE) .

check-poly-import-ids:
	./tools/check_poly_import_ids.sh

check-poly-arch-contract:
	./tools/check_poly_arch_contract.sh

check-poly-cpuid-contract:
	./tools/check_poly_cpuid_contract.sh

boot:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e RUN_NATIVE_CHECK=1 \
		-e EXPECT_POLY_CPUID=0 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_EXEC=0 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-probe-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_PROBE=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-apps-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_EXEC=0 \
		-e RUN_POLY_CALL=0 \
		-e RUN_POLY_THREAD=0 \
		-e RUN_POLY_SIGNAL=0 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-call-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_CALL=1 \
		-e RUN_POLY_THREAD=1 \
		-e RUN_POLY_SIGNAL=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-bench-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_BENCH=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-binfmt-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-full-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_EXEC=0 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e RUN_POLY_BENCH=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-full:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_NATIVE_CHECK=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_EXEC=0 \
		-e RUN_POLY_ARCH_TRAP_EXEC=1 \
		-e RUN_POLY_BENCH=1 \
		-e RUN_POLY_BINFMT=1 \
		-e RUN_POLY_BINFMT_ARCH_TRAPS=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

clean:
	rm -rf cache out tmp
