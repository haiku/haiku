
# settings for debug builds:
#CXXFLAGS = -g -O0 -DDEBUG -Wall -Wno-multichar

# settings for release builds:
CXXFLAGS = -O2 -Wall -Wno-multichar -Wno-sign-compare -Wno-unused

all: rc librdef.so

parser.cpp: parser.y
	bison -d -o$@ $^

lexer.cpp: lexer.l parser.cpp
	flex -o$@ lexer.l

librdef.so: compile.cpp decompile.cpp lexer.cpp parser.cpp rdef.cpp
	$(CXX) $(CXXFLAGS) -D_BUILDING_RDEF -lbe -lstdc++.r4 -o $@ $^ -nostart -Xlinker -soname=$@
	mimeset -f $@

rc: rc.cpp librdef.so
	$(CXX) $(CXXFLAGS) -L./ -lrdef -o $@ rc.cpp
	mimeset -f $@

clean:
	rm -f *.o rc librdef.so lexer.cpp parser.cpp parser.cpp.h

install:
	cp rc ~/config/bin
	cp librdef.so ~/config/lib
