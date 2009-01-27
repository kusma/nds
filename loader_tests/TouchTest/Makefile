OUTPUT := test

# configure libnds
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif
DEVKITARM := $(DEVKITPRO)/devkitARM
LIBNDS	  := $(DEVKITPRO)/libnds

TARGET_BUILD_DIR ?= build
TARGET_PREFIX ?= arm-eabi-

define setup-gcc
	$(1)CC      = $(2)gcc
	$(1)CXX     = $(2)g++
	$(1)STRIP   = $(2)strip
	$(1)LD      = $(2)g++
	$(1)AS      = $(2)as
	$(1)AR      = $(2)ar
	$(1)OBJCOPY = $(2)objcopy
endef

# setup tools
$(eval $(call setup-gcc,TARGET_, $(TARGET_PREFIX)))
$(eval $(call setup-gcc,,))
MKDIR = mkdir -p

# setup target-flags
TARGET_CFLAGS   = -g -Wall -O3 -fomit-frame-pointer -mthumb -mthumb-interwork
TARGET_CXXFLAGS = -g -Wall -O3 -fomit-frame-pointer -mthumb -mthumb-interwork -fno-rtti -fno-exceptions
TARGET_CPPFLAGS = -I"$(LIBNDS)/include"
TARGET_LDFLAGS  = -g -mthumb-interwork -mno-fpu -Wl,-Map,$*.map -L"$(LIBNDS)/lib"

wram-filter = \
	$(subst $2,.o,        $(filter-out $(IWRAM_SOURCES) $(ITCM_SOURCES), $(filter %$2,$1))) \
	$(subst $2,.iwram.so, $(filter     $(IWRAM_SOURCES),                 $(filter %$2,$1))) \
	$(subst $2,.itcm.so,  $(filter     $(ITCM_SOURCES),                  $(filter %$2,$1)))

source-to-object = \
	$(call wram-filter, $1,.cpp) \
	$(call wram-filter, $1,.c) \
	$(call wram-filter, $1,.S)

source-to-depend = $(subst .o,.d, $(call source-to-object, $1))

make-target-objs = $(addprefix $(TARGET_BUILD_DIR)/, $(call source-to-object, $1))
make-target-deps = $(addprefix $(TARGET_BUILD_DIR)/, $(call source-to-depend, $1))

ARM7_SOURCES := \
	src/arm7/main.cpp \
	src/arm7/nds_loader_arm7.c

ARM9_SOURCES := \
	src/arm9/main.cpp \
	src/arm9/font.cpp \
	src/arm9/nds_loader_arm9.c \
	src/arm9/load_bin.S \
	src/arm9/fade.S \
	src/arm9/icon_folder.S \
	src/arm9/icon_slot1-cart.S

SOURCES = $(ARM7_SOURCES) $(ARM9_SOURCES)
IWRAM_SOURCES = 
ITCM_SOURCES = 

ARM7_OBJS = $(call make-target-objs, $(ARM7_SOURCES))
ARM9_OBJS = $(call make-target-objs, $(ARM9_SOURCES))

.PHONY: clean all

all: $(OUTPUT).nds $(OUTPUT).ds.gba

clean:
	$(RM) $(OUTPUT).nds $(OUTPUT).ds.gba $(OUTPUT).arm9.map
	$(RM) -r $(TARGET_BUILD_DIR)

$(TARGET_BUILD_DIR)/$(OUTPUT).arm9.elf : $(ARM9_OBJS)
$(TARGET_BUILD_DIR)/$(OUTPUT).arm7.elf : $(ARM7_OBJS)

%.ds.gba: %.nds
	dsbuild $<

%.nds: $(TARGET_BUILD_DIR)/%.arm9.bin $(TARGET_BUILD_DIR)/%.arm7.bin
	ndstool -c $@ -9 $< -7 $(<:.arm9.bin=.arm7.bin)
	dlditool.exe gmtf.dldi $@

%.bin : %.elf
	$(OBJCOPY) -O binary $< $@

define COMPILE.cpp
	@$(MKDIR) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)
endef

define COMPILE.c
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)
endef

define COMPILE.S
	@$(MKDIR) $(dir $@)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c $< -o $@ -MMD -MP -MF $(@:.o=.d)
endef

define COMPILE.s
	@$(MKDIR) $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@
endef


%.elf:
	@$(MKDIR) $(dir $@)
	$(CXX) $^ $(LDFLAGS) -o $@

# configure common tools
$(TARGET_BUILD_DIR)/%.o:   CC  = $(TARGET_CC)
$(TARGET_BUILD_DIR)/%.o:   AS  = $(TARGET_AS)
$(TARGET_BUILD_DIR)/%.o:   CXX = $(TARGET_CXX)
$(TARGET_BUILD_DIR)/%.elf: CXX = $(TARGET_CXX)
$(TARGET_BUILD_DIR)/%.bin: OBJCOPY = $(TARGET_OBJCOPY)

# configure common flags
$(TARGET_BUILD_DIR)/$(OUTPUT).arm9.elf: LDFLAGS  = -specs=ds_arm9.specs $(TARGET_LDFLAGS) -lfat -lnds9
$(TARGET_BUILD_DIR)/$(OUTPUT).arm7.elf: LDFLAGS  = -specs=ds_arm7.specs $(TARGET_LDFLAGS) -lnds7

$(ARM9_OBJS): CFLAGS   = $(TARGET_CFLAGS)   -mcpu=arm946e-s -mtune=arm946e-s
$(ARM9_OBJS): CXXFLAGS = $(TARGET_CXXFLAGS) -mcpu=arm946e-s -mtune=arm946e-s
$(ARM9_OBJS): CPPFLAGS = $(TARGET_CPPFLAGS) -DARM9

$(ARM7_OBJS): CFLAGS   = $(TARGET_CFLAGS)   -mcpu=arm7tdmi -mtune=arm7tdmi
$(ARM7_OBJS): CXXFLAGS = $(TARGET_CXXFLAGS) -mcpu=arm7tdmi -mtune=arm7tdmi
$(ARM7_OBJS): CPPFLAGS = $(TARGET_CPPFLAGS) -DARM7

# C++ rules
$(TARGET_BUILD_DIR)/%.o: %.cpp
	$(COMPILE.cpp)

# C rules
$(TARGET_BUILD_DIR)/%.o: %.c
	$(COMPILE.c)

# ASM rules
$(TARGET_BUILD_DIR)/%.o: %.S
	$(COMPILE.S)

# deps
-include $(call make-target-deps, $(SOURCES))
