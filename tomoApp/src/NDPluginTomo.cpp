/**
 * This file is a basic template for implementing areaDetector plugins.
 * You must implement all of the functions already listed here along with any 
 * additional plugin specific functions you require.
 * 
 * Author: Jakub Wlodek
 * Created on: 02/06/2023
 * 
 */



//include some standard libraries
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <stdio.h>


//include epics/area detector libraries
#include <epicsExit.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <iocsh.h>
#include "NDArray.h"
// Include your plugin's header file here
#include "NDPluginTomo.hpp"
#include <epicsExport.h>


// Error message formatters
#define ERR(msg)                                                                                 \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERR  | %s::%s: %s\n", pluginName, functionName, \
              msg)

#define ERR_ARGS(fmt, ...)                                                              \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "ERR  | %s::%s: " fmt "\n", pluginName, \
              functionName, __VA_ARGS__);

// Warning message formatters
#define WARN(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: %s\n", pluginName, functionName, msg)

#define WARN_ARGS(fmt, ...)                                                            \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "WARN | %s::%s: " fmt "\n", pluginName, \
              functionName, __VA_ARGS__);

// Log message formatters
#define LOG(msg) \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "LOG  | %s::%s: %s\n", pluginName, functionName, msg)

#define LOG_ARGS(fmt, ...)                                                                       \
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, "LOG  | %s::%s: " fmt "\n", pluginName, functionName, \
              __VA_ARGS__);


// Include your external dependency library headers


// Namespaces
using namespace std;


// Name of the plugin
static const char *pluginName="NDPluginTomo";


int sockInit(void)
{
  #ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(1,1), &wsa_data);
  #else
    return 0;
  #endif
}

int sockQuit(void)
{
  #ifdef _WIN32
    return WSACleanup();
  #else
    return 0;
  #endif
}


int sockClose(int sock)
{

  int status = 0;

  #ifdef _WIN32
    status = shutdown(sock, SD_BOTH);
    if (status == 0) { status = closesocket(sock); }
  #else
    status = shutdown(sock, SHUT_RDWR);
    if (status == 0) { status = close(sock); }
  #endif

  return status;

}


static void exitCallback(void* pPvt) {
    NDPluginTomo* pNDPluginTomo = (NDPluginTomo*) pPvt;
    delete pNDPluginTomo;
}

static void connectToClientThread(void* pPvt) {
    NDPluginTomo* pNDPluginTomo = (NDPluginTomo*) pPvt;
    pNDPluginTomo->connectToClient(pNDPluginTomo);
}


void NDPluginTomo::connectToClient(void* pPvt){
    printf("Here2\n");
    const char* functionName = "connectToClient";
    int opt = 1;
    TomoConnStatus_t connectionStatus;
    getIntegerParam(NDTomo_ConnectionStatus, (int*) &connectionStatus);

    if (connectionStatus == TOMO_STREAM_CONNECTED){
        WARN("Client already connected!");
        return;
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR("Socket creation failed!");
        return;
    } else {
        printf("Created socket\n");
        if (setsockopt(sockfd, SOL_SOCKET,
                SO_REUSEADDR | SO_REUSEPORT, &opt,
                sizeof(opt))) {
            ERR("Operation: setsockopt failed!");
            return;
        }
        serveraddr->sin_family = AF_INET;
        serveraddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        //serveraddr.sin_addr.s_addr = INADDR_ANY;
        serveraddr->sin_port = htons(PORT);
        printf("Binding to socket\n");
        if((bind(sockfd, (struct sockaddr*) serveraddr, sizeof(struct sockaddr))) != 0) {
            ERR("Socket binding failed!");
        } else {
            LOG("Socket successfully binded.\n");
        }

        if ((listen(sockfd, 3)) != 0) {
            ERR("Failed to listen on bound socket!");
            return;
        } else {
            LOG("Server listening and awaiting client connection...");
        }

        setIntegerParam(NDTomo_ConnectionStatus, TOMO_STREAM_AWAITING_CONNECTION);
        callParamCallbacks();

        len = sizeof(struct sockaddr_in);
        connfd = accept(sockfd, (struct sockaddr*) clientaddr, (socklen_t*) &len);
        if (connfd < 0) {
            setIntegerParam(NDTomo_ConnectionStatus, TOMO_STREAM_DISCONNECTED);
            ERR("Server failed to accept client!");
            exit(0);
        } else {
            setIntegerParam(NDTomo_ConnectionStatus, TOMO_STREAM_CONNECTED);
            LOG("Connected to client.");
        }
        callParamCallbacks();
    }
}



