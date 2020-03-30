.PHONY:    all clean test
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

libgraphtools.so-sources += Graph500Data.cpp
libgraphtools.so-sources += Graph.cpp
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

# Lists of basic graphtools tests and their sources
# Add more tests here (in namespace graph_tools)
graphtools-test-modules += Graph500Data
graphtools-test-modules += Graph
graphtools-tests := $(addsuffix -test,$(graphtools-test-modules))
graphtools-tests-sources := $(addsuffix .cpp, $(graphtools-tests))

# Lists of all memory modeling tests and their sources
# Add more tests here (in namespace memory_modeling)
memory-modeling-test-modules += VectorWithCache
# dont touch
memory-modeling-tests := $(addsuffix -test,$(memory-modeling-test-modules))
memory-modeling-tests-sources := $(addsuffix .cpp, $(memory-modeling-tests))

# Lists of all tests and their sources (dont touch these)
all-tests-sources := $(graphtools-tests-sources)
all-tests-sources += $(memory-modeling-tests-sources)
all-tests += $(graphtools-tests) $(memory-modeling-tests)

# Tests that we don't want cleaned (add more as needed)
#all-tests-no-clean += VectorWithCache
# Dont touch these
all-tests-no-clean-tests := $(addsuffix -test,$(all-tests-no-clean))
all-tests-no-clean-tests-sources := $(addsuffix .cpp,$(all-tests-no-clean-tests))

$(all-tests-sources):             namespaces += graph_tools
$(memory-modeling-tests-sources): namespaces += memory_modeling
VectorWithCache-test.cpp:         templates  := <int>
$(all-tests-sources):
	@echo "#include <$(@:-test.cpp=.hpp)>" > $@
	@echo "int main(int argc, char *argv[]) {" >> $@
	@$(foreach namespace,$(namespaces),\
	echo    "    using namespace $(namespace);" >> $@;)
	@echo   "    return $(@:-test.cpp=)$(templates)::Test(argc, argv);" >> $@
	@echo "}" >> $@

$(all-tests): LDFLAGS += -L$(graphtools-dir)
$(all-tests): LDFLAGS += -lgenerator -lboost_serialization
$(all-tests): LDFLAGS += -Wl,-rpath=$(graphtools-dir)
$(all-tests): CXXFLAGS += -std=c++11 -I$(graphtools-dir)
$(all-tests): CXXFLAGS += -std=c++11 -I$(generator-dir)
$(all-tests): libgenerator.so
$(all-tests): libgraphtools.so
$(all-tests): %: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
	./$@ $($@-argv)

test: $(all-tests)

pr-%:
	@echo $($(subst pr-,,$@))
clean:
	rm -f libgenerator.so
	rm -f libgraphtools.so
	rm -f *.o
	rm -f *~
	rm -f $(all-tests)
	rm -f $(filter-out $(all-tests-no-clean-tests-sources), $(all-tests-sources))
