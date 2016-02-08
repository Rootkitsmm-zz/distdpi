#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <vector>

namespace distdpi {

class SignalHandler {
public:

    static void signalreceived(int signal);

    SignalHandler() {};
    
    void install(SignalHandler *hdl, std::vector<int>& signals);

protected:
    virtual void stop() = 0;
};

}
#endif
