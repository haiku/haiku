// KernelDebug.h

#ifndef USERLAND_FS_KERNEL_DEBUG_H
#define USERLAND_FS_KERNEL_DEBUG_H

class KernelDebug {
public:
	static	void				AddDebuggerCommands();
	static	void				RemoveDebuggerCommands();

private:
	static	int					DebugUFS(int argc, char** argv);
	static	int					DebugPortPool(int argc, char** argv);
	static	int					DebugPort(int argc, char** argv);
};

// no kernel debugger commands in userland
#if USER
inline void KernelDebug::AddDebuggerCommands()		{}
inline void KernelDebug::RemoveDebuggerCommands()	{}
#endif

#endif	// USERLAND_FS_KERNEL_DEBUG_H
