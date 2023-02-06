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

using namespace std;

//include base plugin driver
#include "NDPluginDriver.h"

//version numbers
#define TOMO_VERSION      	0
#define TOMO_REVISION     	0
#define TOMO_MODIFICATION 	0



// Define the PVStrings for all of your PV values here in the following format
//#define NDPluginTomoPVNameString 	"TOMO_PVNAME" 		//DTYP (ex. asynInt32, asynFloat64, asynOctet)


// Define all necessary tpyes, structs, and enums here


/* Plugin class, extends plugin driver */
class NDPluginTomo : public NDPluginDriver {
    public:
        NDPluginTomo(const char *portName, int queueSize, int blockingCallbacks,
            const char* NDArrayPort, int NDArrayAddr, int maxBuffers,
            size_t maxMemory, int priority, int stackSize, int maxThreads);


        void processCallbacks(NDArray *pArray);

        virtual asynStatus writeInt32(asynUser* pasynUser, epicsInt32 value);

    protected:

        // Define the Param index variables here. Ex:
        // int NDPluginTomoPVName;


        // Define these two variables as the first and last param indexes.
        #define ND_TOMO_FIRST_PARAM 0
        #define ND_TOMO_LAST_PARAM 0 

    private:

        // init all global variables here

        // init all plugin additional functions here

};

// Def that computes the number of params specific to the plugin
#define NUM_TOMO_PARAMS ((int)(&ND_TOMO_LAST_PARAM - &ND_TOMO_FIRST_PARAM + 1))

#endif
