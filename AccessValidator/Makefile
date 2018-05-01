.PHONY: all

CXX=g++
CXXFLAGS=--std=c++11
LD=g++
 
%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

OBJECTS=predicate.o graph.o AccessValidator.o solver.o

all: access_validator

access_validator: $(OBJECTS)
	$(LD) -o $@ $(OBJECTS)


