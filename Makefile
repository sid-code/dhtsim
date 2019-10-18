SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.hpp)

OBJECTS = $(SOURCES:%.cpp=%.o)
PROGRAM = $(shell basename `pwd`)

CC := $(shell which gcc || which clang)
CFLAGS = -Wall -Wextra -pedantic -W -O -fno-exceptions -fno-rtti --std=c++17
LIBS = stdc++ m ssl crypto
LDFLAGS = $(LIBS:%=-l%)

$(PROGRAM) : $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o : %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY : clean
clean :
	rm -f $(PROGRAM) $(OBJECTS)
