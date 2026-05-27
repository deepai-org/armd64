IMAGE ?= armd64-bochs

.PHONY: image boot boot-poly boot-poly-arch-traps boot-poly-full clean

image:
	docker build --platform=linux/arm64 -t $(IMAGE) .

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
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-arch-traps:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e POLY_COMPAT_TRAPS=0 \
		-e RUN_NATIVE_CHECK=1 \
		-e EXPECT_POLY_CPUID=1 \
		$(IMAGE) \
		./scripts/boot.sh

boot-poly-full:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_BENCH=1 \
		-e RUN_POLY_BINFMT=1 \
		$(IMAGE) \
		./scripts/boot.sh

clean:
	rm -rf cache out tmp
