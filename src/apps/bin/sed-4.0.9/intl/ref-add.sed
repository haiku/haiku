/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ sed / sed /
  tb
  s/ $/ sed /
  :b
  s/^/# Packages using this file:/
}
