.PHONY:    all clean
.SUFFIXES:

all: libgenerator.so libgraphtools.so

generator-dir := $(CURDIR)/graph500/generator
graphtools-dir := $(CURDIR)

libgenerator.so-sources := $(filter-out %/mrg_transitions.c,$(wildcard $(generator-dir)/*.c))
libgenerator.so-headers := $(wildcard $(generator-dir)/*.h)

libgenerator.so: CFLAGS += -I$(generator-dir)
libgenerator.so: CFLAGS += -Drestrict=__restrict__
libgenerator.so: CFLAGS += -fPIC
libgenerator.so: CFLAGS += -shared
libgenerator.so: $(libgenerator.so-headers)
libgenerator.so: $(libgenerator.so-sources)
	$(CC) $(CFLAGS) -o $@ $(filter %.c, $^)

libgraphtools.so-sources := $(wildcard $(graphtools-dir)/*.cpp)
libgraphtools.so-headers := $(wildcard $(graphtools-dir)/*.hpp)
libgraphtools.so-objects := $(libgraphtools.so-sources:.cpp=.o)

$(libgraphtools.so-objects): CXXFLAGS += -std=c++11
$(libgraphtools.so-objects): CXXFLAGS += -I$(graphtools-dir)
$(libgraphtools.so-objects): CXXFLAGS += -I$(generator-dir)
$(libgraphtools.so-objects): CXXFLAGS += -fPIC
$(libgraphtools.so-objects): $(libgenerator.so-headers)
$(libgraphtools.so-objects): $(libgraphtools.so-headers)
$(libgraphtools.so-objects): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

libgraphtools.so: LDFLAGS += -shared
libgraphtools.so: $(libgraphtools.so-objects)
	$(CXX) $(LDFLAGS) -o $@ $^

graphtools-tests += Graph500Data
graphtools-tests += Graph

$(graphtools-tests): LDFLAGS += -L$(graphtools-dir)
$(graphtools-tests): LDFLAGS += -lgraphtools -lgenerator
$(graphtools-tests): LDFLAGS += -Wl,-rpath=$(graphtools-dir)
$(graphtools-tests): %: %.o
	$(CXX) $(LDFLAGS) -o $@ $<

pr-%:
	@echo $($(subst pr-,,$@))
clean:
	rm -f libgenerator.so
	rm -f libgraphtools.so
	rm -f *.o
	rm -f *~
	rm -f $(graphtools-tests)
