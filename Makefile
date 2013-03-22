CPP = g++
CPPOPTS = -O3 -Wall -Werror -Werror=effc++ -g

INCLUDES = -Iinclude
LIBS = -lcurl

all: test

driver: driver.cpp include/aws/*
	$(CPP) $(CPPOPTS) $(INCLUDES) $(LIBS) -o driver driver.cpp 

test: test.cpp include/aws/*
	$(CPP) $(CPPOPTS) $(INCLUDES) $(LIBS) -o test test.cpp
	./test

clean:
	rm -rdf test driver
