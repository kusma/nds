ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

DEVKITARM = $(DEVKITPRO)/devkitARM
LIBGBA    = $(DEVKITPRO)/libgba

PREFIX ?= arm-eabi-
CC      = $(PREFIX)gcc
CXX     = $(PREFIX)g++
OBJCOPY = $(PREFIX)objcopy
STRIP   = $(PREFIX)strip
LD      = $(PREFIX)g++
AS      = $(PREFIX)as
AR      = $(PREFIX)ar

LIBS     = -lgba

CPPFLAGS = -I$(DEVKITARM)/include -I$(LIBGBA)/include -Iinclude # -DNDEBUG
CFLAGS   = -mthumb-interwork -mlong-calls
CXXFLAGS = -mthumb-interwork -mlong-calls -fconserve-space -fno-rtti -fno-exceptions
ASFLAGS  =  -mthumb-interwork -x assembler-with-cpp
LDFLAGS  = -mthumb-interwork -Wl,--gc-section -Wl,-Map,$(basename $@).map
LDFLAGS += -L$(LIBGBA)/lib -Llib $(LIBS)

ARM   = -marm
THUMB = -mthumb

VPATH = src

OBJS = \
	libgbfs.o \
	gbfs_stdio.o

.PHONY: all clean test-clean example-clean

all: example.gba

clean: test-clean example-clean
	$(RM) $(OBJS) $(OBJS:.o=.d)

example.gbfs : example-data/test.txt example-data/dummy.txt example-data/file3.txt
example.elf : example.o $(OBJS)
example.gba: example.bin example.gbfs
example-clean:
	$(RM) example.gba example.bin example.elf example.o example.d example.map example.gbfs

test.gbfs : test-data/test.txt test-data/test2.txt
test.elf : test.o $(OBJS)
test.gba: example.bin test.gbfs
test-clean:
	$(RM) test.gba test.bin test.elf test.o test.d test.map test.gbfs


%_mb.elf:
	$(LD) -specs=gba_mb.specs $^ $(LDFLAGS) -o $@

%.elf:
	$(LD) -specs=gba.specs $^ $(LDFLAGS) -o $@

%.gbfs:
	gbfs $@ $^

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	padbin 256 $@
	gbafix $@ -t$(basename $@)

%.gba: %.bin
	cat $^ > $@

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d) 

-include $(OBJS:.o=.d)
