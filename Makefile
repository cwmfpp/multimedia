OBJC = src/demo.c src/media_video_h264.c src/media_video_h265.c src/media_svc.c src/media_psmux.c src/media_mpegenc.c src/media_fileops.c src/media_psfile.c src/media_mpegdec.c src/media_psdemux.c src/media_rtppacket.c src/media_tsmux.c
INC = include/demo.h
INC_DIR = include
OBJ = $(OBJC:%c=%o)

OBJ_LIBNAME = libmultimedia
OBJ_TEST = multimedia_test

OBJ_TEST_SRC = src/demo.c src/test.c

LDLIBS = -lm -pthread

COMPILER_DIR = out

LIBVERSION = 0.0.1
MULTIMEDIA_SOVERSION = 1

MULTIMEDIA_SO_LDFLAG=-Wl,-soname=$(OBJ_LIBNAME).so.$(MULTIMEDIA_SOVERSION)

PREFIX ?= /usr/local
INCLUDE_PATH ?= include/multimedia
LIBRARY_PATH ?= lib

INSTALL_INCLUDE_PATH = $(DESTDIR)$(PREFIX)/$(INCLUDE_PATH)
INSTALL_LIBRARY_PATH = $(DESTDIR)$(PREFIX)/$(LIBRARY_PATH)

INSTALL ?= cp -a

# validate gcc version for use fstack-protector-strong
MIN_GCC_VERSION = "4.9"
GCC_VERSION := "`$(CC) -dumpversion`"
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
    CFLAGS += -fstack-protector-strong
else
    CFLAGS += -fstack-protector
endif

# -std=c99 -pedantic -Wundef -Wconversion 

R_CFLAGS = -fPIC -Wall -Werror -Wstrict-prototypes -Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2 -Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wc++-compat -Wswitch-default -Os $(CFLAGS)

uname := $(shell sh -c 'uname -s 2>/dev/null || echo false')

#library file extensions
SHARED = so
STATIC = a

## create dynamic (shared) library on Darwin (base OS for MacOSX and IOS)
ifeq (Darwin, $(uname))
	SHARED = dylib
	MULTIMEDIA_SO_LDFLAG = ""
endif

#multimedia library names
MULTIMEDIA_SHARED = $(OBJ_LIBNAME).$(SHARED)
MULTIMEDIA_SHARED_VERSION = $(OBJ_LIBNAME).$(SHARED).$(LIBVERSION)
MULTIMEDIA_SHARED_SO = $(OBJ_LIBNAME).$(SHARED).$(MULTIMEDIA_SOVERSION)
MULTIMEDIA_STATIC = $(OBJ_LIBNAME).$(STATIC)

SHARED_CMD = $(CC) -shared -o

.PHONY: all shared static tests clean install

all: create_dir shared static tests
	$(warning "abcd")
create_dir: 
	mkdir -p $(COMPILER_DIR)
shared: $(MULTIMEDIA_SHARED)

static: $(MULTIMEDIA_STATIC)

tests: $(OBJ_TEST)

test: tests
	./$(OBJ_TEST)

.c.o:
	$(warning "abcd" $^)
	$(warning "abcd" $@)
	$(warning "abcd" $<)
	$(CC) -c $(R_CFLAGS) $< -o $(<:%c=%o) -I./$(INC_DIR)

#tests
#multimedia
$(OBJ_TEST): $(OBJ_TEST_SRC) $(INC)
	$(CC) $(R_CFLAGS) $(OBJC) $(OBJ_TEST_SRC)  -o $(COMPILER_DIR)/$@ $(LDLIBS) -I./$(INC_DIR)

#static libraries
#multimedia
$(MULTIMEDIA_STATIC): $(OBJ)
	$(AR) rcs $(COMPILER_DIR)/$@ $<

#shared libraries .so.1.0.0
#multimedia
$(MULTIMEDIA_SHARED_VERSION): $(OBJ)
	$(warning "abcd")
	echo "abcd" $<
	$(CC) -shared -o $(COMPILER_DIR)/$@ $^ $(MULTIMEDIA_SO_LDFLAG) $(LDFLAGS)

#objects
#multimedia
$(OBJC): $(INC)
$(OBJ): $(OBJC)

#links .so -> .so.1 -> .so.1.0.0
#multimedia
$(MULTIMEDIA_SHARED_SO): $(MULTIMEDIA_SHARED_VERSION)
	$(warning "abcd")
	cd $(COMPILER_DIR) && pwd && ln -s $(MULTIMEDIA_SHARED_VERSION) $(MULTIMEDIA_SHARED_SO)
$(MULTIMEDIA_SHARED): $(MULTIMEDIA_SHARED_SO)
	cd $(COMPILER_DIR) && ln -s $(MULTIMEDIA_SHARED_SO) $(MULTIMEDIA_SHARED)	

#install
#multimedia
install-multimedia:
	mkdir -p $(INSTALL_LIBRARY_PATH) $(INSTALL_INCLUDE_PATH)
	$(INSTALL) $(INC) $(INSTALL_INCLUDE_PATH)
	$(INSTALL) $(COMPILER_DIR)/$(MULTIMEDIA_SHARED) $(COMPILER_DIR)/$(MULTIMEDIA_SHARED_SO) $(COMPILER_DIR)/$(MULTIMEDIA_SHARED_VERSION) $(INSTALL_LIBRARY_PATH)
install: install-multimedia

#uninstall
#multimedia
uninstall-multimedia: 
	$(RM) $(INSTALL_LIBRARY_PATH)/$(MULTIMEDIA_SHARED)
	$(RM) $(INSTALL_LIBRARY_PATH)/$(MULTIMEDIA_SHARED_VERSION)
	$(RM) $(INSTALL_LIBRARY_PATH)/$(MULTIMEDIA_SHARED_SO)
	rmdir $(INSTALL_LIBRARY_PATH)
	$(RM) $(INSTALL_INCLUDE_PATH)/demo.h
	rmdir $(INSTALL_INCLUDE_PATH)

uninstall: uninstall-multimedia

clean:
	$(RM) $(OBJ) #delete object files
	$(RM) $(COMPILER_DIR)/$(MULTIMEDIA_SHARED) $(COMPILER_DIR)/$(MULTIMEDIA_SHARED_VERSION) $(COMPILER_DIR)/$(MULTIMEDIA_SHARED_SO) $(COMPILER_DIR)/$(MULTIMEDIA_STATIC) #delete multimedia
	$(RM) $(COMPILER_DIR)/$(OBJ_TEST)  #delete test
