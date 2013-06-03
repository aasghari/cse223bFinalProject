// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "ThreadsTest.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace  ;

class ThreadsTestHandler : virtual public ThreadsTestIf {
 public:
  ThreadsTestHandler() {
    // Your initialization goes here
  }

  int32_t threadOne(const int32_t sleep) {
    // Your implementation goes here
    printf("threadOne\n");
  }

  int32_t threadTwo(const int32_t sleep) {
    // Your implementation goes here
    printf("threadTwo\n");
  }

  int32_t threadThree(const int32_t sleep) {
    // Your implementation goes here
    printf("threadThree\n");
  }

  int32_t threadFour(const int32_t sleep) {
    // Your implementation goes here
    printf("threadFour\n");
  }

  int32_t stop() {
    // Your implementation goes here
    printf("stop\n");
  }

};

int main(int argc, char **argv) {
  int port = 9090;
  shared_ptr<ThreadsTestHandler> handler(new ThreadsTestHandler());
  shared_ptr<TProcessor> processor(new ThreadsTestProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}
