
// cache-type codes
enum cache_types {
	Unused,
	Inst_TLB,
	Data_TLB,
	L1_Inst_Cache,
	L1_Data_Cache,
	L2_Cache,
	L3_Cache,
	No_L2_Or_L3,
	Trace_Cache
};


// an entry in the TLB/Cache table
typedef struct {
	uint8  descriptor;
	uint8  cache_type;
	char   text[70];
} tlbc_info;


//
// the global TLB/Cache table
// technical reference:
//    "Intel Processor Identification and the CPUID instruction"
//    (ftp://ftp.intel.com/docs/24161821.pdf)
//     Page 18

static tlbc_info
Intel_TLB_Cache_Table[] = {
	{0x0,  0, ""},
	{0x1,  Inst_TLB, "4KB pages, 4-way set associative, 32 entries"},
	{0x2,  Inst_TLB, "4MB pages, fully associative, 2 entries"},
	{0x50, Inst_TLB, "4KB, 2MB or 4MB pages, fully associative, 64 entries"},
	{0x51, Inst_TLB, "4KB, 2MB or 4MB pages, fully associative, 128 entries"},
	{0x52, Inst_TLB, "4KB, 2MB or 4MB pages, fully associative, 256 entries"},
	
	{0x3,  Data_TLB, "4KB pages, 4-way set associative, 64 entries"},
	{0x4,  Data_TLB, "4MB pages, 4-way set associative, 8 entries"},
	{0x5b, Data_TLB, "4KB or 4MB pages, fully associative, 64 entries"},
	{0x5c, Data_TLB, "4KB or 4MB pages, fully associative, 128 entries"},
	{0x5d, Data_TLB, "4KB or 4MB pages, fully associative, 256 entries"},
	
	{0x6,  L1_Inst_Cache, "8KB pages, 4-way set associative, 32 byte line size"},
	{0x8,  L1_Inst_Cache, "16KB pages, 4-way set associative, 32 byte line size"},
	
	{0xa,  L1_Data_Cache, "8KB pages, 2-way set associative, 32 byte line size"},
	{0xc,  L1_Data_Cache, "16KB pages, 4-way set associative, 32 byte line size"},
	{0x66, L1_Data_Cache, "8KB pages, 4-way set associative, 64 byte line size"},
	{0x67, L1_Data_Cache, "16KB pages, 4-way set associative, 64 byte line size"},
	{0x68, L1_Data_Cache, "32KB pages, 4-way set associative, 64 byte line size"},
	
	{0x39, L2_Cache, "128KB pages, 4-way set associative, sectored, 64 byte line size"},
	{0x3c, L2_Cache, "256KB pages, 4-way set associative, sectored, 64 byte line size"},
	{0x41, L2_Cache, "128KB pages, 4-way set associative, 32 byte line size"},
	{0x42, L2_Cache, "256KB pages, 4-way set associative, 32 byte line size"},
	{0x43, L2_Cache, "512KB pages, 4-way set associative, 32 byte line size"},
	{0x44, L2_Cache, "1MB pages, 4-way set associative, 32 byte line size"},
	{0x45, L2_Cache, "2MB pages, 4-way set associative, 32 byte line size"},
	{0x79, L2_Cache, "128KB pages, 8-way set associative, sectored, 64 byte line size"},
	{0x7a, L2_Cache, "256KB pages, 8-way set associative, sectored, 64 byte line size"},
	{0x7b, L2_Cache, "512KB pages, 8-way set associative, sectored, 64 byte line size"},
	{0x7c, L2_Cache, "1MB pages, 8-way set associative, sectored, 64 byte line size"},
	{0x82, L2_Cache, "256KB pages, 8-way set associative, 32 byte line size"},
	{0x83, L2_Cache, "512KB pages, 8-way set associative, 32 byte line size"},
	{0x84, L2_Cache, "1MB pages, 8-way set associative, 32 byte line size"},
	{0x85, L2_Cache, "2MB pages, 8-way set associative, 32 byte line size"},
	
	{0x40, No_L2_Or_L3, "No 2nd-level cache, or if 2nd-level cache exists, no 3rd-level cache\n"},

	{0x22, L3_Cache, "512KB pages, 4-way set associative, sectored, 64 byte line size"},
	{0x23, L3_Cache, "1MB pages, 8-way set associative, sectored, 64 byte line size"},
	{0x25, L3_Cache, "2MB pages, 8-way set associative, sectored, 64 byte line size"},
	{0x29, L3_Cache, "4MB pages, 8-way set associative, sectored, 64 byte line size"},

	{0x70, Trace_Cache, "Trace cache: 12K-uops, 8-way set associative\n"},
	{0x71, Trace_Cache, "Trace cache: 16K-uops, 8-way set associative\n"},
	{0x72, Trace_Cache, "Trace cache: 32K-uops, 8-way set associative\n"},
};




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
char getoption (char *);
void insert (int *, int, int);
void opt_identify (cpuid_info *, int);
void opt_features (cpuid_info *, int);
void opt_TLB_cache (cpuid_info *, int);
void opt_dump_calls (cpuid_info *, uint32);
void print_regs (cpuid_info *);
void usage (void);
