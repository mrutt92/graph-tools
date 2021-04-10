.PHONY:    all clean test
.SUFFIXES:

graphtools-dir := .
generator-dir  := $(graphtools-dir)/graph500/generator

all:

include $(graphtools-dir)/libgenerator.mk
include $(graphtools-dir)/libgraphtools.mk

all: $(graphtools-dir)/libgenerator.so
all: $(graphtools-dir)/libgraphtools.so

# Lists of basic graphtools tests and their sources
# Add more tests here (in namespace graph_tools)
graphtools-test-modules += Graph500Data
graphtools-test-modules += Graph
graphtools-test-modules += WGraph
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

$(all-tests): LDFLAGS += $(libgraphtools-interface-ldflags)
$(all-tests): CXXFLAGS += $(libgraphtools-interface-cxxflags)
$(all-tests): $(libgraphtools-interface-libraries)
$(all-tests): $(libgraphtools-interface-headers)
$(all-tests): %: %.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<
	./$@ $($@-argv)

test: $(all-tests)

pr-%:
	@echo $($(subst pr-,,$@))

clean:
	rm -f $(graphtools-dir)/libgenerator.so
	rm -f $(graphtools-dir)/libgraphtools.so
	rm -f $(graphtools-dir)/*.o
	rm -f $(graphtools-dir)*~
	rm -f $(all-tests)
	rm -f $(filter-out $(all-tests-no-clean-tests-sources), $(all-tests-sources))

