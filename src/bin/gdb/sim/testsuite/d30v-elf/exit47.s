# Verify r2 = 47; exit(r47) works
	add r2, r0, 47
	.long 0x0e000004
	nop
