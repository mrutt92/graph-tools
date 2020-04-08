ifndef LIBGENERATOR_MK
LIBGENERATOR_MK := 1
graphtools-dir ?= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
generator-dir  ?= $(graphtools-dir)/graph500/generator

libgenerator.so-sources := $(filter-out %/mrg_transitions.c,$(wildcard $(generator-dir)/*.c))
libgenerator.so-headers := $(wildcard $(generator-dir)/*.h)

libgenerator-interface-ldflags += -L$(graphtools-dir)
libgenerator-interface-ldflags += -Wl,-rpath=$(graphtools-dir)
libgenerator-interface-ldflags += -lgenerator

libgenerator-interface-cxxflags += -I$(generator-dir)
libgenerator-cxxflags += -fPIC
libgenerator-cxxflags += -Drestrict=__restrict__
libgenerator-cxxflags += -shared
libgenerator-cxxflags += -O3

ifeq ($(shell uname),Darwin)
# MacOS
else
# Linux
libgenerator-cxxflags += -fopenmp
libgenerator-interface-ldflags += -lgomp
endif

libgenerator-interface-headers := $(libgenerator.so-headers)
libgenerator-interface-libraries := $(graphtools-dir)/libgenerator.so

$(graphtools-dir)/libgenerator.so: $(libgenerator.so-headers)
$(graphtools-dir)/libgenerator.so: $(libgenerator.so-sources)
	$(CC) $(libgenerator-cxxflags) $(libgenerator-interface-cxxflags) -o $@ $(filter %.c, $^)

endif
