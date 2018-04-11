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
include Makefile_specs

DEPDIR = depdir

# Program name
PROGNAME=ProcSens

AVR_HOME=/Users/maurik/Documents/codes/MicroDev/AVR

# Source files
# List C/C++/Assembly source files:
# (list all files to compile, e.g. 'a.c b.cpp as.S'):
# Use .cc, .cpp or .C suffix for C++ files, use .S 
# (NOT .s !!!) for assembly source code files.
SOURCES=main.cpp CapSensor.cpp

# additional includes (e.g. -I/path/to/mydir)
INC= -I/Users/maurik/Documents/codes/MicroDev/AVR/include -I/Users/maurik/Documents/codes/MicroDev/AVR/include/variants/$(BOARD_TYPE)

# libraries to link in (e.g. -lmylib)
LIBS= -Wl,-u,vfprintf -lprintf_flt -lm -lc -L/Users/maurik/Documents/codes/MicroDev/AVR/lib/$(BOARD) -larduino  -larduinoutil
#LIBS= -Wl,-u,vfprintf -lprintf_flt -lm -lc 

##### ADDED Flags ####

CFLAGS   += -I. $(INC) 
CXXFLAGS += -I. $(INC)
ASMFLAGS += -I. $(INC)

# HEXFORMAT -- format for .hex file output
HEXFORMAT=ihex

##### automatic target names ####
TARGET=$(PROGNAME).out
DUMPTARGET=$(PROGNAME).s

HEXROMTARGET=$(PROGNAME).hex 
HEXTARGET=$(HEXROMTARGET) $(PROGNAME).ee.hex
GDBINITFILE=gdbinit-$(PROGNAME)

# Define all object files.

# Start by splitting source files by type
#  C++
CPPFILES=$(filter %.cpp, $(SOURCES))
CXXFILES +=$(filter %.cxx, $(SOURCES))
CCFILES=$(filter %.cc, $(SOURCES))
#  C
CFILES=$(filter %.c, $(SOURCES))
#  Assembly
ASMFILES=$(filter %.S, $(SOURCES))


# List all object files we need to create
OBJDEPS=$(CFILES:.c=.o)    \
	$(CXXFILES:.cxx=.o)\
	$(CPPFILES:.cpp=.o)\
	$(CCFILES:.cc=.o)  \
	$(ASMFILES:.S=.o)

# Define all lst files.
LST=$(filter %.lst, $(OBJDEPS:.o=.lst))

# All the possible generated assembly 
# files (.s files)
GENASMFILES=$(filter %.s, $(OBJDEPS:.o=.s)) 


.SUFFIXES : .c .cc .cpp .C .o .out .s .S \
	.hex .ee.hex .h .hh .hpp


# Make targets:
# all, disasm, stats, hex, writeflash/install, clean
all: $(DEPDIR) $(TARGET)


disasm: $(DUMPTARGET) stats

stats: $(TARGET)
	$(OBJDUMP) -h $(TARGET)
	$(SIZE) $(TARGET) 

hex: $(HEXTARGET)

burn: hex
# -V means don't verify burned code.
	$(AVRDUDE) -c $(AVRDUDE_PROGRAMMERID)  -V \
	 -p $(PROGRAMMER_MCU) -P $(AVRDUDE_PORT) -e $(AVRDUDE_EXTRA_FLAGS)   \
	 -U flash:w:$(HEXROMTARGET)

$(DUMPTARGET): $(TARGET) 
	$(OBJDUMP) -S  $< > $@


$(TARGET): $(OBJDEPS)  Makefile
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJDEPS) $(LIBS)

$(DEPDIR):
	mkdir  -p $(DEPDIR)
	echo "Directory is used for dependency files " > $(DEPDIR)/README

FORCE:

.PHONY:	all lib build elf hex eep lss sym program coff extcoff clean applet_files sizebefore sizeafter writeflash clean stats gdbinit stats

