/* Userland modules emulation support
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app/Application.h>
#include <drivers/module.h>

#include "net_stack.h"
// #include <userland_ipc.h>

struct net_stack_module_info * g_stack = NULL;



void my_free(void * ptr, ...)
{
	printf("my_free(%p)\n", ptr);
	free(ptr);
}

void my_free2(void * prompt, ...) // void * ptr)
{
	va_list args;
	void * ptr;
	
	va_start(args, prompt);
	ptr = va_arg(args, void *);
	va_end(args);
	
	printf("%s: my_free2(%p)\n", (char *) prompt, ptr);
	free(ptr);
}

int test_buffer()
{
	net_buffer 	*buffer;
  	char		data[128];
  	size_t		len;
  	char 		*tata;
  	char 		*titi;
  	char 		*toto;
  	char 		*tutu;

  puts("test_buffer():");
	
  tata = (char *) malloc(16);
  strcpy(tata, "0123456789");

  titi = (char *) malloc(16);
  strcpy(titi, "ABCDEF");

  toto = (char *) malloc(48);
  strcpy(toto, "abcdefghijklmnopqrstuvwxyz");

  tutu = (char *) malloc(16);
  strcpy(tutu, "hello there!");
  
  buffer = g_stack->new_buffer();

  g_stack->add_to_buffer(buffer, BUFFER_END, titi, 6, my_free);
  g_stack->add_to_buffer(buffer, 0, tata, 10, my_free);
  g_stack->attach_buffer_free_element(buffer, tutu, (void *) "hello there", my_free2);
  g_stack->add_to_buffer(buffer, 5, toto, 26, my_free);
  g_stack->add_to_buffer(buffer, 0, "this is a net_buffer test: ", 27, NULL);

  g_stack->dump_buffer(buffer);

  printf("Removing 61 bytes at offset 4...\n");
  g_stack->remove_from_buffer(buffer, 4, 61);

  g_stack->dump_buffer(buffer);

  len = g_stack->read_buffer(buffer, 2, data, sizeof(data));
  data[len] = 0;

  printf("data = [%s]\n", data);

  g_stack->delete_buffer(buffer, false);
  return 0;
}


int main(int argc, char **argv)
{
	int ret = -1;
	
	new BApplication("application/x-vnd-OBOS-net_server");

	// if (init_userland_ipc() < B_OK)
	//	goto exit0;

	if (get_module(NET_STACK_MODULE_NAME, (module_info **) &g_stack) != B_OK)
		goto exit1;

	if (g_stack->start() == B_OK) {

		puts("Userland net stack (net_server) is running.");
		puts("Press any key to quit.");

		fflush(stdin);
		fgetc(stdin);;
		
		test_buffer();

		g_stack->stop();
	};

	put_module(NET_STACK_MODULE_NAME);;

	ret = 0;

 exit1:;
	// shutdown_userland_ipc();
	
// exit0:;
	delete be_app;
	return ret;
}

