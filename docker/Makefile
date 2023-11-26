VERSION  ?= latest
DOCKER   ?= docker
IMG_NAME ?= freshtomato

build_freshtomato: .freshtomato.docker_built

.freshtomato.docker_built: Dockerfile nss_wrapper
	$(DOCKER) build -t $(IMG_NAME):$(VERSION) .
	touch .freshtomato.docker_built

run_freshtomato: .freshtomato.docker_built
	$(DOCKER) run -v $(abspath ../):/ws -it --user $(shell id -u):$(shell id -g) $(IMG_NAME):$(VERSION) /bin/bash

build.%: .freshtomato.docker_built
	$(DOCKER) run -v $(abspath ../):/ws -it --user $(shell id -u):$(shell id -g) $(IMG_NAME):$(VERSION) make -C /ws/release/src-rt-6.x.4708 $(subst build.,,$@)

build.noit.%: .freshtomato.docker_built
	$(DOCKER) run -v $(abspath ../):/ws --user $(shell id -u):$(shell id -g) $(IMG_NAME):$(VERSION) make -C /ws/release/src-rt-6.x.4708 $(subst build.noit.,,$@)

clean:
	rm -f .freshtomato.docker_built

help:
	@make -s -C ../release/src-rt-6.x.4708 help | sed 's,^[a-z],build.&,g'
