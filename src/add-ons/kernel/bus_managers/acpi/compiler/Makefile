 

PROG=	iasl
SRCS=	aslcompilerparse.c aslcompilerlex.c aslanalyze.c aslcodegen.c \
	aslcompile.c aslerror.c aslfiles.c asllength.c \
	asllisting.c aslload.c asllookup.c aslmain.c aslmap.c aslopcodes.c \
	asloperands.c aslresource.c aslrestype1.c aslrestype2.c \
	asltree.c aslutils.c asltransform.c aslfold.c aslstubs.c aslopt.c \
	../common/getopt.c \
	../utilities/utalloc.c \
	../utilities/utcache.c \
	../utilities/utcopy.c \
	../utilities/utdebug.c \
	../utilities/utdelete.c \
	../utilities/utglobal.c \
	../utilities/utobject.c \
	../utilities/utmisc.c \
	../utilities/utmath.c \
	../utilities/utmutex.c \
	../utilities/utresrc.c \
	../utilities/utstate.c \
	../namespace/nsaccess.c \
	../namespace/nsalloc.c \
	../namespace/nsdump.c \
	../namespace/nsnames.c \
	../namespace/nsobject.c \
	../namespace/nsparse.c \
	../namespace/nssearch.c \
	../namespace/nsutils.c \
	../namespace/nswalk.c \
	../namespace/nsxfobj.c \
	../interpreter/parser/psargs.c \
	../interpreter/parser/psloop.c \
	../interpreter/parser/psopcode.c \
	../interpreter/parser/psparse.c \
	../interpreter/parser/psscope.c \
	../interpreter/parser/pstree.c \
	../interpreter/parser/psutils.c \
	../interpreter/parser/pswalk.c \
	../interpreter/dispatcher/dswscope.c \
	../interpreter/dispatcher/dswstate.c \
	../interpreter/dispatcher/dsfield.c \
	../interpreter/dispatcher/dsobject.c \
	../interpreter/dispatcher/dsopcode.c \
	../interpreter/dispatcher/dsutils.c \
	../interpreter/dispatcher/dswexec.c \
	../interpreter/dispatcher/dswload.c \
	../interpreter/executer/exconvrt.c \
	../interpreter/executer/excreate.c \
	../interpreter/executer/exdump.c \
	../interpreter/executer/exmisc.c \
	../interpreter/executer/exmutex.c \
	../interpreter/executer/exnames.c \
	../interpreter/executer/exoparg1.c \
	../interpreter/executer/exoparg2.c \
	../interpreter/executer/exoparg3.c \
	../interpreter/executer/exoparg6.c \
	../interpreter/executer/exprep.c \
	../interpreter/executer/exregion.c \
	../interpreter/executer/exresnte.c \
	../interpreter/executer/exresolv.c \
	../interpreter/executer/exresop.c \
	../interpreter/executer/exstore.c \
	../interpreter/executer/exstoren.c \
	../interpreter/executer/exstorob.c \
	../interpreter/executer/exsystem.c \
	../interpreter/executer/exutils.c \
	../common/adfile.c \
	../common/adisasm.c \
	../common/adwalk.c \
	../common/dmrestag.c \
	../common/dmtable.c \
	../common/dmtbinfo.c \
	../common/dmtbdump.c \
	../debugger/dbfileio.c \
	../disassembler/dmbuffer.c \
	../disassembler/dmnames.c \
	../disassembler/dmopcode.c \
	../disassembler/dmobject.c \
	../disassembler/dmresrc.c \
	../disassembler/dmresrcl.c \
	../disassembler/dmresrcs.c \
	../disassembler/dmutils.c \
	../disassembler/dmwalk.c \
	../tables/tbfadt.c \
	../tables/tbinstal.c \
	../tables/tbutils.c \
	../tables/tbxface.c \
	../osunixxf.c

NOMAN=	YES
CFLAGS+= -Wall -O2 -Wstrict-prototypes -D_LINUX -DACPI_ASL_COMPILER -I../include 

#YACC=	yacc
YACC=	bison
YFLAGS+= -v -d
LEX=	flex
LFLAGS+= -i

#.if $(YACC) == "bison"
YFLAGS+= -y -pAslCompiler
#.else
#CFLAGS+= -D_USE_BERKELEY_YACC
#.endif

aslmain : $(patsubst %.c,%.o, $(SRCS))
	$(CC) $(LDFLAGS) $(patsubst %.c,%.o, $(SRCS)) \
		$(LOADLIBES) $(LDLIBS) -o iasl

CLEANFILES= y.output y.tab.c y.tab.h aslcompiler.y.h \
	aslcompilerparse.c aslcompilerlex.c iasl

aslcompilerparse.c: aslcompiler.y
	${YACC} ${YFLAGS} aslcompiler.y
	cp y.tab.c aslcompilerparse.c
	cp y.tab.h aslcompiler.y.h

aslcompilerlex.c: aslcompiler.l
	${LEX} ${LFLAGS} -PAslCompiler -oaslcompilerlex.c aslcompiler.l

clean : 
	rm -f $(CLEANFILES) $(patsubst %.c,%.o, $(SRCS))