/**
 * Override of NDPluginDriver function. Must be implemented by your plugin
 *
 * Performs callback when write operation is performed on an asynInt32 record
 * 
 * @params[in]: pasynUser	-> pointer to asyn User that initiated the transaction
 * @params[in]: value		-> value PV was set to
 * @return: success if PV was updated correctly, otherwise error
 */
asynStatus NDPluginTomo::writeInt32(asynUser* pasynUser, epicsInt32 value){
    const char* functionName = "writeInt32";
    int function = pasynUser->reason;
    asynStatus status = asynSuccess;

    status = setIntegerParam(function, value);
    LOG_ARGS("function = %d value=%d", function, value);

    // TODO: Handle callbacks for any integer param write ops
    
    if(function < ND_TOMO_FIRST_PARAM){
        status = NDPluginDriver::writeInt32(pasynUser, value);
    }
    callParamCallbacks();
    if(status){
        ERR_ARGS("Failed to wrote Int32 val to PV: function = %d value=%d", function, value);
    }
    return status;
}



/* Process callbacks function inherited from NDPluginDriver.
 * You must implement this function for your plugin to accept NDArrays
 *
 * @params[in]: pArray -> NDArray recieved by the plugin from the camera
 * @return: void
*/
void NDPluginTomo::processCallbacks(NDArray *pArray){
    static const char* functionName = "processCallbacks";
    NDArrayInfo arrayInfo;
    double lastAngle, newAngle, angleIncrement;

    TomoConnStatus_t connectionStatus;
    getIntegerParam(NDTomo_ConnectionStatus, (int*) &connectionStatus);
    if(connectionStatus == TOMO_STREAM_DISCONNECTED) {
        WARN("No client connected, spawning connection thread...");
        epicsThreadCreate("ClientConnectionThread", epicsThreadPriorityMedium, 
                           epicsThreadStackMedium, connectToClientThread, this);
        return;
    } else if(connectionStatus == TOMO_STREAM_AWAITING_CONNECTION) {
        LOG("Waiting for client connection...");
        return;
    }

    NSLS2TomoStreamProtocolFrameType_t frameType;
    NSLS2TomoStreamProtocolRefType_t refType;
    getIntegerParam(NDTomo_FrameType, (int*) &frameType);
    getIntegerParam(NDTomo_FrameID, (int*) &refType);


    getDoubleParam(NDTomo_LastAngle, &lastAngle);
    getDoubleParam(NDTomo_AngleIncrement, &angleIncrement);

    //call base class and get information about frame
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);

    //unlock the mutex for the processing portion
    this->unlock();

    NSLS2TomoStreamProtocolHeader_t header;
    //header.frame_type = PROJECTION_FRAME;
    //header.reference_type = REF_TIMESTAMP;
    header.frame_type = frameType;
    header.reference_type = refType;
    header.dataType = (uint8_t) pArray->dataType;

    newAngle = lastAngle + angleIncrement;
    if(refType == REF_TIMESTAMP) {
        header.reference = pArray->timeStamp;
    } else {
        header.reference = newAngle;
    }

    header.num_bytes = arrayInfo.totalBytes;
    header.x_size = arrayInfo.xSize;
    header.y_size = arrayInfo.ySize;
    header.color_channels = arrayInfo.colorSize;

    LOG("Sending header to client...");
    size_t ret = send(connfd, &header, sizeof(NSLS2TomoStreamProtocolHeader_t), MSG_CONFIRM);
    if((int) ret < 0){
        ERR("Failed to transmit header to client - socket disconnected!");
        close(connfd);
        shutdown(sockfd, SHUT_RDWR);
        setIntegerParam(NDTomo_ConnectionStatus, TOMO_STREAM_DISCONNECTED);
        callParamCallbacks();
    } else {
        LOG_ARGS("Sent %d bytes", (int) ret);
        LOG("Sending image data buffer to client...");
        size_t ret_img = send(connfd, pArray->pData, arrayInfo.totalBytes, MSG_CONFIRM);
        if((int) ret_img < 0) {
            ERR("Failed to transmit image data to client - socket disconnected!");
            close(connfd);
            shutdown(sockfd, SHUT_RDWR);
            setIntegerParam(NDTomo_ConnectionStatus, TOMO_STREAM_DISCONNECTED);
            callParamCallbacks();
        } else {
            LOG_ARGS("Sent %d bytes of image data", (int) ret_img);
        }
    }

    // Update angle position
    setDoubleParam(NDTomo_LastAngle, newAngle);

    this->lock();

    NDPluginDriver::endProcessCallbacks(pArray, true, true);

    // If pScratch was allocated in this function, make sure to release it.
    // pScratch.release()

    callParamCallbacks();
}



