ENGINE=docker
#ENGINE=podman

# Example mounting source code directory into container
# (lets you "work on the code used to bootstrap" a bit easier)
#EXTRA=-v $(HOME)/Code/haiku:/work/src/haiku

default:
	${ENGINE} build . -t docker.io/haiku/bootstrap
clean:
	${ENGINE} ps -q --filter=ancestor=docker.io/haiku/bootstrap | xargs -I {} docker kill {}
	${ENGINE} ps -a -q --filter=ancestor=docker.io/haiku/bootstrap | xargs -I {} docker rm {}
	${ENGINE} volume rm bootstrap_work
init:
	${ENGINE} run -v bootstrap_work:/work docker.io/haiku/bootstrap prep
crosstools:
	${ENGINE} run -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work ${EXTRA} docker.io/haiku/bootstrap crosstools
bootstrap:
	${ENGINE} run -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work ${EXTRA} docker.io/haiku/bootstrap bootstrap
enter:
	${ENGINE} run -it -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work ${EXTRA} docker.io/haiku/bootstrap /bin/bash -l
