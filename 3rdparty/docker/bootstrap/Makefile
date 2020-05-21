ENGINE=docker
#ENGINE=podman

# Example mounting source code directory into container
# (lets you "work on the code used to bootstrap" a bit easier)
SOURCES=$(HOME)/Code
EXTRA=-v $(SOURCES)/haiku:/work/src/haiku -v $(SOURCES)/buildtools:/work/src/buildtools

default:
	${ENGINE} build . --no-cache -t docker.io/haiku/bootstrap
clean:
	${ENGINE} ps -q --filter=ancestor=docker.io/haiku/bootstrap | xargs -I {} ${ENGINE} kill {}
	${ENGINE} ps -a -q --filter=ancestor=docker.io/haiku/bootstrap | xargs -I {} ${ENGINE} rm {}
	${ENGINE} volume rm bootstrap_work
prep:
	${ENGINE} run -v bootstrap_work:/work:exec ${EXTRA} docker.io/haiku/bootstrap prep
crosstools:
	${ENGINE} run -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work:exec ${EXTRA} docker.io/haiku/bootstrap crosstools
bootstrap:
	${ENGINE} run -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work:exec ${EXTRA} docker.io/haiku/bootstrap bootstrap
enter:
	${ENGINE} run -it -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work:exec ${EXTRA} docker.io/haiku/bootstrap /bin/bash -l
