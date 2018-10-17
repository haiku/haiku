; Copyright 2005-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
; Distributed under the terms of the MIT License.
;
; Stage 1 boot code for the good (?) old BIOS for use as boot block of HD
; partitions. The offset of the partition in 512 byte blocks must be written at
; position PARTITION_OFFSET_OFFSET (32 bit little endian; makebootable does
; that) or otherwise the code can't find the partition.
; The partition must be BFS formatted. The haiku_loader hpkg file must be located
; at "system/packages/haiku_loader*" and the haiku_loader.bios_ia32 must be the
; first file in the haiku_loader package.
;
; The bios_ia32 stage 2 boot loader is loaded into memory at 0x1000:0x0000
; (linear address 0x10000) and entered at 0x:1000:0x0200 with parameters
; eax - partition offset in 512 byte blocks and dl - BIOS ID of the boot drive.
;
; Compile via:
; nasm -f bin -O5 -o stage1.bin stage1.S

; 1 enables more informative error strings, that shouldn't be seen by the
; normal user, though.
%assign	DEBUG						0


; address/offset definitions

%assign	BOOT_BLOCK_START_ADDRESS	0x7c00
%assign	STACK_ADDRESS				BOOT_BLOCK_START_ADDRESS
%assign	READ_BUFFER_STACK			STACK_ADDRESS - 0x2000
%assign	PARTITION_OFFSET_OFFSET		506
%assign	BFS_SUPERBLOCK_OFFSET		512


; BFS definitions

%define SUPER_BLOCK_MAGIC1			'1SFB'		; nasm reverses '...' consts
%assign SUPER_BLOCK_MAGIC2			0xdd121031
%assign SUPER_BLOCK_MAGIC3			0x15b6830e

%assign	INODE_MAGIC1				0x3bbe0ad9

%assign	NUM_DIRECT_BLOCKS			12

%assign	S_IFMT						00000170000o
%assign	S_IFDIR						00000040000o


; BIOS calls

%assign	BIOS_VIDEO_SERVICES			0x10
%assign	BIOS_DISK_SERVICES			0x13
%assign	BIOS_KEYBOARD_SERVICES		0X16
%assign	BIOS_REBOOT					0X19

; video services
%assign	WRITE_CHAR						0x0e	; al - char
												; bh - page?

; disk services
%assign	READ_DISK_SECTORS				0x02	; dl	- drive
												; es:bx	- buffer
												; dh	- head (0 - 15)
												; ch	- track 7:0 (0 - 1023)
												; cl	- track 9:8,
												;		  sector (1 - 17)
												; al	- sector count
												; -> al - sectors read
%assign	READ_DRIVE_PARAMETERS			0x08	; dl - drive
												; -> cl - max cylinder 9:8
												;		- sectors per track
												;    ch - max cylinder 7:0
												;    dh - max head
												;    dl - number of drives (?)
%assign	CHECK_DISK_EXTENSIONS_PRESENT	0x41	; bx - 0x55aa
												; dl - drive
												; -> success: carry clear
												;    ah - extension version
												;    bx - 0xaa55
												;    cx - support bit mask
												; -> error: carry set
%assign	EXTENDED_READ					0x42	; dl - drive
												; ds:si - address packet
												; -> success: carry clear
												; -> error: carry set

%assign FIXED_DISK_SUPPORT				0x1		; flag indicating fixed disk
												; extension command subset

; keyboard services
%assign	READ_CHAR						0x00	; -> al - ASCII char
												;    ah - scan code


; nasm (0.98.38) doesn't seem to know pushad. popad works though.
%define	pushad		db	0x66, 0x60


; 16 bit code
SECTION .text
BITS 16
ORG BOOT_BLOCK_START_ADDRESS					; start code at 0x7c00

; nicer way to get the size of a structure
%define	sizeof(s)	s %+ _size

; using a structure in a another structure definition
%macro	nstruc	1-2		1
					resb	sizeof(%1) * %2
%endmacro

; 64 bit value
struc	quadword
	.lower			resd	1
	.upper			resd	1
endstruc

