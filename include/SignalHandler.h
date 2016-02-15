#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <vector>

namespace distdpi {

class SignalHandler {
public:

    /**
     * Callback function registered for signall
     * handling
     */
    static void signalreceived(int signal);

    /**
     * Create new SignalHandler
     */
    SignalHandler() {};
    
    /**
     * Get a list of signals to be installed
     */
    void install(SignalHandler *hdl, std::vector<int>& signals);

protected:
    /**
     * Call the stop function of the class
     * inheriting the signal handler
     */
    virtual void stop() = 0;
};

}
#endif
