#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_input(char *data)
{
    strcpy(data, "01234567");
}


int main(void)
{
    char buffer[8];
    get_input(buffer);
    return buffer[0];
}
