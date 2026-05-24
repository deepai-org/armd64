IMAGE ?= armd64-bochs

.PHONY: image boot clean

image:
	docker build -t $(IMAGE) .

boot:
	docker run --rm \
		-v "$(CURDIR)":/work \
		$(IMAGE) \
		./scripts/boot.sh

clean:
	rm -rf cache out tmp
