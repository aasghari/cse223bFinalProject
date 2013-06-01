

ifndef thrift_home
thrift_home=../../
endif #thrift_home

target: all

ifndef boost_home
boost_home=/usr/local/include
endif #boost_home
target: all

include_paths = $(thrift_home)/lib/cpp/src \
		$(boost_home)

include_flags = $(patsubst %,-I%, $(include_paths))

# Tools

THRIFT = thrift




CXX = g++-4.4

# Compiler flags
LFL   =  -L$(thrift_home)/lib/cpp/.libs -lthrift
CCFL  = -Wall -O3 -g -I./gen-cpp $(include_flags)
CFL   = $(CCFL) $(LFL)

all: server client

stubs: ThreadsTest.thrift
	$(THRIFT) --gen cpp  ThreadsTest.thrift

server: stubs
	 $(CXX)  -Wall -O3   -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -o ThreadsServer $(CFL) ThreadsServer.cpp ./gen-cpp/ThreadsTest.cpp ./gen-cpp/ThreadsTest_types.cpp

client: stubs
	$(CXX)  -Wall -O3   -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -o ThreadsClient $(CFL) ThreadsClient.cpp ./gen-cpp/ThreadsTest.cpp ./gen-cpp/ThreadsTest_types.cpp

clean:
	$(RM) -r *.o ThreadsServer ThreadsClient gen-cpp gen-py
