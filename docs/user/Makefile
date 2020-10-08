define DEVELOP_CONFIG
HIDE_UNDOC_MEMBERS = NO
HIDE_UNDOC_CLASSES = NO
ENABLED_SECTIONS = INTERNAL
WARN_LOGFILE = "../../generated/doxygen/warnings.txt"
GENERATE_XML = YES
endef
export DEVELOP_CONFIG
default:
	doxygen
develop:
	(cat Doxyfile; echo "$$DEVELOP_CONFIG") | doxygen -