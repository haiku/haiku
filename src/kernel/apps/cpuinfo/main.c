#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpuinfo.h"
#include "flag_arrays.c"



extern char *__progname;
static cpuid_info CPU_Data;


void
usage()
{
	printf("usage: %s [-option]\n\n"
		"Prints information about your CPU.\n"
		"  -i   CPU identification\n"
		"  -f   supported processor features\n"
		"  -t   TLB and cache info\n"
		"  -d   dump registers from CPUID calls\n", __progname);

	exit(0);
}


char
getoption(char *optstr)
{
	if ((optstr[0] != '-')
		|| (strlen(optstr) > 2)
		|| (strchr("iftd", optstr[1]) == NULL))
		// invalid option
		usage();
	
	return optstr[1];
}


int
main(int argc, char *argv[])
{
	// this program prints out everything you would ever want to know about
	// your computer's processor, or your money back, in full (30 days, no hassle)
	//
	// technical reference:
	//    "Intel Processor Identification and the CPUID Instruction"
	//    (ftp://download.intel.com/design/Xeon/applnots/24161821.pdf)
	
	const char *no_support = "Sorry, your processor does not support this feature\n";
	
	char   option = 0;
	int    vendor_tag;
	uint32 max_level;
	cpuid_info *info = &CPU_Data;
	
	if (argc == 2)
		option = getoption(argv[1]);
	else
		usage();
	
	// get initial info (max_level and vendor_tag)
	get_cpuid(info, 0, 0);
	max_level  = info->eax_0.max_eax;
	vendor_tag = info->regs.ebx;
	
	switch (option) {
		case 'i':
			printf("CPU identification:\n\n");
			if (max_level < 1)
				printf(no_support);
			else
				opt_identify(info, vendor_tag);
			break;
		
		case 'f':
			printf("Supported processor features:\n\n");
			if (max_level < 1)
				printf(no_support);
			else
				opt_features(info, vendor_tag);
			break;
		
		case 't':
			printf("TLB and cache information:\n\n");
			if (max_level < 2)
				printf(no_support);
			else
				opt_TLB_cache(info, vendor_tag);
			break;
		
		case 'd':
			printf("CPUID instruction call dump:\n\n");
			opt_dump_calls(info, max_level);
			break;
	}

	return 0;
}


void
opt_identify(cpuid_info *info, int vendor_tag)
{
	// gosh, these CPU manufacturers have really outdone
	// themselves on creating cutesy vendorID strings, no?
	// (that was sarcasm, btw...)

	char vendorID[12+1] = {0};

	// the 'vendorid' field of the info struct is not null terminated,
	// so it is copied into a properly terminated local string buffer
	memcpy(vendorID, info->eax_0.vendorid, 12);
	printf("%12s '%s'\n", "Vendor ID:", vendorID);
	
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
	// the code for this function looks very boring and tedious
	// (that's because it's very boring and tedious)
	// but I think it's correct anyway (there's always that possibility!)
	
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
	
	if (family == 15) {
		int efamily = (info->regs.eax >> 20) & 0xff;
		printf("Extended family: %d\n", efamily);
	}
		
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
	// for each bit set in the features flag, print out the
	// corresponding index in the flags array (too easy!)
	
	int i;
	int fflags;
	
	get_cpuid(info, 1, 0);
	fflags = info->eax_1.features;
	
	for (i = 0; i < 32; ++i)
		if (fflags & (1<<i))
			printf("   %s\n", Intel_feature_flags[i]);
	
	printf("\n");
}


void
insert(int *a, int elem, int max_index)
{
	// inserts a new element into an ordered integer array
	// using (what else) the trusty old binary search algorithm...
	// assumes the array has sufficient space to do this
	// (i.e. it's the caller's problem to check for bounds overflow)
	
	int i;
	int lo = 0;
	int hi = max_index;
	int mid;

	// find insert index (will end up in lo)
	while (lo < hi) {
		mid = (lo + hi) / 2;
		if (elem < a[mid])
			hi = mid;
		else
			lo = mid + 1;
	}

	// bump up the max index (to make room for the new element)
	hi = max_index + 1;
	
	// move up all the items after the insert point by one
	for (i = hi; i > lo; --i)
		a[i] = a[i-1];
	
	// insert the new guy
	a[lo] = elem;
}


