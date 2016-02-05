#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <memory>
#include <signal.h>

namespace distdpi {

class SignalHandler {
public:

    SignalHandler() {};
    
    void install(SignalHandler *hdl, std::vector<int>& signals) {
        struct sigaction sigAct;

        sh = hdl;        
        for (unsigned int i = 0; i < signals.size(); i++) {
            memset(&sigAct, 0, sizeof(sigAct));
            sigAct.sa_handler = (void *)&SignalHandler::signalreceived;
            sigaction(signals[i], &sigAct, 0);
        }
    }
    void signalreceived(int signal) {
        sh->stop();
    }
   
protected:
    virtual void stop() = 0;

private:
    SignalHandler *sh;
};

}
#endif
