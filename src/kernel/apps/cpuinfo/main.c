#include <OS.h>
#include <stdio.h>
//#include <stdlib.h> -- when exit() is implemented
#include <string.h>

#include "flag_arrays.c"


void AMD_identify (cpuid_info *);
void AMD_features (cpuid_info *);
void AMD_TLB_cache (cpuid_info *);
void Cyrix_identify (cpuid_info *);
void Cyrix_features (cpuid_info *);
void Cyrix_TLB_cache (cpuid_info *);
void Intel_identify (cpuid_info *);
void Intel_features (cpuid_info *);
void Intel_TLB_cache (cpuid_info *);

const char *Intel_brand_string (int, int);
void dump_regs (cpuid_info *, const char *, uint32, uint32);
char getoption (char *optstr);
void opt_identify (cpuid_info *, int);
void opt_features (cpuid_info *, int);
void opt_TLB_cache (cpuid_info *, int);
void opt_dump_calls (cpuid_info *, uint32);
void print_regs (cpuid_info *);
void usage (void);



static cpuid_info CPU_Data;


void
usage()
{
	printf("usage: cpuinfo [-option]\n\n"
	       "Prints information about your CPU.\n"
	       "  -i   CPU identification\n"
	       "  -f   supported processor features\n"
	       "  -t   TLB and cache info\n"
	       "  -d   dump registers from CPUID calls\n");
	//exit(0);
}


char
getoption(char *optstr)
{
	if ((optstr[0] != '-')
		|| (strlen(optstr) > 2)
		|| (strchr("iftd", optstr[1]) == NULL))
		// invalid option
		return 0;
	
	return optstr[1];
}


int
main(int argc, char *argv[])
{
	// technical reference:
	// "Intel Processor Identification and the CPUID Instruction"
	// (ftp://download.intel.com/design/Xeon/applnots/24161821.pdf)
	
	char   option = 0;
	int    vendor_tag;
	uint32 max_level;
	cpuid_info *info = &CPU_Data;
	
	if (argc == 2)
		option = getoption(argv[1]);
	
	if (option == 0) {
		usage();
		return 0;
	}
	
	// get initial info (max_level and vendor_tag)
	get_cpuid(info, 0, 0);
	max_level = info->eax_0.max_eax;
	if (max_level < 1) {
		printf("\nSorry, your computer does not seem to support the CPUID instruction\n");
		return 1;
	}
	
	vendor_tag = info->regs.ebx;
	switch (option) {
		case 'i':
			opt_identify(info, vendor_tag);
			break;
		case 'f':
			opt_features(info, vendor_tag);
			break;
		case 't':
			opt_TLB_cache(info, vendor_tag);
			break;
		case 'd':
			opt_dump_calls(info, max_level);
			break;
	}

	return 0;
}


void
opt_identify(cpuid_info *info, int vendor_tag)
{
	char vendorID[12+1] = {0};
	
	// the 'vendorid' field of the info struct is not null terminated,
	// so it is copied into a properly terminated local string buffer
	memcpy(vendorID, info->eax_0.vendorid, 12);
	
	printf("\nCPU identification:\n");
	printf("%12s '%s'\n",  "Vendor ID:", vendorID);
	
	switch (vendor_tag) {
		case 'uneG':  // "GenuineIntel"
			Intel_identify(info);
			break;
		
		case 'htuA':  // "AuthenticAMD"
			AMD_identify(info);
			break;
		
		case 'iryC':  // "CyrixInstead"
			Cyrix_identify(info);
			break;
	}
}


void
opt_features(cpuid_info *info, int vendor_tag)
{
	switch (vendor_tag) {
		case 'uneG':  // "GenuineIntel"
			Intel_features(info);
			break;
		
		case 'htuA':  // "AuthenticAMD"
			AMD_features(info);
			break;
		
		case 'iryC':  // "CyrixInstead"
			Cyrix_features(info);
			break;
	}
}


