#include<string.h>
#include<getopt.h>
#include<thread>
#include<iostream>
#include<syslog.h>

#include<PacketHandler.h>
#include<FlowTable.h>

using namespace distdpi;

int main(int argc, char* argv[]) {
   int opt;
   int option_index = 0;
   PacketHandlerOptions options;

   while (1) {
      opt = getopt_long (argc, argv, "a:d:",
                         NULL, &option_index);
      if (opt == -1)
         break;

      switch (opt) {
      case 'a':
         options.AgentName.append(optarg);
         break;
      case 'd':
         if (strncmp(optarg,"ACTION_COPY",sizeof("ACTION_COPY")) == 0) {
             options.action = 1;
         }
         break;
      default:
         return 1;
      }
   } 

   PacketHandler pkthdl(std::move(options));

  // Start HTTPServer mainloop in a separate thread
   std::thread t(&PacketHandler::start, &pkthdl);

   t.join();
   return 0;
}
