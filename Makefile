CPP = g++
CPPOPTS = -O3 -Wall -Werror -Werror=effc++ -g

INCLUDES = -I..
LIBS = `pkg-config --libs --cflags libcurl libssl`

PREFIX ?= /usr/local/include

all: test

driver: driver.cpp *.hpp
	$(CPP) $(CPPOPTS) $(INCLUDES) -o driver driver.cpp $(LIBS)

test: test.cpp *.hpp
	$(CPP) $(CPPOPTS) $(INCLUDES) -isystem third/Catch/single_include -o test test.cpp $(LIBS)
	./test

clean:
	rm -rf test driver

install: *.hpp
	mkdir -p $(PREFIX)/include/awscpp
	cp *.hpp $(PREFIX)/include/awscpp/
