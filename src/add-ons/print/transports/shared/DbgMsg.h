// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __DBGMSG_H
#define __DBGMSG_H

#ifdef _DEBUG
	void write_debug_stream(const char *, ...);
	void DUMP_BFILE(BFile *file, const char *name);
	void DUMP_BMESSAGE(BMessage *msg);
	void DUMP_BDIRECTORY(BDirectory *dir);
	#define DBGMSG(args)	write_debug_stream args
#else
	#define DUMP_BFILE(file, name)	(void)0
	#define DUMP_BMESSAGE(msg)		(void)0
	#define DUMP_BDIRECTORY(dir)	(void)0
	#define DBGMSG(args)			(void)0
#endif

#endif	/* __DBGMSG_H */
