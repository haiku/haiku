/* Define a constant for the dgettext domainname for libc internal messages,
   so the string constant is not repeated in dozens of object files.  */

const char _libc_intl_domainname[] = "libc";
INTDEF(_libc_intl_domainname)
