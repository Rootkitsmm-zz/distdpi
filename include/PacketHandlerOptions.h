#ifndef PACKETHANDLEROPTIONS_H
#define PACKETHANDLEROPTIONS_H

//#include "dvfilterlib.h"
#include <string>

namespace distdpi {

class PacketHandlerOptions {
    public:

       std::string AgentName;
       unsigned int action;
       unsigned int capabilities;
       unsigned int acceptedCapabilities;

};

}

#endif
