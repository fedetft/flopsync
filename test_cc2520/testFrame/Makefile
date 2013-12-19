##
## Makefile for Miosix np embedded OS
## TFT:Terraneo Federico Technlogies
##
MAKEFILE_VERSION := 1.01

##
## List here subdirectories which contains makefiles
##
SUBDIRS := 

##
## List here your source files (both .s, .c and .cpp)
##
# Please note that the file containing main() has to be on the same text line
# as the one with SRC or mkpackage.pl won't work
SRC := testFrameRunner.cpp \
testFrame.cpp \
../../drivers/frame.cpp  

##
## List here additional static libraries with relative path
##
LIBS :=

##
## List here additional include directories (in the form -Iinclude_dir)
##
INCLUDE_DIRS :=

##############################################################################
## You should not need to modify anything below                             ##
##############################################################################


## Replaces both "foo.cpp"-->"foo.o" and "foo.c"-->"foo.o"
OBJ := $(addsuffix .o, $(basename $(SRC)))

## Setting flags
CXXFLAGS  := $(CXXFLAGS_BASE) -c -Wall -std=c++11
CFLAGS    := $(CFLAGS_BASE) -c
AFLAGS    := $(AFLAGS_BASE)
LFLAGS    := $(LFLAGS_BASE)

LINK_LIBS := $(LIBS)  -lstdc++ -lc -lgcc -lcppunit

all: all-recursive test

clean: clean-recursive clean-topdir

program:
	$(PROGRAM_CMDLINE)

all-recursive:
	@for i in $(SUBDIRS); do  \
		$(MAKE) -C $$i FOLDER="$(FOLDER) $$i/" || exit 1;  \
	done

clean-recursive:
	@for i in $(SUBDIRS); do  \
		$(MAKE) -C $$i FOLDER="$(FOLDER) $$i/" clean  || exit 1;  \
	done

clean-topdir:
	-rm $(OBJ) test

test :  $(OBJ)
	@ echo "linking"
	$(CXX) $(LFLAGS) -o test $(OBJ) $(LINK_LIBS)

%.o: %.s
	$(AS) $(AFLAGS) $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@
