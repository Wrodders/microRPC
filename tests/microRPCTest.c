#include <stdio.h>
#include <stdlib.h>
#include "include/microRPCTest.h"
#include "include/helpersTest.h"


// Color codes for printing
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"


// *** // MICRO RPC FRAMEWORK TESTS // *** //


// *** // TEST Services // *** //

// Test Service 1
int test_service1(Command *cmd, char *response){
    // @brief: Test service 1
    // @return: 0 if successful, -1 if not

    //-------------MICRO RPC WRAPPER-------------//
    if(cmd->state != CMD_OK) return -1;
    Protocol *proto = cmd->proto; 

    // *** // STATIC ARGUMENTS // *** //
    char args[MAX_ARGS][MAX_ARG_SIZE] = {0};
    DataArg tmpArg;
    for(int i = 0; i < proto->tmp_argCount; i++){
        tmpArg = proto->msgDataFormat[i];
        uCTrunk(args[i], tmpArg.arg.buf, 0, tmpArg.arg.len);
    }
    //-------------MICRO RPC WRAPPER-------------//
    //-------------SERVICE CODE-------------//
    uCcpy(response, "TS1,OK");
    //-------------SERVICE CODE-------------//
    return 0;
}




int main(void){

    // *** // SETUP MICRO RPC // *** //
    RPCServiceManager rpcManager;
    initServiceManager(&rpcManager);
    Protocol proto = {
        .delim = ',',
        .maxArgSize = 10,
        .maxDataArgs = 2,
        .maxMsgLen = 22,
        .msgDataFormat = {
            {.id = "ND", .maxSize = 5},
            {.id = "DA", .maxSize = 10},
        },
        .tmp_argCount = 0
    };
    Service testService1;
    registerService(&rpcManager, &testService1, "S1", &proto, &test_service1);



    // RUN TESTS
    rpcError status = RPC_OK;
    char response[255] = {0};
    char input[255] = "S1,1234,ABCDEFGHI";


    printf("TEST 1: \n");

    runRPCManager(input, &rpcManager, response);


    return 0;
}