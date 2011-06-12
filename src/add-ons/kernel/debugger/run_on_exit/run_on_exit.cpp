/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch
 * Distributed under the terms of the MIT License.
 */
#include <debug.h>
#include <signal.h>
#include <string.h>
#include <image.h>


static sem_id sRequestSem = -1;
static char sCommandBuffer[1024];
static uint32 sCommandOffset = 0;
static uint32 sCommandCount = 0;


static int32
run_on_exit_loop(void *data)
{
	while (true) {
		if (acquire_sem(sRequestSem) != B_OK)
			break;

		char *pointer = sCommandBuffer;
		while (sCommandCount > 0) {
			uint8 argCount = (uint8)pointer[0];
			pointer++;

			const char *args[argCount];
			for (uint8 i = 0; i < argCount; i++) {
				args[i] = pointer;
				uint32 length = strlen(pointer);
				pointer += length + 1;
			}

			thread_id thread = load_image(argCount, args, NULL);
			if (thread >= B_OK)
				resume_thread(thread);
			sCommandCount--;
		}

		sCommandOffset = 0;
	}

	return 0;
}


static int
add_run_on_exit_command(int argc, char **argv)
{
	if (argc < 2 || strcmp(argv[1], "--help") == 0) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	if (argc > 256) {
		kprintf("too many arguments\n");
		return 0;
	}

	size_t totalLength = 1;
	for (int32 i = 1; i < argc; i++)
		totalLength += strlen(argv[i]) + 1;

	if (sCommandOffset + totalLength > sizeof(sCommandBuffer)) {
		kprintf("no space left in command buffer\n");
		return 0;
	}

	char *pointer = sCommandBuffer + sCommandOffset;
	*pointer++ = (char)(argc - 1);

	for (int32 i = 1; i < argc; i++) {
		strcpy(pointer, argv[i]);
		pointer += strlen(argv[i]) + 1;
	}

	sCommandOffset += totalLength;
	sCommandCount++;
	return 0;
}


static void
exit_debugger()
{
	if (sCommandCount > 0)
		release_sem_etc(sRequestSem, 1, B_DO_NOT_RESCHEDULE);
}


static status_t
std_ops(int32 op, ...)
{
	if (op == B_MODULE_INIT) {
		sRequestSem = create_sem(0, "run_on_exit_request");
		if (sRequestSem < B_OK)
			return sRequestSem;

		thread_id thread = spawn_kernel_thread(&run_on_exit_loop,
			"run_on_exit_loop", B_NORMAL_PRIORITY, NULL);
		if (thread < B_OK)
			return thread;

		resume_thread(thread);

		add_debugger_command_etc("on_exit", &add_run_on_exit_command,
			"Adds a command to be run when leaving the kernel debugger",
			"<command> [<arguments>]\n"
			"Adds a command to be run when leaving the kernel debugger.\n", 0);

		return B_OK;
	} else if (op == B_MODULE_UNINIT) {
		remove_debugger_command("on_exit", &add_run_on_exit_command);
		// deleting the sem will also cause the thread to exit
		delete_sem(sRequestSem);
		sRequestSem = -1;
		return B_OK;
	}

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/run_on_exit/v1",
		B_KEEP_LOADED,
		&std_ops
	},

	NULL,
	exit_debugger,
	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};