#### Generating assembly ####
# asm from C
%.s: %.c
	$(CC) -S $(CFLAGS) $< -o $@

# asm from (hand coded) asm
%.s: %.S
	$(CC) -S $(ASMFLAGS) $< > $@


# asm from C++
.cpp.s .cc.s .C.s :
	$(CXX) -S  $(CXXFLAGS) $< -o $@



#### Generating object files ####
# object from C
.c.o: 
	$(CC) $(CFLAGS) -c $< -o $@


# object from C++ (.cc, .cpp, .C files)
.cc.o .cpp.o .C.o :
	$(CXX) $(CXXFLAGS) -c $< -o $@

# object from asm
.S.o :
	$(CC) $(ASMFLAGS) -c $< -o $@

# Automatic dependencies
#%.d: %.c
#	$(CC) -M $(CFLAGS) $< | sed "s;$(notdir $*).o:;$*.o $*.d:;" > $@
#
#%.d: %.cpp
#	$(CXX) -M $(CXXFLAGS) $< | sed "s;$(notdir $*).o:;$*.o $*.d:;" > $@

CFLAGS +=  -Wp,-MD,$(DEPDIR)/$*.d
CXXFLAGS+=  -Wp,-MD,$(DEPDIR)/$*.d


#### Generating hex files ####
# hex files from elf
#####  Generating a gdb initialisation file    #####
.out.hex:
	$(OBJCOPY) --only-section .text  \
		--only-section .data    \
		--output-target $(HEXFORMAT) $< $@

.out.ee.hex:
	$(OBJCOPY) --only-section .eeprom            \
		--change-section-lma .eeprom=0 \
		--output-target $(HEXFORMAT) $< $@


#####  Generating a gdb initialisation file    #####
##### Use by launching simulavr and avr-gdb:   #####
#####   avr-gdb -x gdbinit-myproject           #####
gdbinit: $(GDBINITFILE)

$(GDBINITFILE): $(TARGET)
	@echo "file $(TARGET)" > $(GDBINITFILE)	
	@echo "target remote localhost:1212" \
		                >> $(GDBINITFILE)
	@echo "load"        >> $(GDBINITFILE) 
	@echo "break main"  >> $(GDBINITFILE)
	@echo "continue"    >> $(GDBINITFILE)
	@echo
	@echo "Use 'avr-gdb -x $(GDBINITFILE)'"


help:
	@echo "Make file for AVR programming."
	@echo ""
	@echo "make           -	build the *.out executable."
	@echo "make lib       - make the library in lib."
	@echo "make disasm    -	make a *.s disassembled file."
	@echo "make stats     - print the objects stats."
	@echo "make hex       -	make a *.hex file, ready for burning."
	@echo "make burn      -	burn the code using avrdude. "
	@echo " "
	@echo "make gdb       -	initialize gdb debugging"
	@echo " "
	@echo "make clean     -	cleanup"
	@echo "make distclean - really cleanup. "	 


#### Cleanup ####
clean:
	$(REMOVE) $(TARGET) $(TARGET).map $(DUMPTARGET)
	$(REMOVE) $(OBJDEPS)  
	$(REMOVE) $(addprefix $(DEPDIR)/,$(CFILES:.c=.d)  $(CXXFILES:.cxx=.d)  $(CPPFILES:.cpp=.d)  $(CCFILES:.cc=.d))
	$(REMOVE) $(LST) $(GDBINITFILE)
	$(REMOVE) $(GENASMFILES)
	$(REMOVE) $(HEXTARGET)


distclean: clean
	(cd lib/src; make distclean)

-include $(CFILES:%.c=$(DEPDIR)/%.d)
-include $(CPPFILES:%.cpp=$(DEPDIR)/%.d)
-include $(CXXFILES:%.cxx=$(DEPDIR)/%.d)
-include $(CCFILES:%.cc=$(DEPDIR)/%.d)