void
Intel_TLB_cache(cpuid_info *info)
{
	// displays technical info for the CPU's various instruction and data pipeline caches
	// and the TLBs (Translation Lookaside Buffers).
	//
	// The method for extracting and displaying this info might be less than obvious,
	// so here's an explanation:
	//
	// The TLB/Cache info is stored in a global table. Each entry is uniquely identified
	// by a descriptor byte. Additionally, each entry is catagorized according to the
	// cache type. A textual string contains the information to display.
	//
	// To retrieve the CPUs capabilities, the pass loop repeatedly (maybe) calls
	// get_cpuid() which fills the registers. Each register is a 4-byte datum,
	// capable of holding up to 4 descriptor bytes. The descriptor bytes are extracted
	// (thru shifting and masking) and then inserted into a slot array. The slot array
	// stores indexes into the table. Since descriptor bytes are not indexes themselves,
	// an index array is created and used to convert them. The slot array is filled so
	// as to keep the indexes in sorted order -- this guarantees than all entries for
	// a particular cache type will display together.
	//
	// After the pass loop is finished, the slot array contains the indexes of all the
	// table entries that apply to the host processor. Displaying the info is merely a matter
	// of spinning thru the slot array and printing the text. The output, however, is organized
	// by the cache types -- this technique *only* works because the TLB/Cache table has been
	// carefully arranged in that order.
	
	#define BIT_31_MASK (1 << 31)
	
	int    pass, matches;       // counters
	uint32 reg;                 // value of an individual register (eax, ebx, ecx, edx)
	uint8  db;                  // descriptor byte
	int    indexOf[256] = {0};  // converts descriptor bytes to table indexes
	int    slot[256] = {0};     // indexes into the TLB/Cache table (for entries found)
	
	tlbc_info *tab = Intel_TLB_Cache_Table;
	int   tabsize  = sizeof Intel_TLB_Cache_Table / sizeof(Intel_TLB_Cache_Table[0]);

	// fill the index conversion array
	int i = 0;
	int n = 0;

	while (n < tabsize)
		indexOf[tab[n++].descriptor] = i++;

	// pass loop: insert relevant table indexes into the slot array
	i = 0;
	for (pass = 0; ; ++pass) {
		get_cpuid(info, 2, 0);
		
		// low byte of eax register holds maximum iterations
		reg = info->regs.eax;
		if (pass >= (reg & 0xff))
			break;
		
		reg >>= 8; // skip low byte
		for (; reg; reg >>= 8)
			if ((db = (reg & 0xff)))
				insert(slot, indexOf[db], i++);
		
		reg = info->regs.ebx;  // ebx
		if ((reg & BIT_31_MASK) == 0)
			for (; reg; reg >>= 8)
				if ((db = (reg & 0xff)))
					insert(slot, indexOf[db], i++);

		reg = info->regs.ecx;  // ecx
		if ((reg & BIT_31_MASK) == 0)
			for (; reg; reg >>= 8)
				if ((db = (reg & 0xff)))
					insert(slot, indexOf[db], i++);

		reg = info->regs.edx;  // edx
		if ((reg & BIT_31_MASK) == 0)
			for (; reg; reg >>= 8)
				if ((db = (reg & 0xff)))
					insert(slot, indexOf[db], i++);
	}

	// reset slot index for output loop
	i = 0;
	
	// eeiuw! an icky macro (but it really does prevent much repetitive code)	
	#define display_matching_entries(type_name,type_code) \
		printf(type_name ":\n"); \
		matches = 0; \
		while (tab[n = slot[i]].cache_type == type_code) { \
			printf("   %s\n", tab[n].text); \
			++matches; \
			++i; \
		} \
		if (matches == 0) \
			printf("   None\n")
	
	// at long last... output the data
	display_matching_entries("Instruction TLB", Inst_TLB);
	display_matching_entries("Data TLB", Data_TLB);
	display_matching_entries("L1 Instruction Cache", L1_Inst_Cache);
	display_matching_entries("L1 Data Cache", L1_Data_Cache);
	display_matching_entries("L2 Cache", L2_Cache);
	
	if (tab[slot[i]].cache_type == No_L2_Or_L3)
		// no need to print the text on this one, just skip it
		++i;
	
	display_matching_entries("L3 Cache", L3_Cache);
	display_matching_entries("Trace Cache", Trace_Cache);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AMD specs

// Brennan says: I'm just digging around and trying to find AMD documentation
// on product signatures and CPU types...not easy to find. This is a dirty hack
// until I can find proper docs...

static const char *
AMD_brand_string(int family, int model, int processor_signature)
{
	int sig = processor_signature;
	
	switch (family) {
		//K5 and K6 models
		case 0x5: 
			if(model >= 0x0 && model <= 0x3) {
				return "AMD K5";
			}
			else if(model == 0x8){
				return "AMD K6-2";
			}
			else if(model == 0x9) {
				return "AMD K6-III";
			}
			else {
				return "AMD K6";
			}
		//Athlon and Duron models, very general
		case 0x6:
			//Hack.  I own one, so I know this is the processor signature!
			if(sig == 1634) {
				return "AMD Mobile Athlon";
			}
			else if(model == 0x1 || model == 0x2 || model == 0x4) {
				return "AMD Athlon";
			}
			else if(model == 0x3 ||model == 0x7) {
				return "AMD Duron";
			}
			else if(model == 0x6){
				return "AMD Duron/AMD Athlon XP/AMD Athlon MP";
			}
		case 0xf: 
			if(model == 0x5) {
				return "AMD Athlon 64";
			}
			if(model == 0x6) {
				return "AMD Opteron";
			}
			//This is the new 266FSB Duron model
			else if(model == 0x8) {
				return "AMD Duron (266 FSB)";
			}
		default:
			return "Unknown AMD Processor";
	}
}


void
AMD_identify(cpuid_info *info)
{
	int type, family, model, sig;
	const char *id_string;
	
	get_cpuid(info, 1, 0);
	type   = info->eax_1.type;
	family = info->eax_1.family;
	model  = info->eax_1.model;
	sig = info->regs.eax;
	
	id_string = AMD_brand_string(family, model, sig);
	
	printf("%s, Model: %d\n", id_string, model);
	printf("Family: %d\n", family);
	printf("Signature: %d\n", sig);
	
}


void
AMD_features(cpuid_info *info)
{
	printf("Sorry! Info about AMD CPU's not implemented yet :(\n");
}


void
AMD_TLB_cache(cpuid_info *info)
{
	printf("Sorry! Info about AMD CPU's not implemented yet :(\n");
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Cyrix specs


void
Cyrix_identify(cpuid_info *info)
{
	printf("Sorry! Info about Cyrix CPU's not implemented yet :(\n");
}


void
Cyrix_features(cpuid_info *info)
{
	printf("Sorry! Info about Cyrix CPU's not implemented yet :(\n");
}


void
Cyrix_TLB_cache(cpuid_info *info)
{
	printf("Sorry! Info about Cyrix CPU's not implemented yet :(\n");
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// dumps


void
opt_dump_calls(cpuid_info *info, uint32 max_level)
{
	// dumps the registers for standard (and if supported) extended
	// calls to the CPUID instruction

	uint32 max_extended;

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
	// gosh, it's so perty (in a text-based output kind of way)
	
	uint32 i;
	
	printf("%s (max eax level = %lx)\n", heading, max_level);
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
	printf("\n");
}


void
print_regs(cpuid_info *info)
{
	// this function either prints the contents of all the
	// registers as a single character string, or it does
	// something else entirely (you decide)
	
	int i;
	char s[17] = {0};
	
	for (i = 0; i < 4; ++i)
		s[i]    = info->regs.eax >> (8*i),
		s[i+4]  = info->regs.ebx >> (8*i),
		s[i+8]  = info->regs.ecx >> (8*i),
		s[i+12] = info->regs.edx >> (8*i);
	
	printf("%s", s);
}

