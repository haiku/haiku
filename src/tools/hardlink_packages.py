#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Hardlink only packages used in the build from one directory to another,
# and updates the RemotePackageRepository file at the same time.
#
# Copyright 2017-2020 Augustin Cavalier <waddlesplash>
# Distributed under the terms of the MIT License.

import sys, os, subprocess, re, hashlib
from distutils.version import LooseVersion

if len(sys.argv) != 5:
	print("usage: hardlink_packages.py [arch] [jam RemotePackageRepository file] "
		+ "[prebuilt packages directory] [destination root directory]")
	print("note that the [jam RemotePackageRepository file] will be modified.")
	print("note that [target directory] is assumed to have a 'packages' subdirectory, "
		+ " and a repo.info.template file (using $ARCH$)")
	sys.exit(1)

if subprocess.run(['package_repo'], None, None, None,
		subprocess.DEVNULL, subprocess.PIPE).returncode != 1:
	print("package_repo command does not seem to exist.")
	sys.exit(1)

args_arch = sys.argv[1]
args_jamf = sys.argv[2]
args_src = sys.argv[3]
args_dst = sys.argv[4]

if not args_dst.endswith('/'):
	args_dst = args_dst + '/'
if not args_src.endswith('/'):
	args_src = args_src + '/'

args_dst_packages = args_dst + 'packages/'

packageVersions = []
for filename in os.listdir(args_src):
	if (not (filename.endswith("-" + args_arch + ".hpkg")) and
			not (filename.endswith("-any.hpkg"))):
		continue
	packageVersions.append(filename)

# Read RemotePackageRepository file and hardlink relevant packages
pattern = re.compile("^[a-z0-9]")
newFileForJam = []
packageFiles = []
filesNotFound = False
with open(args_jamf) as f:
	for line in f:
		pkg = line.strip()
		if (len(pkg) == 0):
			continue
		if not (pattern.match(pkg)):
			# not a package (probably a Jam directive)
			newFileForJam.append(line)
			continue

		try:
			pkgname = pkg[:pkg.index('-')]
		except:
			pkgname = ''
		if (len(pkgname) == 0):
			# no version, likely a source/debuginfo listing
			newFileForJam.append(line)
			continue

		greatestVersion = None
		for pkgVersion in packageVersions:
			if (pkgVersion.startswith(pkgname + '-') and
					((greatestVersion == None)
						or (LooseVersion(pkgVersion) > LooseVersion(greatestVersion)))):
				greatestVersion = pkgVersion
		if (greatestVersion == None):
			print("not found: " + pkg)
			newFileForJam.append(line)
			filesNotFound = True
			continue
		else:
			# found it, so hardlink it
			if not (os.path.exists(args_dst_packages + greatestVersion)):
				os.link(args_src + greatestVersion, args_dst_packages + greatestVersion)
			if ('packages/' + greatestVersion) not in packageFiles:
				packageFiles.append('packages/' + greatestVersion)
			# also hardlink the source package, if one exists
			srcpkg = greatestVersion.replace("-" + args_arch + ".hpkg",
				"-source.hpkg").replace('-', '_source-', 1)
			if os.path.exists(args_src + srcpkg):
				if not os.path.exists(args_dst_packages + srcpkg):
					os.link(args_src + srcpkg, args_dst_packages + srcpkg)
				if ('packages/' + srcpkg) not in packageFiles:
					packageFiles.append('packages/' + srcpkg)
		newFileForJam.append("\t" + greatestVersion[:greatestVersion.rfind('-')] + "\n");

if filesNotFound:
	sys.exit(1)

finalizedNewFile = "".join(newFileForJam).encode('UTF-8')
with open(args_jamf, 'wb') as f:
	f.write(finalizedNewFile)

listhash = hashlib.sha256(finalizedNewFile).hexdigest()
try:
	os.mkdir(args_dst + listhash)
except:
	print("dir " + listhash + " already exists. No changes?")
	sys.exit(1)

repodir = args_dst + listhash + '/'
os.symlink('../packages', repodir + 'packages')

with open(args_dst + 'repo.info.template', 'r') as ritf:
	repoInfoTemplate = ritf.read()

repoInfoTemplate = repoInfoTemplate.replace("$ARCH$", args_arch)
with open(repodir + 'repo.info', 'w') as rinf:
	rinf.write(repoInfoTemplate)

packageFiles.sort()
with open(repodir + 'package.list', 'w') as pkgl:
	pkgl.write("\n".join(packageFiles))

if os.system('cd ' + repodir + ' && package_repo create repo.info ' + " ".join(packageFiles)) != 0:
	print("failed to create package repo.")
	sys.exit(1)

if os.system('cd ' + repodir + ' && sha256sum repo >repo.sha256') != 0:
	print("failed to checksum package repo.")
	sys.exit(1)
