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
# Specs come from a SPEC file, also used for library.
#
include ../Makefile_specs.mk

DEPDIR = depdir

# Define the program name

PROGNAME=UART_Test

# Source files
# List C/C++/Assembly source files:
# (list all files to compile, e.g. 'a.c b.cpp as.S'):
# Use .cc, .cpp or .C suffix for C++ files.
# Use .c              suffix for C   files.
# Use .S  (NOT .s !!!) for assembly source code files.

SOURCES= main.cpp uart.cpp 
# my_fputs.cpp

#
# Set a simpler LIB - NON Ardiuno
#
# For printf:
# -Wl,-u,vfprintf -lprintf_min      # Minimalist version
# -Wl,-u,vfprintf -lprintf_flt -lm  # Floating point version
# -Wl,-u,vfscanf -lscanf_min        # Minimalist scanf version
# -Wl,-u,vfscanf -lscanf_flt        # Floating point scanf version

LIBS =  -lc 
#
# Include all the common rules for AVR.
#
OTHER_DEFINES= -DUART_CTS_ENABLE=1 -DUART_USE_BUFFERS=1 -DTX_UART_BUF_SIZE=512 -DRX_UART_BUF_SIZE=64 
#OTHER_DEFINES= -DUART_USE_BUFFERS=1 -DTX_UART_BUF_SIZE=512 -DRX_UART_BUF_SIZE=64 

include ../Makefile_common.mk
