BOOST_DIR := /usr/local/include
INCLUDE := ../include
OBJS := http_get.o http_post.o
TARGET := webserver

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -I$(INCLUDE)

.PHONY: all clean

all: $(TARGET)

%.o: %.c
	$(CXX) -g -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

%.o: %.cpp
	$(CXX) -g -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ 

clean:
	$(RM) *.o $(TARGET)

