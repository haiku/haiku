/* This file is included in fs_shell:fsh.c
 * Insert your definition of additional commands here
 *
 * Format:
 *	{ commandName, functionName, commandDescription },
 *
 * And don't forget the comma after the line :-)
 */

    { "chkbfs",	 do_chkbfs, "does a chkbfs on the volume" },

/* You can also define your own prompt so that your fs shell
 * can be differentiated from others easily:
 */

#define FS_SHELL_PROMPT "bfs_shell"

