/**
 * Template header file fo NDPlugins
 * 
 * 
 * Author:Jakub Wlodek 
 * Created on: 02/06/2023
 * 
 */

#ifndef NDPluginTomo_H
#define NDPluginTomo_H

//Define necessary includes here

#ifdef _WIN32
  /* See http://stackoverflow.com/questions/12765743/getaddrinfo-on-win32 */
  #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0501  /* Windows XP. */
  #endif
  #include <winsock2.h>
  #include <Ws2tcpip.h>
#else
  /* Assume that any non-Windows platform uses POSIX-style sockets instead. */
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
  #include <unistd.h> /* Needed for close() */
#endif

#include "NSLS2TomoStreamProtocol.hpp"


using namespace std;

//include base plugin driver
#include "NDPluginDriver.h"

//version numbers
#define TOMO_VERSION        0
#define TOMO_REVISION       0
#define TOMO_MODIFICATION   0

#define PORT 8090
#define SA struct sockaddr


// Define the PVStrings for all of your PV values here in the following format
//#define NDPluginTomoPVNameString 	"TOMO_PVNAME" 		//DTYP (ex. asynInt32, asynFloat64, asynOctet)
#define NDPluginTomoFrameIDString           "NDTOMO_FRAME_ID"     //asynInt32
#define NDPluginTomoFrameTypeString         "NDTOMO_FRAME_TYPE"   //asynInt32
#define NDPluginTomoConnectString           "NDTOMO_CONN"         //asynInt32
#define NDPluginTomoConnectionStatusString  "NDTOMO_CONN_STATUS"  //asynInt32
#define NDPluginTomoAngleIncrementString    "NDTOMO_ANGLE_INC"    //asynFloat64
#define NDPluginTomoLastAngleString         "NDTOMO_LAST_ANGLE"   //asynFloat64


typedef enum TomoConnStatus {
  TOMO_STREAM_DISCONNECTED = 0,
  TOMO_STREAM_AWAITING_CONNECTION = 1,
  TOMO_STREAM_CONNECTED = 2,
} TomoConnStatus_t;


/* Plugin class, extends plugin driver */
class NDPluginTomo : public NDPluginDriver {
    public:
        NDPluginTomo(const char *portName, int queueSize, int blockingCallbacks,
            const char* NDArrayPort, int NDArrayAddr, int maxBuffers,
            size_t maxMemory, int priority, int stackSize, int maxThreads);

        ~NDPluginTomo();

        void connectToClient(void* pPvt);
        void processCallbacks(NDArray *pArray);

        virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);

    protected:

        // Define the Param index variables here. Ex:
        // int NDPluginTomoPVName;
        int NDTomo_FrameID;
        int NDTomo_FrameType;
        int NDTomo_Connect;
        int NDTomo_ConnectionStatus;
        int NDTomo_AngleIncrement;
        int NDTomo_LastAngle;
        // Define these two variables as the first and last param indexes.
        #define ND_TOMO_FIRST_PARAM NDTomo_FrameID
        #define ND_TOMO_LAST_PARAM NDTomo_LastAngle


    private:


        int sockfd, connfd, len;
        struct sockaddr_in* serveraddr;
        struct sockaddr* clientaddr;
        // init all global variables here

        // init all plugin additional functions here

};

// Def that computes the number of params specific to the plugin
#define NUM_TOMO_PARAMS ((int)(&ND_TOMO_LAST_PARAM - &ND_TOMO_FIRST_PARAM + 1))

#endif
