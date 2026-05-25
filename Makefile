IMAGE ?= armd64-bochs

.PHONY: image boot boot-poly boot-poly-full clean

image:
	docker build --platform=linux/arm64 -t $(IMAGE) .

boot:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
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

boot-poly-full:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		-e POLY_ENABLED=1 \
		-e RUN_POLY_PROBE=1 \
		-e RUN_POLY_APPS=1 \
		-e RUN_POLY_BINFMT=1 \
		$(IMAGE) \
		./scripts/boot.sh

clean:
	rm -rf cache out tmp
