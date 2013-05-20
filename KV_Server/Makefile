BOOST_DIR := /usr/local/include
INCLUDE := ../include
LIB := ../lib
OBJS := kv_server.o
TARGET := $(LIB)/kv_server


INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -I$(INCLUDE)

.PHONY: all clean

all: $(TARGET)

%.o: %.c
	$(CXX) -g -Wall -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

%.o: %.cpp
	$(CXX) -g -Wall -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) -L$(LIB) $^ -o $@ -lkv_base 

clean:
	$(RM) *.o $(TARGET)

