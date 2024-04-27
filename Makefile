.PHONY : all trace count clean test

CPP=g++
CPPOPT=-O3 # -D_DEBUG
CPPFLAGS=$(CPPOPT) -Wall -Wextra -std=c++17 -I.

# documents and scripts
DOCS=Tasks.txt
SCRS=

# default target
all : ExternalSort.exe

# headers and code sources
HDRS=	defs.h \
		Iterator.h Scan.h Sort.h \
		Record.h Device.h SortFunc.h Consts.h \
		Utils.h Validate.h LoserTree.h
SRCS=	Iterator.cpp Scan.cpp Sort.cpp \
		SortFunc.cpp Validate.cpp

# compilation targets
OBJS=	Iterator.o Scan.o Sort.o \
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
	@rm -rf $(OBJS) ExternalSort.exe ExternalSort.exe.stackdump trace data
	@rm -f $(TEST_OBJS) $(TEST_DIR)/test_record $(TEST_LIBS)