; address packet as required by the EXTENDED_READ BIOS call
struc	AddressPacket
	.packet_size	resb	1
	.reserved1		resb	1
	.block_count	resb	1
	.reserved2		resb	1
	.buffer			resd	1
	.offset			nstruc	quadword
;	.long_buffer	nstruc	quadword
	; We don't need the 64 bit buffer pointer. The 32 bit .buffer is more
	; than sufficient.
endstruc

; BFS block run
struc	BlockRun
	.allocation_group	resd	1
	.start				resw	1
	.length				resw	1
endstruc

; BFS superblock
struc	SuperBlock
	.name			resb	32
	.magic1			resd	1
	.fs_byte_order	resd	1
	.block_size		resd	1
	.block_shift	resd	1
	.num_blocks		nstruc	quadword
	.used_blocks	nstruc	quadword
	.inode_size		resd	1
	.magic2			resd	1
	.blocks_per_ag	resd	1
	.ag_shift		resd	1
	.num_args		resd	1
	.flags			resd	1
	.log_blocks		nstruc	BlockRun
	.log_start		nstruc	quadword
	.log_end		nstruc	quadword
	.magic3			resd	1
	.root_dir		nstruc	BlockRun
	.indices		nstruc	BlockRun
	.pad			resd	8
endstruc

; BFS inode data stream
struc	DataStream
	.direct						nstruc	BlockRun, NUM_DIRECT_BLOCKS
	.max_direct_range			nstruc	quadword
	.indirect					nstruc	BlockRun
	.max_indirect_range			nstruc	quadword
	.double_indirect			nstruc	BlockRun
	.max_double_indirect_range	nstruc	quadword
	.size						nstruc	quadword
endstruc

; BFS inode (shortened)
struc	BFSInode
	.magic1				resd	1
	.inode_num			nstruc	BlockRun
	.uid				resd	1
	.gid				resd	1
	.mode				resd	1
	.flags				resd	1
	.create_time		nstruc	quadword
	.last_modified_time	nstruc	quadword
	.parent				nstruc	BlockRun
	.attributes			nstruc	BlockRun
	.type				resd	1
	.inode_size			resd	1
	.etc				resd	1
	.data				nstruc	DataStream
	; ...
endstruc

; BFS B+ tree node
struc	BPlusTreeNode
	.left_link			nstruc	quadword
	.right_link			nstruc	quadword
	.overflow_link		nstruc	quadword
	.all_key_count		resw	1
	.all_key_length		resw	1
endstruc

; The beginning of a HPKG header
struc	HPKGHeader
	.magic				resd	1
	.header_size		resw	1
	; ...

; That's what esp points to after a "pushad".
struc	PushadStack
	.edi						resd	1
	.esi						resd	1
	.ebp						resd	1
	.tmp						resd	1	; esp
	.ebx						resd	1
	.edx						resd	1
	.ecx						resd	1
	.eax						resd	1
endstruc

; helper macro for defining static variables
%macro	define_static_var	1
	%define %1	static_variables + StaticVariables. %+ %1
%endmacro

; Structure containing the static variables we use (the ones that don't need
; pre-initialization at least). By using this structure we can easily place
; them onto our "heap".
struc	StaticVariables
	.bios_drive_id				resd	1
	.address_packet				nstruc	AddressPacket
	.write_int32_buffer			resb	32
	.inode						resb	512
	.indirect_block				resb	512
	.buffer						resb	1024
endstruc

; define short names for our static variables
define_static_var	bios_drive_id
define_static_var	write_int32_buffer
define_static_var	address_packet
define_static_var	inode
define_static_var	indirect_block
define_static_var	buffer

; Macro for defining a string prefixed by a byte containing its length. Used
; for the list of components of the path to the stage 2 boot loader.
%macro	pathComponent	1
	%strlen	.componentLen	%1
	db						.componentLen
	db						%1
%endmacro

; macro to be invoked in case of error -- parameter is the error string
%macro	error	1
	%if	DEBUG
		mov			si, %1
	%else
		mov			si, kErrorString
	%endif
	jmp			_error
%endmacro


