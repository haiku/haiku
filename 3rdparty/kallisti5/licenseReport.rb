#!/bin/env ruby
#
# Source Code License Guesser.
# Copyright, 2017 Alexander von Gluck IV. All Rights Reserved
# Released under the terms of the MIT license.
#
# Give a file, and I guess the license.
#
# Example Usage:
# 	
# 	haiku $ find src -name "*.cpp" -exec ./3rdparty/kallisti5/licenseReport.rb {} \;
#

@file = ARGV.first
@licenses = [
	{"MIT" => ["MIT License", "MIT Licence", "Haiku License", "X11 license"]},
	{"BSD" => ["express or implied warranties", "provided by the author ``AS IS''", "the software is provided \"AS IS\"", "BSD license", "provided by the author \"as is\""]},
	{"BeOS Sample Code" => ["be sample code license"]},
	{"LGPL" => ["GNU Lesser", "GNU L-GPL license"]},
	{"GPL" => ["terms of the GNU General Public License", "GPL license", "Free Software Foundation"]},
]

def check_license(filename)
	license = "unknown"
	lines = File.foreach(filename).first(30).join("\n")
	return "empty file" if lines == nil
	@licenses.each do |entry|
		entry.values.first.each do |pattern|
			if lines.downcase.include?(pattern.downcase)
				license = entry.keys.first
				break
			end
		end
		break if license != "unknown"
	end
	license
end

puts "#{@file}: #{check_license(@file)}"
