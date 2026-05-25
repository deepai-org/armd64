IMAGE ?= armd64-bochs

.PHONY: image boot clean

image:
	docker build --platform=linux/arm64 -t $(IMAGE) .

boot:
	docker run --rm \
		--platform=linux/arm64 \
		-v "$(CURDIR)":/work \
		$(IMAGE) \
		./scripts/boot.sh

clean:
	rm -rf cache out tmp