start:
	; set ds, es, ss to the value of cs (which is 0x0000)
	push		cs
	pop			ds
	push		cs
	pop			es

	push		cs				; setup stack
	pop			ss
	mov			sp, STACK_ADDRESS

	cli							; disable interrupts
	cld							; clear direction flag (for string ops)

	; save the BIOS drive ID
	mov			[bios_drive_id], dl

	; check for BIOS int 0x13 (disk services) extensions
	mov			ah, CHECK_DISK_EXTENSIONS_PRESENT
	mov			bx, 0x55aa
	; dl has not changed yet, still contains the drive ID
	int			BIOS_DISK_SERVICES

	jc			.no_disk_extensions	; error

	cmp			bx, 0xaa55
	jne			.no_disk_extensions	; call not supported?

	test		cl, FIXED_DISK_SUPPORT
	jz			.no_disk_extensions

	; we have disk extensions
	mov	byte	[sHasDiskExtensions], 1

.no_disk_extensions:

	; read in our second half
	xor			eax, eax					; block offset (1) to eax
	inc			ax
	lea			ebp, [second_boot_block]	; buffer to ebp

	call		readOneBlock

	; check superblock magic
	cmp	dword	[superblock + SuperBlock.magic1], SUPER_BLOCK_MAGIC1
	je			.valid_superblock_magic

	error		kBadSuperBlockMagicString

.valid_superblock_magic:
	jmp			continueMain


; Jumped to in case of error. si must contain the pointer to the error string.
_error:
	call		_writeString

	; wait for a key and reboot
	mov			ah, READ_CHAR
	int			BIOS_KEYBOARD_SERVICES
	int			BIOS_REBOOT


; _writeString
; Prints a string to the screen.
; [entry]
; si:		pointer to null terminated string
; [exit]
; trashes:	si, ax
_writeString:
.loop:
	lodsb		; al <- [si++]
	or			al, al
	jz			.loopend

	mov			ah, WRITE_CHAR
	int			BIOS_VIDEO_SERVICES

	jmp			.loop
.loopend:

	ret


; readOneBlock
; Reads one disk block from the given offset into the specified buffer.
; [entry]
; eax:		block offset
; ebp:		buffer address
; [exit]
; trashes:	di
readOneBlock:
	mov			di, 1
	; fall through ...


; readBlocks
; Reads one or more disk blocks from the given offset into the specified buffer.
; [entry]
; eax:	block offset
; di:	block count
; ebp:	buffer address
readBlocks:
	pushad

	; add the partition offset
	add	dword	eax, [kPartitionOffset]

	; drive ID to dl
	mov			dl, [bios_drive_id]

	cmp	byte	[sHasDiskExtensions], 0
	jz			.no_extension_support

	; set packet size, block count, and buffer in the address packet
	mov	word	[address_packet + AddressPacket.packet_size], \
				sizeof(AddressPacket)
	mov			[address_packet + AddressPacket.block_count], di
	mov			[address_packet + AddressPacket.buffer], ebp

	; write the block offset to the address packet
	mov	dword	[address_packet + AddressPacket.offset + quadword.lower], eax
	xor			eax, eax
	mov	dword	[address_packet + AddressPacket.offset + quadword.upper], eax

	; do the "extended read" call
	; address packet address in si
	mov			si, address_packet
	mov			ah, EXTENDED_READ
	int			BIOS_DISK_SERVICES

	jnc			.no_error	; error?

.read_error:
	error		kReadErrorString

.no_extension_support:
	; no fixed disk extension support

	; save parameters
	push		eax

	; read drive parameters
	mov			ah, READ_DRIVE_PARAMETERS
	int			BIOS_DISK_SERVICES
	jc			.read_error

	; -> cl - max cylinder 9:8
	;		- sectors per track
	;    ch - max cylinder 7:0
	;    dh - max head

	; compute sectors
	pop			eax			; LBA
	xchg		dl, dh		; max head to dx
	xor			dh, dh		;
	push		dx			; save max head
	xor			edx, edx
	and			ecx, 0x3f	; filter sectors per track
	div			ecx			; divide by sectors per track

	inc			dl			; sector numbering starts with 1
	pop			cx			; restore max head
	push		dx			; save sector

	; compute heads and cylinders
	xor			dx, dx
	xor			ch, ch		; cl only
	inc			cx			; max head -> head count
	div			cx			; divide by head count
	; result: ax: cylinder, dx: head

	; we need to shuffle things a bit
	or			dh, dl		; head

	mov			cx, ax
	xchg		cl, ch		; ch: 7:0 cylinder
	ror			cl, 2		; cl: 9:8 cylinder in bits 7:6
	pop			dx			; restore sector
	or			cl, dl		; cl: 9:8 cylinder, 5:0 sector

	; buffer address must be in es:bx
	mov			eax, ebp	; count
	shr			eax, 16
	push		es			; save es
	push		ax
	pop			es
	mov			bx, bp

	; block count to ax
	mov			ax, di		; count

	mov			dl, [bios_drive_id]
	mov			ah, READ_DISK_SECTORS
	int			BIOS_DISK_SERVICES

	pop			es			; restore es

	cmp			ax, di
	jne			.read_error

