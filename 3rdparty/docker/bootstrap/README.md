# Haiku bootstrap in a container

1) make
2) make init
3) TARGET_ARCH=arm make crosstools
4) TARGET_ARCH=arm make bootstrap
5) ```make enter``` lets you enter the container and poke around.
6) profit!
