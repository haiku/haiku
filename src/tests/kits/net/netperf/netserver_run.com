$ define sys$output sys$sysroot:[netperf]netperf.log 
$ define sys$error  sys$sysroot:[netperf]netperf.log 
$ run sys$sysroot:[netperf]netserver.exe 