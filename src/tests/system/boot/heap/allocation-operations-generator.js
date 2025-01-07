/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

var fs = require('fs');

var file = fs.readFileSync(process.argv[2], {encoding: "utf-8"});
const lines = file.split("\r\n");
file = '';

console.log("static const AllocationOperation kOperations[] = {");

var live_allocations = {}, allocations_count = 0;
for (var i in lines) {
	const line = lines[i].split(/[ \(\),]/);
	if (line[0] == "malloc") {
		// [ 'malloc', '1024', '', '->', '0xffffffff82022f60' ]
		const size = line[1], address = line[4];
		live_allocations[address] = allocations_count++;
		console.log("\t{ OP_MALLOC, %s },", size);
	} else if (line[0] == "free") {
		// [ 'free', '0xffffffff9705e480', '' ]
		const address = line[1];
		if (parseInt(address) === 0)
			continue;
		let index = live_allocations[address];
		if (isNaN(parseInt(index)))
			continue;

		delete live_allocations[address];
		console.log("\t{ OP_FREE, %d },", index);
	} else if (line[0] == "realloc") {
		// [ 'realloc', '0x00101ae0', '', '192', '', '->', '0x00105268' ]
		const address = line[1], size = line[3], newAddress = line[6];
		if (parseInt(address) === 0) {
			live_allocations[newAddress] = allocations_count++;
			console.log("\t{ OP_MALLOC, %d },", size);
			continue;
		}
		let index = live_allocations[address];
		delete live_allocations[address];
		live_allocations[newAddress] = index;
		console.log("\t{ OP_REALLOC, %d },", index);
		console.log("\t{ 0, %d },", size);
	}
}

console.log("};");
console.log("static const uint32 kAllocationsCount = %d;\n", allocations_count);
