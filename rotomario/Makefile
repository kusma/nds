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

LIBS     = -lpimpmobile -lgba

CPPFLAGS = -I$(DEVKITARM)/include -I$(LIBGBA)/include -Iinclude -O3
CFLAGS   = -mthumb-interwork -mlong-calls -O3
CXXFLAGS = -mthumb-interwork -mlong-calls -fconserve-space -fno-rtti -fno-exceptions
ASFLAGS  =  -mthumb-interwork -x assembler-with-cpp
LDFLAGS  = -mthumb-interwork -Wl,--gc-section -Wl,-Map,$(basename $@).map
LDFLAGS += -L$(LIBGBA)/lib -Llib $(LIBS)

ARM   = -marm
THUMB = -mthumb

VPATH = src

TARGET = example.gba
OBJS = \
	main.o \
	data.o
	
.PHONY: all clean

all: $(TARGET)

data.S : data/*

clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGET) $(TARGET:.gba=.elf) $(TARGET:.gba=.map)

$(TARGET:.gba=.elf): $(OBJS)

%.elf:
	$(LD) -specs=gba.specs $^ $(LDFLAGS) -o $@

%.gba: %.elf
	$(OBJCOPY) -O binary $< $@
	gbafix $@ -t$(basename $@)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(THUMB) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.iwram.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(ARM) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.iwram.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ARM) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)

%.o: %.S
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@ -MMD -MF $(@:.o=.d) 

-include $(OBJS:.o=.d)
