.PHONY:    all clean test pr-object pr-source pr-header
.SUFFIXES:
.SECONDEXPANSION:


cxxflags += -I./
cxxflags += -std=c++11
ccflags += -I./

##############################
# Graph500 Generator Sources #
##############################
generator-dir := graph500/generator

objects += make_graph.o
sources-make_graph.o += $(generator-dir)/make_graph.c

headers-make_graph.o += $(generator-dir)/make_graph.h
headers-make_graph.o += $(generator-dir)/graph_generator.h
headers-make_graph.o += $(generator-dir)/user_settings.h
ccflags-make_graph.o += -Drestrict=__restrict__

objects += graph_generator.o
sources-graph_generator.o += $(generator-dir)/graph_generator.c
headers-graph_generator.o += $(generator-dir)/graph_generator.h
headers-graph_generator.o += $(generator-dir)/user_settings.h
ccflags-graph_generator.o += -Drestrict=__restrict__

objects += splittable_mrg.o
sources-splittable_mrg.o += $(generator-dir)/splittable_mrg.c
headers-splittable_mrg.o += $(generator-dir)/splittable_mrg.h
headers-splittable_mrg.o += $(generator-dir)/user_settings.h
ccflags-splittable_mrg.o += -Drestrict=__restrict__

objects += utils.o
sources-utils.o += $(generator-dir)/utils.c
headers-utils.o += $(generator-dir)/utils.h
headers-utils.o += $(generator-dir)/user_settings.h
ccflags-utils.o += -Drestrict=__restrict__

objects-graph500-generator += make_graph.o
objects-graph500-generator += graph_generator.o
objects-graph500-generator += splittable_mrg.o
objects-graph500-generator += utils.o

#######################
# Graph Tools Objects #
#######################
objects += Graph500Data.o
sources-Graph500Data.o += Graph500Data.cpp
headers-Graph500Data.o += Graph500Data.hpp
cxxflags-Graph500Data.o += -Igraph500/generator

objects += Graph.o
sources-Graph.o += Graph.cpp
headers-Graph.o += Graph500Data.hpp
headers-Graph.o += Graph.hpp
cxxflags-Graph.o += -Igraph500/generator

###########
# Sources #
###########
sources := $(foreach obj,$(objects),$(sources-$(obj)))
c-sources   := $(filter %.c, $(sources))
cxx-sources := $(filter %.cpp, $(sources))

###########
# Headers #
###########
headers += $(foreach obj,$(objects),$(headers-$(obj)))

################
# Object files #
################
cxx-objects := $(foreach obj,$(cxx-sources:.cpp=.o),$(notdir $(obj)))
c-objects   := $(foreach obj,$(c-sources:.c=.o),$(notdir $(obj)))

$(cxx-objects): %.o: $$(headers-$$@)
$(cxx-objects): %.o: $$(sources-$$@)
	$(CXX) $(cxxflags) $(cxxflags-$@) -c $<



$(c-objects): %.o: $$(headers-$$@)
$(c-objects): %.o: $$(sources-$$@)
	$(CC) $(ccflags) $(ccflags-$@) -c $<

.PHONY: clean-objects
clean: clean-objects
clean-objects:
	rm -f $(objects)

###############
# Executables #
###############
executables += Graph500Data
objects-Graph500Data += Graph500Data.o $(objects-graph500-generator)

executables += Graph
objects-Graph += Graph.o $(objects-graph500-generator)

$(executables): $$(objects-$$@)
	$(CXX) -o $@ $(LDFLAGS) $(objects-$@)

.PHONY: clean-executables
clean: clean-executables
clean-executables:
	rm -f $(executables)

#######################################
# Print targets (for other makefiles) #
#######################################
print_targets := pr-objects pr-sources pr-headers pr-c-sources pr-cxx-sources pr-c-objects
.PONHY: $(print_targets)
pr-%:
	@echo $($(subst pr-,,$@))

.DEFAULT_GOAL := all
all: $(objects) $(executables)
