default:
	docker build . -t docker.io/haiku/bootstrap
clean:
	docker rm docker.io/haiku/bootstrap
	docker volume rm bootstrap_work
init:
	docker run -v bootstrap_work:/work docker.io/haiku/bootstrap prep
crosstools:
	docker run -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work docker.io/haiku/bootstrap crosstools
bootstrap:
	docker run -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work docker.io/haiku/bootstrap bootstrap
enter:
	docker run -it -e TARGET_ARCH=$(TARGET_ARCH) -v bootstrap_work:/work docker.io/haiku/bootstrap /bin/bash -l
