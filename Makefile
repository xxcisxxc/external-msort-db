.PHONY : all trace count clean test

CPP=g++
CPPOPT=-g -O0 # -D_DEBUG
CPPFLAGS=$(CPPOPT) -Wall -Wextra -std=c++17

# documents and scripts
DOCS=Tasks.txt
SCRS=

# default target
all : test ExternalSort.exe

# headers and code sources
HDRS=	defs.h \
		Iterator.h Scan.h Filter.h Sort.h \
		Record.h Device.h SortFunc.h Consts.h \
		Utils.h Validate.h LoserTree.h
SRCS=	defs.cpp Assert.cpp \
		Iterator.cpp Scan.cpp Filter.cpp Sort.cpp \
		SortFunc.cpp Validate.cpp

# compilation targets
OBJS=	defs.o Assert.o \
		Iterator.o Scan.o Filter.o Sort.o \
		SortFunc.o Validate.o

ExternalSort.exe : Makefile $(OBJS) ExternalSort.cpp $(HDRS)
	$(CPP) $(CPPFLAGS) -o ExternalSort.exe ExternalSort.cpp $(OBJS)

trace : ExternalSort.exe Makefile
	@date > trace
	./ExternalSort.exe >> trace
	@size -t ExternalSort.exe $(OBJS) | sort -r >> trace

$(OBJS) : $(HDRS)

count :
	@wc Makefile $(HDRS) $(SRCS) $(DOCS) $(SCRS) | sort -n

TEST_DIR=tests
TEST_SRCS=$(TEST_DIR)/test_record.cpp $(TEST_DIR)/test_device.cpp $(TEST_DIR)/test_sort.cpp
TEST_OBJS=$(TEST_SRCS:.cpp=.o)
TEST_TARGETS=$(TEST_SRCS:.cpp=)
TEST_LIBS=catch2/catch_amalgamated.o

test : $(TEST_TARGETS)
	@for test in $(TEST_TARGETS) ; do \
		./$$test.out ; \
	done

$(TEST_TARGETS) : % : %.o $(TEST_LIBS) $(OBJS)
	$(CPP) $(CPPFLAGS) -o $@.out $@.o $(TEST_LIBS) $(OBJS)

$(TEST_OBJS) : catch2/catch_amalgamated.hpp $(HDRS) $(TEST_SRCS)
	$(CPP) $(CPPFLAGS) -c -o $@ $*.cpp -I.

$(TEST_LIBS) : catch2/catch_amalgamated.hpp

clean :
	@rm -f $(OBJS) ExternalSort.exe ExternalSort.exe.stackdump trace
	@rm -f $(TEST_OBJS) $(TEST_DIR)/test_record $(TEST_LIBS)

# generate compile_commands.json for clangd
# compile_commands.json : Makefile $(SRCS) $(HDRS)
# 	@echo -n > compile_commands.json
# 	@echo "[" >> compile_commands.json
# 	@for file in $(SRCS) ; do \
# 		if [ $$file = `echo $(SRCS) | awk '{print $$NF}'` ] ; then \
# 			echo -n "  { \"directory\": \"`pwd`\", \"command\": \"$(CPP) $(CPPFLAGS) -c $$file\", \"file\": \"$$file\" }" >> compile_commands.json ; \
# 		else \
# 			echo -n "  { \"directory\": \"`pwd`\", \"command\": \"$(CPP) $(CPPFLAGS) -c $$file\", \"file\": \"$$file\" }," >> compile_commands.json ; \
# 		fi ; \
# 		echo >> compile_commands.json ; \
# 	done
# 	@echo "]" >> compile_commands.json