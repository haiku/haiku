# For quotearg:
s/^`$/“[1m/
s/^'$/”[0m/

s/"\([^"]*\)"/“\1”/g
s/`\([^`']*\)'/‘\1’/g
s/ '\([^`']*\)' / ‘\1’ /g
s/ '\([^`']*\)'$/ ‘\1’/g
s/^'\([^`']*\)' /‘\1’ /g
s/“”/""/g
s/“/“[1m/g
s/”/[0m”/g
s/‘/‘[1m/g
s/’/[0m’/g

# At least in all of our current strings, ' should be ’.
s/'/’/g
# Special: write Hrvoje’s last name properly.
s/Niksic/Nikšić/g
s/opyright (C)/opyright ©/g
