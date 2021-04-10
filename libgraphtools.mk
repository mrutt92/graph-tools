ifndef LIBGRAPHTOOLS_MK
LIBGRAPHTOOLS_MK:=1

graphtools-dir ?= $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
generator-dir  ?= $(graphtools-dir)/graph500/generator

include $(graphtools-dir)/libgenerator.mk

libgraphtools.so-sources += $(graphtools-dir)/Graph500Data.cpp
libgraphtools.so-sources += $(graphtools-dir)/Graph.cpp
#libgraphtools.so-sources += $(graphtools-dir)/WGraph.cpp
libgraphtools.so-headers := $(wildcard $(graphtools-dir)/*.hpp)
libgraphtools.so-objects := $(notdir $(libgraphtools.so-sources:.cpp=.o))

# ldflags for executables and libraries that use libgraphtools
libgraphtools-interface-ldflags += $(libgenerator-interface-ldflags)
libgraphtools-interface-ldflags += -L$(graphtools-dir)
libgraphtools-interface-ldflags += -lgraphtools #-lboost_serialization
ifeq ($(shell uname),Darwin)
libgraphtools-interface-ldflags += -Wl,-rpath,$(graphtools-dir)
else
libgraphtools-interface-ldflags += -Wl,-rpath=$(graphtools-dir)
endif

# ldflags for linking libgraphstools.so
libgraphtools-ldflags += $(libgenerator-interface-ldflags)
libgraphtools-ldflags += -shared
#libgraphtools-ldflags += -lboost_serialization
#libgraphtools-ldflags += -lboost_exception

# cxxflags for executables and libraries that use libgraphtools
libgraphtools-interface-cxxflags += $(libgenerator-interface-cxxflags)
libgraphtools-interface-cxxflags += -I$(graphtools-dir)
libgraphtools-interface-cxxflags += -std=c++11

# cxxflags for compiling libgraphtools.so
libgraphtools-cxxflags += $(libgenerator-interface-cxxflags)
libgraphtools-cxxflags += -I$(graphtools-dir)
libgraphtools-cxxflags += -std=c++11

# libgraphstools header - for dependencies
libgraphtools-interface-headers += $(libgenerator-interface-headers)
libgraphtools-interface-headers += $(libgraphtools.so-headers)

# libgraphtools libraries - for dependencies
libgraphtools-interface-libraries += $(libgenerator-interface-libraries)
libgraphtools-interface-libraries += $(graphtools-dir)/libgraphtools.so

$(libgraphtools.so-objects): $(libgenerator.so-headers)
$(libgraphtools.so-objects): $(libgraphtools.so-headers)
$(libgraphtools.so-objects): %.o: $(graphtools-dir)/%.cpp
	$(CXX) -fPIC $(libgraphtools-cxxflags) -c $<

$(graphtools-dir)/libgraphtools.so: $(libgraphtools.so-objects)
	$(CXX) $(libgraphtools-ldflags) -o $@ $^

.PHONY: libgraphtools.so-clean
libgraphtools.so-clean:
	rm -f $(libgraphtools.so-objects)
	rm -f $(graphtools-dir)/libgraphtools.so

clean: libgraphtools.so-clean

endif
