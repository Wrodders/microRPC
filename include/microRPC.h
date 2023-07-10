#ifndef UCOMMANDER_H
#define UCOMMANDER_H


// *** // Includes // *** //
#include "helpers.h"


/*
Messages are Passed as Packets of Data and may contain multiple arguments
Valid Messages inlude a SerivceId and a list of arguments
The Protocol defines the format of the arguemtns in the Data section of the message
The Protocl is used to decode the message data and pass it to the Service function

Services are stored in a hash table and are called by their ServiceId
Services are passed a Command object that contains the decoded message data



Protocol:
----------------------------------
| ServiceId | Data Length | Data | 
----------------------------------
| Hex Byte  | Hex Byte    | ...  |
----------------------------------

State Machine:
---------------------------
- Receive String from Transport Layer
- Decode ServiceId
- Validate message
- Pass data to service
- Encode response
----------------------------
*/


/*
Terminology:
Size: Size in Bytes of an Array
Len: Number of chars in an array not including '\0'
*/


// *** // Constants // *** //

const int SERVICE_ID_LEN = 2; 

// *** // Static Array Allocation // *** //
const int MAX_ID_SIZE = 3;
const int MAX_ARGS = 5;
const int MAX_ARG_SIZE = 10;
const int MAX_SERVICES = 5;


// Error Codes //
typedef enum rpcError{
    RPC_OK = 0,
    RPC_SERVICE_NOT_FOUND = -1,
    RPC_INVALID_ARG_LEN = -2,
    RPC_INVALID_ARG_COUNT = -3,
    INVALID_CMD_LEN = -4,
    RPC_INIT_ERROR = -5,
    RPC_ARG_NOT_FOUND = -6,
    RPC_NULL_POINTER = -7,
    RPC_REG_COLLISION = -8,
}rpcError;

typedef enum cmdState{
    CMD_OK = 0,
    CMD_INVALID = -1,
    CMD_TEMP = -2
}cmdState;

// *** // Data Structures // *** //
typedef struct uCStr{
    char *buf; 
    unsigned int len; // excluding null character
}uCStr; // A char buffer and its length

typedef struct DataArg{
    char id[MAX_ID_SIZE]; 
    int maxSize; // Max Size including null character
    uCStr arg;
}DataArg; 

typedef struct Protocol{
    char delim;
    unsigned int maxDataArgs;
    unsigned int maxMsgLen; // excluding '\0'
    unsigned int maxArgSize; // Max Size including null character
    DataArg msgDataFormat[MAX_ARGS]; // MALLOC THIS ??
    unsigned int tmp_argCount; // Number of arguments in the current message
}Protocol; 

typedef struct Command{
    char tartetServiceId[MAX_ID_SIZE];
    uCStr msgData;
    cmdState state;
    Protocol *proto; 
}Command; // A command that can be executed by an RPC Service

typedef rpcError (*rpcFunc)(Command *cmd, char *response);

typedef struct Service{
    char name[MAX_ID_SIZE]; 
    Protocol *proto; 
    rpcFunc func; 
}Service; // An executable function that can be called by a client

typedef struct RPCServericeManager{
    //cmdState state;
    Service *services[MAX_SERVICES];
    unsigned int count;

    Command cmd; // the current tocmand bieng excetued 
    rpcFunc serviceFunc;
}RPCServiceManager; // Manages RPC Services




// *** // Internal Functions // *** //  
unsigned int hash(char *key, int tableSize){
    int m = 31;
    unsigned int len = uCsize(key); 
    unsigned int hashValue = 0;
    for(int i = 0; i < len; i++){
        hashValue = (hashValue * m + key[i]);
    }
    return hashValue % tableSize;
}


rpcError updateCmd(RPCServiceManager *manager, const uCStr *msg){
    // @brief: Update the command with a new message
    // @return: 0 if successful, -1 if not

    // Check for null pointers
    Command *cmd = &manager->cmd;
    if(cmd == 0 || msg->buf == 0) return RPC_NULL_POINTER;

    // Identify target service
    int msgStart = uCTrunk(cmd->tartetServiceId, msg->buf, 0, SERVICE_ID_LEN) + 1;


    // update Command
    cmd->msgData.buf = msg->buf + msgStart; // point to start of message data
    cmd->msgData.len = msg->len - msgStart; // excluding service id
    cmd->state = CMD_TEMP; 

    return RPC_OK;
}

Service *getService(RPCServiceManager *manager, char *name){
    // @brief: Get a service from the manager by name
    // @return: Pointer to the service if found, 0 if not found

    // Check for null pointers
    if(manager == 0 || name == 0) return 0;
    int hId = hash(name, MAX_SERVICES); 
    if(manager->services[hId] == 0) return 0; // Service does not exist

    return manager->services[hId];
}


