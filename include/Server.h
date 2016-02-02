#ifndef SERVER_H
#define SERVER_H

namespace distdpi {

class Server {
public:

    Server() {
        buflen_ = 1024;
        buf_ = new char[buflen_ + 1];
    }
    ~Server() {
        delete buf_;
    }

    void run() {
        create();
        serve();
    }

protected:
    virtual void create() = 0;
    virtual void closeSocket() = 0;
    virtual void pktcb(uint8_t *pkt, uint32_t len) = 0;

    void handle(int client) {
        while (1) {
            string request = get_request(client);
            if (request.empty())
                break;
            pktcb((uint8_t *) request.c_str(), request.size());
            //bool success = send_response(client,request);
            //if (not success)
            //    break;
        }
        close(client);
    }

    void serve() {
        int client;
        struct sockaddr_in client_addr;
        socklen_t clientlen = sizeof(client_addr);

        while ((client = accept(server_,(struct sockaddr *)&client_addr,&clientlen)) > 0) {

            handle(client);
        }
        closeSocket();
    }

    string get_request(int client) {
        string request = "";
        // read until we get a newline
        while (request.find("xBADA55") == string::npos) {
            int nread = recv(client,buf_,1024,0);
            if (nread < 0) {
                if (errno == EINTR)
                    // the socket call was interrupted -- try again
                    continue;
                else
                    // an error occurred, so break out
                    return "";
            } else if (nread == 0) {
                // the socket is closed
                return "";
            }
            // be sure to use append in case we have binary data
            request.append(buf_,nread);
            //std::cout << " ----- " << buf_ << std::endl;
        }
        // a better server would cut off anything after the newline and
        // save it in a cache
        return request;
    }

    bool send_response(int client, string response) {
        // prepare to send response
        const char* ptr = response.c_str();
        int nleft = response.length();
        int nwritten;
        // loop to be sure it is all sent
        while (nleft) {
            if ((nwritten = send(client, ptr, nleft, 0)) < 0) {
                if (errno == EINTR) {
                    // the socket call was interrupted -- try again
                    continue;
                } else {
                    // an error occurred, so break out
                    perror("write");
                    return false;
                }
            } else if (nwritten == 0) {
                // the socket is closed
                return false;
            }
            nleft -= nwritten;
            ptr += nwritten;
        }
        return true;
    }

    int server_;
    int buflen_;
    char* buf_;    
};

}

#endif 
