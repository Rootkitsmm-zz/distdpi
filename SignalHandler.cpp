#include <memory>
#include <csignal>

#include <SignalHandler.h>

namespace distdpi {

void *sh = NULL;

void SignalHandler::signalreceived(int signal) {
    ((SignalHandler *)sh)->stop();
}

void SignalHandler::install(SignalHandler *hdl, std::vector<int>& signals) {

    sh = hdl;
    for (unsigned int i = 0; i < signals.size(); i++) {
        std::signal(signals[i], SignalHandler::signalreceived);
    }
}

}