rpcError validateCommand(RPCServiceManager *manager){
    //@Brief: Validate a command
    //@Desc: Check target service exists, validate messageData against protocol

    // Check for null pointers
    Command *cmd = &manager->cmd;
    if(cmd == 0 || manager->count == 0) return RPC_NULL_POINTER;
    // Check if the target service exists
    Service *service = getService(manager, cmd->tartetServiceId);
    if(service == 0){
        cmd->state = CMD_INVALID;
        return RPC_SERVICE_NOT_FOUND;
    }
    // Validate Message Data
    Protocol *proto = service->proto;
    // Validate Arg Max Size, Arg Max Count, Max Msg Len
    if(cmd->msgData.len > proto->maxMsgLen) return INVALID_CMD_LEN; // Message is too long
    int argSize = 0; // Size of the current argument
    int argCount = 0;
    char msgChar;
    for(int i = 0; i < cmd->msgData.len + 1; i++){
        msgChar = cmd->msgData.buf[i];
        if(msgChar == proto->delim || msgChar == '\0'){
            if(argCount >= proto->maxDataArgs){
                cmd->state = CMD_INVALID;
                return RPC_INVALID_ARG_COUNT;
            }
            if(argSize > proto->msgDataFormat[argCount].maxSize){
                cmd->state = CMD_INVALID;
                return RPC_INVALID_ARG_LEN;
            }
            // Point to start of argument using offset
            proto->msgDataFormat[argCount].arg.buf = cmd->msgData.buf + (i - argSize); 
            proto->msgDataFormat[argCount].arg.len = argSize; // excuding delim
            argSize = 0; // reset 
            argCount++; 
        }
        else{
            argSize++; // 
        }
    }
    cmd->proto = proto; // Save the protocol for the target service
    manager->serviceFunc = service->func; // Save the function for the target service
    cmd->proto->tmp_argCount = argCount; // Save the number of arguments in the message
    cmd->state = CMD_OK;
    return RPC_OK;
}



rpcError extractDataArg(const char *argId, Protocol *proto, char *argBuf){
    //@Brief: Extract the data arguments from a message
    //@Desc: Copys the data argument by id into the argBuf

    if(proto->maxDataArgs == 0 || argBuf == 0) return RPC_NULL_POINTER; 
    DataArg *tmpArg = 0;
    for(int i = 0; i < proto->maxDataArgs; i ++){
        tmpArg = &proto->msgDataFormat[i];
        if(uStrcmp(tmpArg->id, argId) == 0){
            uCTrunk(argBuf, tmpArg->arg.buf, 0, tmpArg->arg.len);
            return RPC_OK;
        }
    }
    return RPC_ARG_NOT_FOUND;
}

rpcError clearDataArgs(Protocol *proto){
    //@Brief: Clear the data arguments
    //@Desc: Clear the data arguments in the protocol

    if(proto == 0) return RPC_NULL_POINTER;
    for(int i = 0; i < proto->maxDataArgs; i++){
        proto->msgDataFormat[i].arg.buf = 0;
        proto->msgDataFormat[i].arg.len = 0;
    }
    return RPC_OK;
}

rpcError clearCommand(Command *cmd){
    //@Brief: Clear the command
    //@Desc: Clear the command data

    if(cmd == 0) return RPC_NULL_POINTER;
    cmd->tartetServiceId[0] = '\0';
    cmd->msgData.buf = 0;
    cmd->msgData.len = 0;
    cmd->state = CMD_INVALID;
    clearDataArgs(cmd->proto);
    cmd->proto = 0;
    return RPC_OK;
}


// *** // Public Functions // *** //

int initServiceManager(RPCServiceManager *manager){
    // @Brief: Initialize the RPC Service Manager
    // @Desc: Set the function pointers and initialize the services array to 0
    // @Return: 0 if successful, -1 if not

    // Check for null pointers
    if(manager == 0) return -1;
    // Initialize services array to 0
    for(int i = 0; i < MAX_SERVICES; i++){
        manager->services[i] = 0;
    }
    manager->count = 0;
    return RPC_OK;
}

rpcError registerService(RPCServiceManager *manager, Service *service, char *name, Protocol *proto, rpcFunc func){
    // @Breif: Register a service with the RPC Service Manager
    // Check for null pointers
    if(manager == 0 || service == 0 || name == 0 || proto == 0 || func == 0) return -1;

    unsigned int hId = hash(name, MAX_SERVICES); 
    if(manager->services[hId] != 0) return RPC_REG_COLLISION; // Service already exists

    // Assign the service values
    uCcpy(service->name, name);
    service->proto = proto;
    service->func = func;

    // Add the service to the manager
    manager->services[hId] = service;
    manager->count++;
    return RPC_OK;
}

rpcError runRPCManager(char *message, RPCServiceManager *manager, char *response){
    // @Brief: Execute a service by name
    // @Return: 0 if successful, -1 if not
    rpcError err = RPC_OK;
    // FIX: verify message len before allocationg

    uCStr msg = {
        .buf = message,
        .len = uCsize(message) -1 // remove the null terminator
    };

    // UPDATE COMMAND
    err = updateCmd(manager, &msg);
    err = validateCommand(manager);
    if(manager->cmd.state != CMD_OK || response == 0) return err;
    
    // execute the service function and return the status code
    return manager->serviceFunc(&manager->cmd, response); 
}

void cleanRPCManager(RPCServiceManager *manager){
    //@Breif: Cleans up data for next Command
    //Desc: Resets data buffers and varaibales

    clearDataArgs(manager->cmd.proto);
    clearCommand(&manager->cmd);
    manager->serviceFunc = 0; // Clear the function pointer
    return;
}

#endif 


