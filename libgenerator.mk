ifndef LIBGENERATOR_MK
LIBGENERATOR_MK := 1
graphtools-dir ?= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
generator-dir  ?= $(graphtools-dir)/graph500/generator

libgenerator.so-sources := $(filter-out %/mrg_transitions.c,$(wildcard $(generator-dir)/*.c))
libgenerator.so-headers := $(wildcard $(generator-dir)/*.h)

# ldflags for executables and libraries that use libgenerator
libgenerator-interface-ldflags += -L$(graphtools-dir)
ifeq ($(shell uname),Darwin)
libgenerator-interface-ldflags += -Wl,-rpath,$(graphtools-dir)
else
libgenerator-interface-ldflags += -Wl,-rpath=$(graphtools-dir)
libgenerator-interface-ldflags += -lgomp
endif
libgenerator-interface-ldflags += -lgenerator

# cxxflags for executables and libraries that use libgenerator
libgenerator-interface-cxxflags += -I$(generator-dir)

# cxxflags for compiling libgenerator.so
libgenerator-cxxflags += -fPIC
libgenerator-cxxflags += -Drestrict=__restrict__
libgenerator-cxxflags += -shared
libgenerator-cxxflags += -O3
ifeq ($(shell uname),Darwin)
# MacOS
else
# Linux
libgenerator-cxxflags += -fopenmp
endif

# libgraphstools header - for dependencies
libgenerator-interface-headers := $(libgenerator.so-headers)

# libgenerator libraries - for dependencies
libgenerator-interface-libraries := $(graphtools-dir)/libgenerator.so

$(graphtools-dir)/libgenerator.so: $(libgenerator.so-headers)
$(graphtools-dir)/libgenerator.so: $(libgenerator.so-sources)
	$(CC) $(libgenerator-cxxflags) $(libgenerator-interface-cxxflags) -o $@ $(filter %.c, $^)

endif
