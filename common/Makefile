BOOST_DIR := /usr/local/include
INCLUDE := ../include
LIB := ../lib
OBJS := kv_store.o
TARGET := $(LIB)/libkv_base.so


INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -I$(INCLUDE)

.PHONY: all clean

all: $(TARGET)

%.o: %.c
	$(CXX) -g -Wall -fPIC -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

%.o: %.cpp
	$(CXX) -g -Wall -fPIC -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

$(TARGET): $(OBJS)
	mkdir -p $(LIB)
	$(CXX) $^ -shared -o $(LIB)/$@ 

clean:
	$(RM) *.o $(TARGET)

