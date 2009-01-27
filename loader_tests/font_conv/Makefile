CC = g++
CPPFLAGS += -Iinclude
CCFLAGS = -g
LDLIBS = lib/FreeImage.lib

OBJS = font_conv.o

all: font_conv
clean:
	$(RM) $(OBJS) font_conv

font_conv: $(OBJS)
	$(LINK.o) $^ $(LDLIBS) -o $@
