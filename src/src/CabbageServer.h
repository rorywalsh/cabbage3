#pragma once

#include "httplib.h"


class CabbageServer
{
public:
    CabbageServer();
    ~CabbageServer(){
        stop();
    }
    void changeMountPoint(std::string mp);

    void start(std::string mountPoint);

    bool isRunning() { return isThreadRunning(); }
    httplib::Server& getCabbageServer() 
    { 
        return mServer; 
    }

    int getCurrentPort() { return mPortNumber; }

    bool isThreadRunning() {
        return serverThread.joinable();
    }

protected:
    void run();
    void stop();

private:
    httplib::Server          mServer;
    int                      mPortNumber;
    bool isListening = true;
    std::string mountPoint = "";
    std::thread serverThread;

    
};
