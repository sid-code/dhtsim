SOURCES = $(wildcard *.cpp) $(wildcard kademlia/*.cpp)
HEADERS = $(wildcard *.hpp) $(wildcard kademlia/*.hpp)

OBJECTS = $(SOURCES:%.cpp=%.o)
PROGRAM = $(shell basename `pwd`)

CC := $(shell which gcc || which clang)
CFLAGS = -g -O0 -Wall -Wextra -pedantic -W -fno-exceptions -fno-rtti --std=c++17
INCLUDES = -I. -I./libnop/include/
LIBS = stdc++ m ssl crypto
LDFLAGS = $(LIBS:%=-l%)

$(PROGRAM) : $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o : %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $< $(INCLUDES)

.PHONY : clean
clean :
	rm -f $(PROGRAM) $(OBJECTS)
