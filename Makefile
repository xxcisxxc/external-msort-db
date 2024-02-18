CPP=g++
CPPOPT=-g -Og -D_DEBUG
# -O2 -Os -Ofast
# -fprofile-generate -fprofile-use
CPPFLAGS=$(CPPOPT) -Wall -ansi -pedantic
# -Wparentheses -Wno-unused-parameter -Wformat-security
# -fno-rtti -std=c++11 -std=c++98

# documents and scripts
DOCS=Tasks.txt
SCRS=

.PHONY : all trace list count clean

# default target
all : Test.exe compile_commands.json

# headers and code sources
HDRS=	defs.h \
		Iterator.h Scan.h Filter.h Sort.h
SRCS=	defs.cpp Assert.cpp Test.cpp \
		Iterator.cpp Scan.cpp Filter.cpp Sort.cpp

# compilation targets
OBJS=	defs.o Assert.o Test.o \
		Iterator.o Scan.o Filter.o Sort.o

# generate compile_commands.json for clangd
compile_commands.json : Makefile $(SRCS) $(HDRS)
	@echo -n > compile_commands.json
	@echo "[" >> compile_commands.json
	@for file in $(SRCS) ; do \
		echo -n "  { \"directory\": \"`pwd`\", \"command\": \"$(CPP) $(CPPFLAGS) -c $$file\", \"file\": \"$$file\" }," >> compile_commands.json ; \
		echo >> compile_commands.json ; \
	done
	@echo "{}" >> compile_commands.json
	@echo "]" >> compile_commands.json

# default target
#
Test.exe : Makefile $(OBJS)
	$(CPP) $(CPPFLAGS) -o Test.exe $(OBJS)

trace : Test.exe Makefile
	@date > trace
	./Test.exe >> trace
	@size -t Test.exe $(OBJS) | sort -r >> trace

$(OBJS) : Makefile defs.h
Test.o : Iterator.h Scan.h Filter.h Sort.h
Iterator.o Scan.o Filter.o Sort.o : Iterator.h
Scan.o : Scan.h
Filter.o : Filter.h
Sort.o : Sort.h

list : Makefile
	echo Makefile $(HDRS) $(SRCS) $(DOCS) $(SCRS) > list
count : list
	@wc `cat list`

clean :
	@rm -f $(OBJS) Test.exe Test.exe.stackdump trace compile_commands.json