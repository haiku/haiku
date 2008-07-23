/*
 * Copyright 2008, François Revol, revol@free.fr
 * Distributed under the terms of the MIT License.
 */
#include <ctype.h>
#include <cxxabi.h>
#include <debug.h>
#include <malloc.h>
#include <string.h>

#define DEMANGLE_BUFFER_SIZE (16*1024)
static char sDemangleBuffer[DEMANGLE_BUFFER_SIZE];

extern "C" void set_debug_demangle_hook(const char *(*demangle_hook)(const char *));

/* gcc's __cxa_demangle calls malloc and friends...
 * we don't want to let it call the real one from inside the kernel debugger... 
 * instead we just return it a static buffer.
 */

void *malloc(size_t len)
{
	if (len < DEMANGLE_BUFFER_SIZE)
		return sDemangleBuffer;
	return NULL;
}


void free(void *ptr)
{
}


void *realloc(void * old_ptr, size_t new_size)
{
	return NULL;
}


/* doesn't work, va_list sux... */
#if 0
class DemangleDebugOutputFilter : public DebugOutputFilter {
public:
        DemangleDebugOutputFilter();
	virtual ~DemangleDebugOutputFilter();
        void SetChain(DebugOutputFilter *chain);
	virtual void PrintString(const char* string);
	virtual void Print(const char* format, va_list args);
private:
	DebugOutputFilter *fChained;
};


DemangleDebugOutputFilter::DemangleDebugOutputFilter()
{
}


DemangleDebugOutputFilter::~DemangleDebugOutputFilter()
{
}


void
DemangleDebugOutputFilter::SetChain(DebugOutputFilter *chain)
{
	fChained = chain;
}


void
DemangleDebugOutputFilter::PrintString(const char* string)
{
	fChained->PrintString(string);
}


void
DemangleDebugOutputFilter::Print(const char* format, va_list args)
{
	int i;
	const char *p = format;
	while ((p = strchr(p, '%'))) {
		if (p[1] == '%') {
			p+=2;
			continue;
		}
		p++;
		while (*p && !isalpha(*p))
			p++;
		switch (*p) {
			case 's':
			{
				const char **ptr = &va_arg(args, char *);
				//char *symbol = va_arg(args, char *);
				char *symbol = *ptr;
				char *demangled;
				size_t length = DEMANGLE_BUFFER_SIZE;
				int status;

				demangled = abi::__cxa_demangle(symbol, sDemangleBuffer, &length, &status);
				if (status > 0)	{
				}
				break;
			}
			case 'd':
			case 'i':
			{
				int d;
				d = va_arg(args, int);
				break;
			}
			case 'L':
			{
				long long d;
				d = va_arg(args, long long);
				break;
			}
			case 'f':
			{
				float f;
				f = va_arg(args, float);
				break;
			}
			case '\0':
				break;
			default:
				panic("unknown printf format\n");
				break;
		}
		i++;
	}
	fChained->Print(format, args);
}
#endif

static const char *kdebug_demangle(const char *sym)
{
	char *demangled;
	size_t length = DEMANGLE_BUFFER_SIZE;
	int status;

	demangled = abi::__cxa_demangle(sym, sDemangleBuffer, &length, &status);
	if (status == 0)
		return demangled;
	// something bad happened
	return sym;
}

static void
exit_debugger()
{
}


static status_t
std_ops(int32 op, ...)
{
	if (op == B_MODULE_INIT) {
#if 0
		DebugOutputFilter *old;
		DemangleDebugOutputFilter *filter = new(std::nothrow) DemangleDebugOutputFilter;
		old = set_debug_output_filter(filter);
		filter->SetChain(old);
#endif
		debug_set_demangle_hook(kdebug_demangle);

		return B_OK;
	} else if (op == B_MODULE_UNINIT) {
		return B_OK;
	}

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/demangle/v1",
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

