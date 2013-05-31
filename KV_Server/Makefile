BOOST_DIR := /usr/local/include
INCLUDE := ../include
LIB := ../lib
OBJS := kv_server.o MessageHandler.o
TARGET := $(LIB)/kv_server


INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -I$(INCLUDE)

.PHONY: all clean

all: $(TARGET)

%.o: %.c
	$(CXX) -g -Wall -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

%.o: %.cpp
	$(CXX) -g -Wall -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) -L$(LIB) $^ '-Wl,--rpath=$$ORIGIN' -o $@ -pthread -lkv_base -lboost_system-mt -lboost_thread-mt 

clean:
	$(RM) *.o $(TARGET)

