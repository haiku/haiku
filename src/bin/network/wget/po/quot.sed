# For quotearg:
s/^`$/“/
s/^'$/”/

s/"\([^"]*\)"/“\1”/g
s/`\([^`']*\)'/‘\1’/g
s/ '\([^`']*\)' / ‘\1’ /g
s/ '\([^`']*\)'$/ ‘\1’/g
s/^'\([^`']*\)' /‘\1’ /g
s/“”/""/g

# At least in all of our current strings, ' should be ’.
s/'/’/g
# Special: write Hrvoje’s last name properly.
s/Niksic/Nikšić/g
s/opyright (C)/opyright ©/g
