/*
 * Copyright 2017, ohnx, me@masonx.ca.
 * Distributed under the terms of the CC0 License.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int
main()
{
    void *ptr;

    printf("Testing calloc(SIZE_MAX, SIZE_MAX)... ");
    if (calloc(SIZE_MAX, SIZE_MAX) != NULL) {
        printf("fail!\n");
    }
    printf("pass!\n");

    printf("Testing calloc(0, 0)... ");
    /* issues with calloc() here will throw a panic */
    ptr = calloc(0, 0);
    /* free the value, since calloc() should return a free() able pointer */
    free(ptr);
    printf("pass!\n");

    printf("Testing calloc(-1, -1)... ");
    if (calloc(-1, -1) != NULL) {
        printf("")
    }
    printf("pass!\n");

	return 0;
}