//constructror from base class, replace with your plugin name
NDPluginTomo::NDPluginTomo(
        const char *portName, int queueSize, int blockingCallbacks,
        const char *NDArrayPort, int NDArrayAddr,
        int maxBuffers, size_t maxMemory,
        int priority, int stackSize, int maxThreads)
        /* Invoke the base class constructor */
        : NDPluginDriver(portName, queueSize, blockingCallbacks,
        NDArrayPort, NDArrayAddr, 1, maxBuffers, maxMemory,
        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
        asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
        ASYN_MULTIDEVICE, 1, priority, stackSize, maxThreads)
{

    const char* functionName = "NDPluginTomo";
    char versionString[25];
    printf("Here\n");

    this->serveraddr = (sockaddr_in*) malloc(sizeof(sockaddr_in));
    this->clientaddr = (sockaddr*) malloc(sizeof(sockaddr_in));

    // Initialize Parameters here, using the string vals and indexes from the header. Ex:
    createParam(NDPluginTomoFrameIDString,  asynParamInt32, &NDTomo_FrameID);
    createParam(NDPluginTomoFrameTypeString,  asynParamInt32, &NDTomo_FrameType);
    createParam(NDPluginTomoConnectString,  asynParamInt32, &NDTomo_Connect);
    createParam(NDPluginTomoConnectionStatusString,  asynParamInt32, &NDTomo_ConnectionStatus);
    createParam(NDPluginTomoAngleIncrementString,  asynParamFloat64, &NDTomo_AngleIncrement);
    createParam(NDPluginTomoLastAngleString,  asynParamFloat64, &NDTomo_LastAngle);


    // Set some basic plugin info Params
    setStringParam(NDPluginDriverPluginType, "NDPluginTomo");
    epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", TOMO_VERSION, TOMO_REVISION, TOMO_MODIFICATION);
    setStringParam(NDDriverVersion, versionString);

    LOG_ARGS("Initializing NDPluginTomo instance with version %s", versionString);

    epicsAtExit(exitCallback, (void*) this);

    connectToArrayPort();
    printf("Here3\n");

    epicsThreadCreate("ClientConnectionThread", epicsThreadPriorityMedium, 
                       epicsThreadStackMedium, connectToClientThread, this);
}


NDPluginTomo::~NDPluginTomo(){
    printf("Closing socket...\n");
    close(connfd);
    free(serveraddr);
    free(clientaddr);
    shutdown(sockfd, SHUT_RDWR);
}



/**
 * External configure function. This will be called from the IOC shell of the
 * detector the plugin is attached to, and will create an instance of the plugin and start it
 * 
 * @params[in]	-> all passed to constructor
 */
extern "C" int NDTomoConfigure(
        const char *portName, int queueSize, int blockingCallbacks,
        const char *NDArrayPort, int NDArrayAddr,
        int maxBuffers, size_t maxMemory,
        int priority, int stackSize, int maxThreads){

    // Initialize instance of our plugin and start it.
    NDPluginTomo *pPlugin = new NDPluginTomo(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, maxMemory, priority, stackSize, maxThreads);
    return pPlugin->start();
}


/* IOC shell arguments passed to the plugin configure function */
static const iocshArg initArg0 = { "portName", iocshArgString };
static const iocshArg initArg1 = { "frame queue size", iocshArgInt };
static const iocshArg initArg2 = { "blocking callbacks", iocshArgInt };
static const iocshArg initArg3 = { "NDArrayPort", iocshArgString };
static const iocshArg initArg4 = { "NDArrayAddr", iocshArgInt };
static const iocshArg initArg5 = { "maxBuffers", iocshArgInt };
static const iocshArg initArg6 = { "maxMemory", iocshArgInt };
static const iocshArg initArg7 = { "priority", iocshArgInt };
static const iocshArg initArg8 = { "stackSize", iocshArgInt };
static const iocshArg initArg9 = { "maxThreads", iocshArgInt };
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};


// Define the path to your plugin's extern configure function above
static const iocshFuncDef initFuncDef = { "NDTomoConfigure", 10, initArgs };


/* link the configure function with the passed args, and call it from the IOC shell */
static void initCallFunc(const iocshArgBuf *args){
    NDTomoConfigure(
            args[0].sval, args[1].ival, args[2].ival,
            args[3].sval, args[4].ival, args[5].ival,
            args[6].ival, args[7].ival, args[8].ival, args[9].ival);
}


/* function to register the configure function in the IOC shell */
extern "C" void NDTomoRegister(void){
    iocshRegister(&initFuncDef,initCallFunc);
}


/* Exports plugin registration */
extern "C" {
    epicsExportRegistrar(NDTomoRegister);
}
