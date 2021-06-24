.PHONY:clean all
CC=gcc

INCLUDE_DIR=

C_FLAGS=

SUBDIRS= ./vcam ./x-compressor ./test

ROOT_DIR=$(shell pwd)

all:$(SUBDIRS)

$(SUBDIRS):ECHO
	make -C $@

ECHO:
	@echo $(SUBDIRS)

clean:
	for dir in $(SUBDIRS);\
	do $(MAKE) -C $$dir clean||exit 1;\
	done
	
