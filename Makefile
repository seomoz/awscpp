CPP = g++
CPPOPTS = -O3 -Wall -Werror -Werror=effc++ -g

INCLUDES = -Iinclude -isystem third/Catch/single_include
LIBS = -lcurl

PREFIX ?= /usr/local/include

all: test

driver: driver.cpp *.hpp
	$(CPP) $(CPPOPTS) $(INCLUDES) $(LIBS) -o driver driver.cpp 

test: test.cpp *.hpp
	$(CPP) $(CPPOPTS) $(INCLUDES) $(LIBS) -o test test.cpp
	./test

clean:
	rm -rdf test driver

install: *.hpp
	mkdir -p $(PREFIX)/awscpp
	cp *.hpp $(PREFIX)/awscpp/
