/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ grep / grep /
  tb
  s/ $/ grep /
  :b
  s/^/# Packages using this file:/
}