void
opt_TLB_cache(cpuid_info *info, int vendor_tag)
{
	switch (vendor_tag) {
		case 'uneG':  // "GenuineIntel"
			Intel_TLB_cache(info);
			break;
		
		case 'htuA':  // "AuthenticAMD"
			AMD_TLB_cache(info);
			break;
		
		case 'iryC':  // "CyrixInstead"
			Cyrix_TLB_cache(info);
			break;
	}
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Intel specs

void
Intel_identify(cpuid_info *info)
{
	int type, family, model, sig;
	
	get_cpuid(info, 1, 0);
	type   = info->eax_1.type;
	family = info->eax_1.family;
	model  = info->eax_1.model;
	
	sig = info->regs.eax;
	printf("%12s  0x%08X\n", "Signature:", sig);
	
	printf("%12s %2d '", "Type:", type);
	if      (type == 0) printf("Original OEM");
	else if (type == 1) printf("Overdrive");
	else if (type == 2) printf("Dual-capable");
	else if (type == 3) printf("Reserved");
	printf("'\n");

	printf("%12s %2d '", "Family:", family);
	if      (family == 3)  printf("i386");
	else if (family == 4)  printf("i486");
	else if (family == 5)  printf("Pentium");
	else if (family == 6)  printf("Pentium Pro");
	else if (family == 15) printf("Pentium 4");
	printf("'\n");
		
	printf("%12s %2d '", "Model:", model);
	switch (family) {
	case 3:
		break;
	case 4:
		if      (model == 0) printf("DX");
		else if (model == 1) printf("DX");
		else if (model == 2) printf("SX");
		else if (model == 3) printf("487/DX2");
		else if (model == 4) printf("SL");
		else if (model == 5) printf("SX2");
		else if (model == 7) printf("write-back enhanced DX2");
		else if (model == 8) printf("DX4");			
		break;
	case 5:
		if      (model == 1) printf("60/66");
		else if (model == 2) printf("75-200");
		else if (model == 3) printf("for 486 system");
		else if (model == 4) printf("MMX");
		break;
	case 6:
		if      (model == 1) printf("Pentium Pro");
		else if (model == 3) printf("Pentium II Model 3");
		else if (model == 5) printf("Pentium II Model 5/Xeon/Celeron");
		else if (model == 6) printf("Celeron");
		else if (model == 7) printf("Pentium III/Pentium III Xeon - external L2 cache");
		else if (model == 8) printf("Pentium III/Pentium III Xeon - internal L2 cache");
		break;
	case 15:
		break;
	}
	printf("'\n");
	
	if (model == 15) {
		int emodel = (info->regs.eax >> 16) & 0xf;
		printf("Extended model: %d\n", emodel);
	}
	
	printf("%12s  0x%08lX\n", "Features:", info->eax_1.features);
	
	{
		int id = info->regs.ebx & 0xff;
		printf("%12s %2d '%s'\n", "Brand ID:", id, Intel_brand_string(id, sig));
	}
	
	printf("%12s %2d\n", "Stepping:", info->eax_1.stepping);
	printf("%12s %2d\n", "Reserved:", info->eax_1.reserved_0);
	
	get_cpuid(info, 0x80000000, 0);
	if (info->regs.eax & 0x80000000) {
		// extended feature/signature bits supported
		if (info->regs.eax >= 0x80000004) {
			int i;
			
			printf("\nExtended brand string:\n'");
			for (i = 0x80000002; i <= 0x80000004; ++i) {
				get_cpuid(info, i, 0);
				print_regs(info);
			}
			printf("'\n");
		}
	}
}


const char *
Intel_brand_string(int id, int processor_signature)
{
	int sig = processor_signature;
	
	switch (id) {
	case 0x0: return "Unsupported";
	case 0x1: return "Intel(R) Celeron(R) processor";
	case 0x2: return "Intel(R) Pentium(R) III processor";
	case 0x3: return (sig == 0x6B1) ? "Intel(R) Celeron(R) processor" : "Intel(R) Pentium(R) III Xeon(TM) processor";
	case 0x4: return "Intel(R) Pentium(R) III processor";
	case 0x5: return "";
	case 0x6: return "Mobile Intel(R) Pentium(R) III Processor-M";
	case 0x7: return "Mobile Intel(R) Celeron(R) processor";
	case 0x8: return (sig >= 0xF13) ? "Intel(R) Genuine processor" : "Intel(R) Pentium(R) 4 processor";
	case 0x9: return "Intel(R) Pentium(R) 4 processor";
	case 0xA: return "Intel(R) Celeron(R) processor";
	case 0xB: return (sig <  0xF13) ? "Intel(R) Xeon(TM) processor MP" : "Intel(R) Xeon(TM) processor";
	case 0xC: return "Intel(R) Xeon(TM) processor MP";
	case 0xD: return "";
	case 0xE: return (sig <  0xF13) ? "Intel(R) Xeon(TM) processor" : "Mobile Intel(R) Pentium(R) 4 Processor-M";
	case 0xF: return "Mobile Intel(R) Celeron(R) processor";
	default:
		return "";
	}
}


void
Intel_features(cpuid_info *info)
{
	// 
	
	int i;
	int fflags;
	
	get_cpuid(info, 1, 0);
	fflags = info->eax_1.features;
	
	printf("\nSupported processor features:\n");
	for (i = 0; i < 32; ++i)
		if (fflags & (1<<i))
			printf("   %s\n", Intel_feature_flags[i]);
	
	printf("\n");
}


void
Intel_TLB_cache(cpuid_info *info)
{
	//get_cpuid(info, 2, 0);
	printf("\nnot just yet...\n");
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AMD specs


void
AMD_identify(cpuid_info *info)
{
	printf("Sorry! Info about AMD cpu's not implemented yet :(\n");
}


void
AMD_features(cpuid_info *info)
{
	printf("Sorry! Info about AMD cpu's not implemented yet :(\n");
}


void
AMD_TLB_cache(cpuid_info *info)
{
	printf("Sorry! Info about AMD cpu's not implemented yet :(\n");
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Cyrix specs


void
Cyrix_identify(cpuid_info *info)
{
	printf("Sorry! Info about Cyrix cpu's not implemented yet :(\n");
}


void
Cyrix_features(cpuid_info *info)
{
	printf("Sorry! Info about Cyrix cpu's not implemented yet :(\n");
}


void
Cyrix_TLB_cache(cpuid_info *info)
{
	printf("Sorry! Info about Cyrix cpu's not implemented yet :(\n");
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// dumps


void
opt_dump_calls(cpuid_info *info, uint32 max_level)
{
	uint32 max_extended;
	
	printf("CPUID instruction call dump:\n");

	dump_regs(info, "Standard Calls", 0, max_level);

	get_cpuid(info, 0x80000000, 0);
	max_extended = info->regs.eax;
	
	if (max_extended == 0)
		printf("\nNo extended calls available for this CPU\n");
	else
		dump_regs(info, "Extended Calls", 0x80000000, max_extended);
}


void
dump_regs(cpuid_info *info, const char *heading, uint32 min_level, uint32 max_level)
{
	uint32 i;
	
	printf("\n%s (max eax level = %lx)\n", heading, max_level);
	printf("----------------------------------------------\n");
	printf("  Input  |   Output\n");
	printf("----------------------------------------------\n");
	printf("  eax    |   eax      ebx      ecx      edx\n");

	for (i = min_level; i <= max_level; ++i) {
		get_cpuid(info, i, 0);
		printf("%08lx | %08lx %08lx %08lx %08lx\n",
		       i,
		       info->regs.eax,
		       info->regs.ebx,
		       info->regs.ecx,
		       info->regs.edx);
	}
}


void
print_regs(cpuid_info *info)
{
	int i;
	char s[17] = {0};
	
	for (i = 0; i < 4; ++i)
		s[i]    = info->regs.eax >> (8*i),
		s[i+4]  = info->regs.ebx >> (8*i),
		s[i+8]  = info->regs.ecx >> (8*i),
		s[i+12] = info->regs.edx >> (8*i);
	
	printf("%s", s);
}

