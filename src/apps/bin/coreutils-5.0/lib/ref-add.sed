/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ coreutils / coreutils /
  tb
  s/ $/ coreutils /
  :b
  s/^/# Packages using this file:/
}
