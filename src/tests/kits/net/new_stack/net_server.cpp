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

int test_data()
{
	net_data 	*data;
  	char		buffer[128];
  	size_t		len;
  	char 		*tata;
  	char 		*titi;
  	char 		*toto;
  	char 		*tutu;

  puts("test_data():");
	
  tata = (char *) malloc(16);
  strcpy(tata, "0123456789");

  titi = (char *) malloc(16);
  strcpy(titi, "ABCDEF");

  toto = (char *) malloc(48);
  strcpy(toto, "abcdefghijklmnopqrstuvwxyz");

  tutu = (char *) malloc(16);
  strcpy(tutu, "hello there!");
  
  data = g_stack->new_data();

  g_stack->append_data(data, titi, 6, my_free);
  g_stack->prepend_data(data, tata, 10, my_free);
  g_stack->add_data_free_node(data, tutu, (void *) "hello there", my_free2);
  g_stack->insert_data(data, 5, toto, 26, my_free);
  g_stack->prepend_data(data, "this is a net_data test: ", 25, NULL);

  g_stack->dump_data(data);

  len = g_stack->copy_from_data(data, 2, buffer, 128);
  buffer[len] = 0;

  printf("buffer = [%s]\n", buffer);

  g_stack->delete_data(data, false);

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
		
		test_data();

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

