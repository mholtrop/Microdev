#
# Usage:
#
# make
# make disasm 
# make stats 
# make hex
# make writeflash
# make gdbinit
# make clean
#
#
# Specs come from SPEC file, also used for library.
#
include ../Makefile_specs.mk

DEPDIR = depdir

# Program name
PROGNAME=LEDBall

# Source files
# List C/C++/Assembly source files:
# (list all files to compile, e.g. 'a.c b.cpp as.S'):
# Use .cc, .cpp or .C suffix for C++ files, use .S 
# (NOT .s !!!) for assembly source code files.
SOURCES=main.cpp uart.cpp NeoPixel.cpp LEDBall.cpp

# Simplify the libraries. No Arduino stuff. Minimal printf
LIBS = -Wl,-u,vfprintf -lprintf_min -lc


include ../Makefile_common.mk