.no_error:
	popad

	ret


; readBuffer
; Reads the next 1024 bytes from the data of the current inode (the one read
; with the last call to readInode) into a buffer. The function uses an own stack
; which it switches to at the beginning (switching back at the end). This
; custom stack is initially in an "after pushad" state. The functions does a
; popad at the beginning and pushad at the end, thus keeping its register
; configuration between calls (see some lines below for the meaning of the
; registers). readBufferInit must be called before the first call to readBuffer
; for a node. After that manipulating the parameters (registers) of the
; function can be done by accessing the respective values on the custom stack.
; [exit]
; ax:	0x0000 - nothing read (end of file/directory)
;		0x0001 - read one buffer
readBuffer:
	xor			ax, ax				; the default result (end)
	pushad

	; set our custom stack and get our registers
	xchg		sp, [sReadBufferStack]
	popad

	; registers:
	; eax		- read offset
	; ecx		- blocks left of the current block run
	; edx		- max remaining block runs
	; ebp		- pointer to the read buffer
	; si		- pointer to the current block run
	; edi		- index of the current indirect block
	; bx		- remaining indirect blocks

	and			ecx, ecx
	jg			.blocks_left_valid

	; blocks_left <= 0: decrement remaining block run index
	dec			dl
	jge			.next_block_run

.next_indirect_block:
	; no more block runs: decrement remaining indirect block count
	dec			bx
	jl			.nothing_read

	; there are more indirect blocks: read the next one
	; get the buffer address to esi first
	xor			esi, esi
	lea			si, [indirect_block]

	; read the block
	pushad
	xchg		eax, edi		; block index to eax
	mov			ebp, esi		; buffer pointer to ebp

	call		readOneBlock

	popad

	; increment the indirect block index
	inc			edi

	; maximal number of block runs in this block to edx
	xor			edx, edx
	mov			dl, 512 / sizeof(BlockRun)

.next_block_run:
	; convert block run to disk block offset
	call		blockRunToDiskBlock

	and			eax, eax
	jz			.next_indirect_block

.more_blocks:
	; compute blocks_left
	xchg		eax, ecx							; save eax
	movzx		eax, word [si + BlockRun.length]	; length to eax
	call		bfsBlockToDiskBlock					; convert
	xchg		eax, ecx							; blocks_left now in ecx

	; move to the next block run
	add			si, sizeof(BlockRun)

.blocks_left_valid:
	; we'll read 2 disk blocks: so subtract 2 from blocks_left
	sub			ecx, 2

	push		edi			; save edi -- we use it for other stuff for the
							; moment

	mov			di, 2

	call		readBlocks

	; adjust read_offset
	add			eax, 2

	; success
	mov			di, [sReadBufferStack]
	mov	byte	[di + PushadStack.eax], 1

	pop			edi			; restore edi

.nothing_read:
	pushad
	xchg		sp, [sReadBufferStack]
	popad
	ret


; readBufferInit
; Initializes readBuffer's context for the current inode (the one read with
; the last call to readInode). Must be called before the first call to
; readBuffer for an inode.
readBufferInit:
	; switch to readBuffer context
	pushad
	xchg		sp, [sReadBufferStack]
	popad

	; clear the number of indirect blocks, for the case there aren't any
	xor			bx, bx

	; check whether there are indirect blocks (max_indirect_range != 0)
	lea			si, [inode + BFSInode.data + DataStream.max_indirect_range]
	cmp	dword	[si], 0
	jnz			.has_indirect_blocks
	cmp	dword	[si + 4], 0
	jz			.no_indirect_blocks

