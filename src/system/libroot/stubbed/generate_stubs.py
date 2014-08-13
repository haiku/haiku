#! /usr/bin/env python

import sys;

dataSymbolsByAddress = {}
dataSymbols = []
versionedDataSymbolsByName = {}
functionSymbolsByAddress = {}
functionSymbols = []
versionedFunctionSymbolsByName = {}

for line in sys.stdin.readlines():
	if line[0] != '0':
		# ignore lines without an address
		continue

	(address, type, symbol) = line.split()

	# select interesting types of symbols
	if type not in 'BCDGRSTuVvWw':
		continue

	# drop symbols from legacy compiler that contain a dot (those produce
	# syntax errors)
	if '.' in symbol:
		continue

	if type in 'BD':
		if '@' in symbol:
			versionedDataSymbolsByName[symbol] = address
		else:
			if type not in 'VW':
				dataSymbolsByAddress[address] = symbol
			dataSymbols.append(symbol)
	else:
		if '@' in symbol:
			versionedFunctionSymbolsByName[symbol] = address
		else:
			if type not in 'VW':
				functionSymbolsByAddress[address] = symbol
			functionSymbols.append(symbol)

# add data symbols
for dataSymbol in sorted(dataSymbols):
	print 'int %s;' % dataSymbol

print

# add function symbols
for functionSymbol in sorted(functionSymbols):
	print 'void %s() {}' % functionSymbol

print
print "#include <symbol_versioning.h>"
print

# add symbol versioning information for data symbols
for symbol in sorted(versionedDataSymbolsByName.keys()):
	address = versionedDataSymbolsByName[symbol]
	targetSymbol = dataSymbolsByAddress[address]
	(symbolName, unused, versionTag) = symbol.partition('LIBROOT_')
	print 'DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("%s", "%s", "%s");'	\
		% (targetSymbol, symbolName, versionTag)

# add symbol versioning information for function symbols
for symbol in sorted(versionedFunctionSymbolsByName.keys()):
	address = versionedFunctionSymbolsByName[symbol]
	targetSymbol = functionSymbolsByAddress[address]
	(symbolName, unused, versionTag) = symbol.partition('LIBROOT_')
	print 'DEFINE_LIBROOT_KERNEL_SYMBOL_VERSION("%s", "%s", "%s");'	\
		% (targetSymbol, symbolName, versionTag)
