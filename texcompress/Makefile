CC = g++
CPPFLAGS += -Iinclude
LDLIBS = lib/FreeImage.lib

OBJS = main.o compress_texture.o

all: nds_texcompress
clean:
	$(RM) $(OBJS) nds_texcompress

nds_texcompress: $(OBJS)
	$(LINK.o) $^ $(LDLIBS) -o $@