.has_indirect_blocks:
	; get the first indirect block index
	lea			si, [inode + BFSInode.data + DataStream.indirect]
	call		blockRunToDiskBlock

	and			eax, eax
	jz			.no_indirect_blocks

	; indirect block index to edi
	xchg		edi, eax

	; get number of indirect blocks
	movzx		eax, word [si + BlockRun.length]
	call		bfsBlockToDiskBlock
	xchg		bx, ax							; number to bx

.no_indirect_blocks:

	; blocks_left = 0, max_remaining_direct_block_runs = NUM_DIRECT_BLOCKS
	xor			ecx, ecx
	mov			dl, NUM_DIRECT_BLOCKS

	; position si at the 1st block run
	lea			si, [inode + BFSInode.data + DataStream.direct]

	; buffer address
	xor			ebp, ebp
	mov			bp, buffer

	; switch context back (use readBuffer's code)
	jmp			readBuffer.nothing_read


; data

; the custom stack for readBuffer
sReadBufferStack			dw	READ_BUFFER_STACK - sizeof(PushadStack)
								; already start in "after pushad" state
sHasDiskExtensions			db	0	; 1 if the drive supports the extended read
									; BIOS call.

; error strings
; If DEBUG is enabled, we use more descriptive ones.
%if DEBUG
kReadErrorString			db	"read error", 0
kBadSuperBlockMagicString	db	"bad superblock", 0
kNotADirectoryString		db	"not a directory", 0
kBadInodeMagicString		db	"bad inode", 0
kNoZbeosString				db	"no loader", 0
%else
kErrorString				db	"bios_ia32 stage1: Failed to load OS. Press any key to reboot..."
							db	0
%endif

kPathComponents:
pathComponent				"system"
pathComponent				"packages"
kLastPathComponent:
pathComponent				"haiku_loader"
							db	0



first_code_part_end:

; The first (max) 512 - 4 -2 bytes of the boot code end here
; ---------------------------------------------------------------------------

				%if first_code_part_end - start > PARTITION_OFFSET_OFFSET
					%error "Code exceeds first boot code block!"
				%endif

				; pad to the partition offset
				%rep start + PARTITION_OFFSET_OFFSET - first_code_part_end
					db	0
				%endrep

kPartitionOffset			dd	0
kBootBlockSignature			dw	0xaa55


second_boot_block:

; first comes the BFS superblock
superblock:
				%rep sizeof(SuperBlock)
					db	0
				%endrep


; the second part of the boot block code

; readBootLoader
; Jumped to when all path components to the stage 2 boot loader have been
; resolved and the current inode is the one of the boot loader. The boot
; loader will be read into memory and jumped into.
readBootLoader:
	; prepare for the first call to readBuffer
	call		readBufferInit

	; the destination address: start at the beginning of segment 0x1000
	mov			ecx, 0x10000000
	mov			edi, ecx			; 0x1000:0000

	xor			ebx, ebx
	mov			bx, [sReadBufferStack]

.read_loop:
	; write destination address
	mov			[bx + PushadStack.ebp], edi

	; compute next destination address
	add			di, 1024
	jnc			.no_carry
	add			edi, ecx		; the lower 16 bit wrapped around: add 0x1000
								; (64 KB) to the segment selector
.no_carry:

	call		readBuffer

	; loop as long as reading succeeds
	and			ax, ax
	jnz			.read_loop

	; We have successfully read the complete boot loader package. The actual
	; code is located at the beginning of the package file heap which starts
	; directly after the header. Since the boot loader code expects to be
	; located at a fixed address, we have to move it there.

	; First set up the source and target segment registers. We keep the current
	; value also in bx, since it is easier to do arithmetics with it. We keep
	; the increment value for the segment registers in dx.
	mov			bx, 0x1000		; initial ds and es value
	push		bx
	pop			ds
	push		bx
	pop			es
	mov			dx, bx			; the increment value for the segment registers

	; Determine how much we have to copy and from where to where. si will be
	; the source and di the destination pointer (both segment relative) and
	; ecx is our byte count. To save instructions we only approximate the count.
	; It may be almost a full segment greater than necessary. But that doesn't
	; really harm.
	mov			ecx, edi
	shr			ecx, 12								; byte count
	xor			di, di								; target
	mov			ax, [di + HPKGHeader.header_size]	; header size (big endian)
	xchg		ah, al
	mov			si, ax								; source

.move_loop:
	movsb
	test		si, si
	jnz			.move_no_source_overflow
	add			bx, dx
	push		bx
	pop			ds
.move_no_source_overflow:
	test		di, di
	jnz			.move_no_destination_overflow
	push		bx
	pop			es
				; Using bx (the value of ds) is fine, since we assume that the
				; source is less than a segment ahead of the destination.
.move_no_destination_overflow:
	dec			ecx
	jnz			.move_loop

	; We have successfully prepared the boot loader code. Set up the
	; environment for it.
	; eax - partition offset in sectors
	push		cs
	pop			ds
				; temporarily reset ds, so we can access our variables
	mov dword	eax, [kPartitionOffset]

	; dl - BIOS drive ID
	xor			edx, edx
	mov			dl, [bios_drive_id]

	; Note: We don't have to set ds and es, since the stage 2 sets them
	; anyway before accessing any data.

	; enter the boot loader
	jmp			0x1000:0x200


; continueMain
; Continues the "main" function of the boot code. Mainly contains the loop that
; resolves the path to the stage 2 boot loader, jumping to readBootLoader when
; it was found.
continueMain:

	; load root node
	; convert root node block run to block
	lea			si, [superblock + SuperBlock.root_dir]
	call		blockRunToDiskBlock

	call		readInode

	; stack:
	; word		number of keys					(in .key_loop)
	; word		previous key length				(in .key_loop)

	; registers:
	; di	- path component					(in .search_loop)
	; cx	- path component length				(in .search_loop)
	; ax	- remaining key count (-1)			(in .key_loop)
	; bx	- key lengths array (current pos)	(in .key_loop)
	; dx	- previous absolute key length		(in .key_loop)
	; si	- current key						(in .key_loop)
	; bp	- current key length				(in .key_loop)

	lea			di, [kPathComponents]

.search_loop:
	; the path component we're looking for
	xor			cx, cx
	mov			cl, [di]	; the path component length
	inc			di			; the path component itself
	and			cl, cl
	jz			readBootLoader ; no more path components: We found the boot
								 ; loader! Now read it in.

.continue_search:
	; is a directory?
	mov			eax, [inode + BFSInode.mode]
	and			eax, S_IFMT
	cmp			eax, S_IFDIR
	je			.is_directory

	error		kNotADirectoryString

.is_directory:
	; prepare for the first call to readBuffer
	call		readBufferInit

	; we skip the first 1024 bytes (that's the b+tree header)
	call		readBuffer
	and			ax, ax
	jnz			.read_loop

.not_found:
	error		kNoZbeosString

.read_loop:

	; read the next B+ tree node
	call		readBuffer
	and			ax, ax
	jz			.not_found

	; we're only interested in leaf nodes (overflow_link == -1)
	xor			eax, eax
	dec			eax
	cmp			[buffer + BPlusTreeNode.overflow_link + quadword.lower], eax
	jne			.read_loop
	cmp			[buffer + BPlusTreeNode.overflow_link + quadword.upper], eax
	jne			.read_loop

	; get the keylengths and keys

	; the keys
	lea			si, [buffer + sizeof(BPlusTreeNode)]

	; the keylengths array
	mov			bx, [buffer + BPlusTreeNode.all_key_length]
	add			bx, sizeof(BPlusTreeNode) + 7
	and			bl, 0xf8
	add			bx, buffer

	; number of keys
	mov			ax, [buffer + BPlusTreeNode.all_key_count]
	push		ax

	; the "previous" key length
	push word	0

.key_loop:
	; while there are more keys
	dec			ax
	jl			.key_loop_end

	; get current key length
	mov			bp, [bx]

	; exchange previous key length on the stack and compute the actual
	; length (the key lengths array contains summed-up lengths)
	pop			dx
	push		bp
	sub			bp, dx

	; Compare the key length. For the last component we only want to check
	; whether the prefix matches.
	cmp			cx, bp
	jg			.skip_key

	cmp			di, kLastPathComponent + 1
	je			.compare_keys

	cmp			cx, bp
	jne			.skip_key

	; compare path component (di) with key (si), length cx (<= bp)
.compare_keys:
	pusha
	repe cmpsb
	popa

	jne			.skip_key

	; keys are equal

	; get the current index
	pop			dx			; pop previous key length
	pop			dx			; key count
	inc			ax			; ax is decremented already at the loop top
	sub			dx, ax		; the current index

	; get to the end of the key lengths array
	shl			ax, 1		; number of bytes remaining in the array
	add			bx, ax
	shl			dx, 3		; offset in the value (block number) array
	add			bx, dx		; bx now points to the block number of the inode

	; read the block offset and load the Inode
	mov			eax, [bx]
	call		bfsBlockToDiskBlock

	call		readInode

	; next path component
	add			di, cx
	jmp			.search_loop

.skip_key:
	inc			bx			; next key length
	inc			bx
	add			si, bp		; next key
	jmp			.key_loop

.key_loop_end:
	; all keys check, but nothing found: need to read in the next tree node
	pop			dx			; pop previous key length
	pop			dx			; pop key count
	jmp			.read_loop


; readInode
; Reads the inode at the specified disk block offset into the buffer "inode".
; [entry]
; eax:	disk block offset
readInode:
	pushad

	; buffer address to ebp
	xor			ebp, ebp
	mov			bp, inode

	; An inode is actually one BFS block big, but we're interested only in the
	; administrative part (not the small data section), which easily fits into
	; one disk block.
	call		readOneBlock

	cmp	dword	[inode + BFSInode.magic1], INODE_MAGIC1
	je			.no_error

	error		kBadInodeMagicString

.no_error:
	popad

	ret


; blockRunToDiskBlock
; Computes the start address (in disk blocks) of a given BFS block run.
; [entry]
; si:	pointer to the BlockRun
; [exit]
; eax:	disk block number, where the block run begins
blockRunToDiskBlock:
	push		ecx

	; run.allocation_group << superblock.ag_shift
	mov			cl, [superblock + SuperBlock.ag_shift]
	mov 		eax, [si + BlockRun.allocation_group]
	shl			eax, cl

	; add run.start
	xor			ecx, ecx
	mov			cx, [si + BlockRun.start]

	add			eax, ecx

	pop			ecx

	; Fall through to bfsBlockToDiskBlock, which will convert the BFS block
	; number to a disk block number and return to our caller.


; bfsBlockToDiskBlock
; Converts a BFS block number to a disk block number.
; [entry]
; eax:	BFS block number
; [exit]
; eax:	disk block number
bfsBlockToDiskBlock:
	push		cx

	;   1 BFS block == superblock_block_size / 512 disk blocks
	mov byte	cl, [superblock + SuperBlock.block_shift]
	sub			cl, 9

	shl			eax, cl

	pop			cx
	ret


%if 0
; _writeInt32
; Writes the given number in 8 digit hexadecimal representation to screen.
; Used for debugging only.
; [entry]
; eax:	The number to print.
_writeInt32:
	pushad

	mov			bx, write_int32_buffer
	mov byte	[bx + 8], 0	; terminating null
	mov			di, 7

.loop:
	; get the lowest hex digit
	mov			dx, ax
	and			dl, 0xf

	; convert hex digit to character
	cmp			dl, 10
	jl			.digit
	add			dl, 'a' - '0' - 10

.digit:
	add			dl, '0'

	; prepend the digit to the string
	mov			[bx + di], dl

	; shift out lowest digit and loop, if there are more digits
	shr			eax, 4
	dec			di
	jge			.loop

	; write the composed string
	xchg		bx, si
	call		_writeString

	popad
	ret
%endif


; check whether we are small enough
end:
				%if end - start > 1024
					%error "Code exceeds second boot code block!"
				%endif

; pad to 1024 bytes size
				%rep start + 1024 - end
					db	0
				%endrep

; Base offset for static variables.
static_variables:
