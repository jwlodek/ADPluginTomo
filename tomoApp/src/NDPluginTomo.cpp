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
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "LOG  | %s::%s: %s\n", pluginName, functionName, msg)

#define LOG_ARGS(fmt, ...)                                                                       \
    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, "LOG  | %s::%s: " fmt "\n", pluginName, functionName, \
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
    NDArray *pScratch;
    asynStatus status = asynSuccess;
    NDArrayInfo arrayInfo;



    // If set to true, downstream plugins will perform callbacks on output pScratch
    // If false, no downstream callbacks will be performed
    bool performCallbacks = true;

    printf("Here\n");


    //call base class and get information about frame
    NDPluginDriver::beginProcessCallbacks(pArray);

    pArray->getInfo(&arrayInfo);

    printf("Here2\n");

    //unlock the mutex for the processing portion
    this->unlock();

    NSLS2TomoStreamProtocolHeader_t header;
    //header.frame_type = PROJECTION_FRAME;
    //header.reference_type = REF_TIMESTAMP;
    header.frame_type = 0;
    header.reference_type = 1;
    header.dataType = (uint8_t) pArray->dataType;
    header.reference = 3;

    header.num_bytes = arrayInfo.totalBytes;
    header.x_size = arrayInfo.xSize;
    header.y_size = arrayInfo.ySize;
    header.color_channels = arrayInfo.colorSize;
    //header.reference = pArray->timeStamp;

    printf("%d\n", sizeof(header));
    printf("%d\n", sizeof(double));
    printf("%d\n", sizeof(size_t));

    printf("TOMO Plugin Recvd frame %d\n", sizeof(NSLS2TomoStreamProtocolHeader_t));
    size_t ret = send(connfd, &header, 48, MSG_CONFIRM);
    printf("Sent %d bytes\n", ret);

    // Allocate memory for data to send over 
    void* imgData = calloc(arrayInfo.totalBytes, 1);
    memcpy(imgData, pArray->pData, arrayInfo.totalBytes);

    size_t ret_img = send(connfd, imgData, arrayInfo.totalBytes, MSG_CONFIRM);
    printf("Sent %d bytes of image data\n", ret_img);
    free(imgData);
    // If we are manipulating the image/output, we allocate a new scratch frame
    // You will need to specify dimensions, and data type.

    //pScratch = pNDArrayPool->alloc(ndims, dims, dataType, 0, NULL
    //if(pScratch == NULL){
    //    return;
    //}
    

    // Process the image here. pArray is read only, and if any image manipulation is required
    // a copy should be made into pScratch.
    // 
    // Note that this expects any external libraries to be thread safe. If they aren't, move
    // the processing to after this->lock();
    //
    // Access data with pArray->pData.
    // DO NOT CALL pArray.release()

    this->lock();

    // If pScratch was allocated, set the color mode and unique ID attributes here.

    //pScratch->pAttributeList->add("ColorMode", "Color Mode", NDAttrInt32, &colorMode);
    //pScratch->uniqueId = pArray->uniqueId;

    if(status == asynError){
        ERR("Image not processed correctly!");
        return;
    }

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
    int opt = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
    else {
	if (setsockopt(sockfd, SOL_SOCKET,
				SO_REUSEADDR | SO_REUSEPORT, &opt,
				sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	serveraddr->sin_family = AF_INET;
	serveraddr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr->sin_port = htons(PORT);

        if((bind(sockfd, (struct sockaddr*) serveraddr, sizeof(struct sockaddr))) != 0) {
            printf("Socket bind failed...\n");
        }
        else
            printf("Socket successfully binded..\n");


    if ((listen(sockfd, 3)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

    len = sizeof(struct sockaddr_in);
    connfd = accept(sockfd, (struct sockaddr*) clientaddr, (socklen_t*) &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");

    // Initialize Parameters here, using the string vals and indexes from the header. Ex:
    createParam(NDPluginTomoFrameIDString,  asynParamInt32, &NDTomo_FrameID);
    createParam(NDPluginTomoFrameTypeString,  asynParamInt32, &NDTomo_FrameType);
    createParam(NDPluginTomoConnectString,  asynParamInt32, &NDTomo_Connect);
    createParam(NDPluginTomoConnectionStatusString,  asynParamInt32, &NDTomo_ConnectionStatus);
    createParam(NDPluginTomoAngleIncrementString,  asynParamFloat64, &NDTomo_AngleIncrement);


        // Set some basic plugin info Params
        setStringParam(NDPluginDriverPluginType, "NDPluginTomo");
        epicsSnprintf(versionString, sizeof(versionString), "%d.%d.%d", TOMO_VERSION, TOMO_REVISION, TOMO_MODIFICATION);
        setStringParam(NDDriverVersion, versionString);
        connectToArrayPort();
    }
}


NDPluginTomo::~NDPluginTomo(){
    printf("Closing socket...\n");
    close(connfd);
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
