#include <stdlib.h>
#include <stdio.h>
#include <net/if.h>


int main(int argc, char* argv[])
{
	struct if_nameindex* ifs;
	int i;

	ifs = if_nameindex();
	if (ifs == NULL) { 
  		perror("if_nameindex"); 
  		exit(EXIT_FAILURE);
	}

	for (i = 0; ifs[i].if_index != 0 || ifs[i].if_name != NULL; i++) {
		printf("%d %s\n", ifs[i].if_index, ifs[i].if_name);
	}

	return EXIT_SUCCESS;
}
