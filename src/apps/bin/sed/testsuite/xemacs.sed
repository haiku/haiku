# Inspired by xemacs' config.status script
# submitted by John Fremlin (john@fremlin.de)

/^# Generated/d
s%/\*\*/#.*%%
s/^ *# */#/
/^##/d
/^#/ {
  p
  d
}
/./ {
  s/\([\"]\)/\\\1/g
  s/^/"/
  s/$/"/
}
