=========================================
PDFlib and PDFlib Lite source for iSeries
=========================================

How to compile PDFlib on iSeries (AS/400):

1. Copy all source files and directories from the "../libs"
   directory to a mapped network drive on your iSeries.


2. If you are running on an OS400 version < V5R0M0 the compiler
   wouldn't allow you to compile C source code directly from the
   IFS. In this case you need to copy all IFS source files into an
   iSeries source file/member. Make sure that you create these
   source files with CCSID 37 (US/Canada).

   CRTLIB LIB(PDFLIBSRC)

   CRTSRCPF FILE(PDFLIBSRC/QCSRC) RCDLEN(200) CCSID(37)

   CRTSRCPF FILE(PDFLIBSRC/H) RCDLEN(200) CCSID(37)

   CPYFRMSTMF FROMSTMF('/[SRCDIR]/libs/flate/adler32.c') 
   TOMBR('/qsys.lib/pdflibsrc.lib/qcsrc.file/adler32.mbr')


3. Compile all modules into one library. Use the following options
   for the CRTCMOD command:

   CRTCMOD SYSIFCOPT(*IFSIO) TERASPACE(*YES *TSIFC) STGMDL(*TERASPACE) 


   Warning: if you compile PDFlib without Teraspace support, you
   might get unpredictable results.


4.      Create the PDFlib Service Program with the following command:

   CRTLIB LIB(PDFLIB)

   CRTSRVPGM SRVPGM(PDFLIB/PDFLIB) MODULE(PDFLIBSRC/*ALL) EXPORT(*ALL) 
   STGMDL(*SNGLVL)
