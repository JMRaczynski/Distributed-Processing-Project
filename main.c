#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <mpi.h>

#define GOOD_GUY 0
#define BAD_GUY 1
#define REPAIRED 1
#define BROKEN 0

#define REQ 1
#define ACK 2
#define RELEASE 3

int areArgumentsCorrect(const int g, const int b, const int o, const int t, const int size) {
    if (b + g != size) return 0;
    if (t < 0 || g <= 0 || o < 0 || b <= 0) return 0;
    if (o + t < 1) return 0;
    return 1;
}

void finish(const char message[], const int rank) {
    if (rank == 0) printf("%s", message);
    MPI_Finalize();
    exit(0);
}

int chooseResourceFirstTime(const char *resourcesStates, const int numberOfResources, const int processType) {
    struct list availableResources = initializeList(10);
    struct listItem resource;
    resource.processId = -1;
    for (int i = 0; i < numberOfResources; i++) {
        if ((processType == GOOD_GUY && resourcesStates[i] == BROKEN) || 
                (processType == BAD_GUY && resourcesStates[i] == REPAIRED)) {
            resource.resourceId = i;
            insert(&availableResources, resource);
        }
    }
    if (availableResources.size == 0) return 0;
    return availableResources.array[rand() % availableResources.size].resourceId;
}

int chooseResource(struct heap** requestQueues, const int numberOfResources, const int processType, const int oppositeType) { // MOZE SIE WYWALAC!
    struct list groupedResources[5];
    struct listItem resource;
    resource.processId = -1;
    for (int i = 0; i < 5; i++) groupedResources[i] = initializeList(10);
    for (int resourceId = 0; resourceId < numberOfResources; resourceId++) {
        resource.resourceId = resourceId;
        if (requestQueues[processType][resourceId].size == 0 && requestQueues[oppositeType][resourceId].size != 0) insert(&groupedResources[0], resource);
        else if (requestQueues[processType][resourceId].size == 0 && requestQueues[oppositeType][resourceId].size == 0) insert(&groupedResources[1], resource);
        else if (requestQueues[processType][resourceId].size < requestQueues[oppositeType][resourceId].size) insert(&groupedResources[2], resource);
        else if (requestQueues[processType][resourceId].size == requestQueues[oppositeType][resourceId].size) insert(&groupedResources[3], resource);
        else if (requestQueues[processType][resourceId].size > requestQueues[oppositeType][resourceId].size) insert(&groupedResources[4], resource);
    }
    int indexOfHighestNonEmptyList;
    for (int i = 0; i < 5; i++) {
        if (groupedResources[i].size > 0) {
            indexOfHighestNonEmptyList = i;
            break;
        }
    }
    int resourceIndex = rand() % groupedResources[indexOfHighestNonEmptyList].size;
    return groupedResources[indexOfHighestNonEmptyList].array[resourceIndex].resourceId;
}

int incrementLamportClock(int lamportClock) {
    return lamportClock + 1;
}

void broadcastMessage(const int senderId, const int resourceId, const int tag, const int numberOfProcesses, const int clockValue) {
    int messageData[2];
    messageData[0] = clockValue;
    messageData[1] = resourceId;
    for (int receiverId = 0; receiverId < numberOfProcesses; receiverId++) {
        if (receiverId != senderId) MPI_Send(messageData, 2, MPI_INT, receiverId, tag, MPI_COMM_WORLD);
    }
}

int updateLamportClock(const int myLamportClock, const int incomingLamportClock) {
    int biggerClock = myLamportClock >= incomingLamportClock ? myLamportClock : incomingLamportClock;
    return biggerClock + 1;
}

void sendAcceptMessage(const int recipientId, const int clockValue) {
    int messageData[2] = {0};
    messageData[0] = clockValue;
    MPI_Send(messageData, 2, MPI_INT, recipientId, ACK, MPI_COMM_WORLD);
}

char checkIfAcceptedByEveryone(const char* ackList, const int numberOfProcesses) {
    for (int i = 0; i < numberOfProcesses; i++) {
        if (ackList[i] == 0) return 0;
    }
    return 1;
}

void enterCriticalSection(const int processId, char* processType, const int resourceId, int *lamportClock, char* chosenResourceTypeString, char* action) {
    printf("\033[1;31m");
    printf("\tZegar Lamporta - %d - %s o id %d %s %s o id %d\n", *lamportClock, processType, processId, action, chosenResourceTypeString, resourceId);
    printf("\033[0m");
    usleep(rand() % 600000 + 10000);
    printf("\033[1;32m");
    *lamportClock = incrementLamportClock(*lamportClock);
    printf("\tZegar Lamporta - %d - %s o id %d skonczyl %s %s o id %d\n", *lamportClock, processType, processId, action, chosenResourceTypeString, resourceId);
    printf("\033[0m");
}

char checkIfRequestIsOnTopOfQueue(struct heap** requestQueues, const int resourceId, const int processId, const int processType) {
    //printf("%d %d wtf %d\n", processType, resourceId, processId);
    //printHeap(&requestQueues[processType][resourceId]);
    return requestQueues[processType][resourceId].array[1].processId == processId;
}

int removePendingReleases(int resourceId, char* resourcesStates, struct list** releaseQueues, struct heap** requestQueues) {
    struct request request;
    int desiredProcessType;
    int targetResourceState;
    int releaseIndex;
    int result;
    while (1) {
        if (resourcesStates[resourceId] == BROKEN) {
            desiredProcessType = GOOD_GUY;
            targetResourceState = REPAIRED;
        }
        else {
            desiredProcessType = BAD_GUY;
            targetResourceState = BROKEN;
        }
        if (requestQueues[desiredProcessType][resourceId].size != 0) {
            int firstProcessId = requestQueues[desiredProcessType][resourceId].array[1].processId;
            releaseIndex = getIndexOf(&releaseQueues[desiredProcessType][resourceId], firstProcessId, resourceId);
            if (releaseIndex != -1) {
                removeByIndex(&releaseQueues[desiredProcessType][resourceId], releaseIndex);
                resourcesStates[resourceId] = targetResourceState;
                request = removeRoot(&requestQueues[desiredProcessType][resourceId]);
            }
            else {
                result = firstProcessId;
                break;
            }
        }
        else {
            result = -1;
            break;
        }
    }
    return result;
}

int main(int argc, char **argv) {

    MPI_Init(&argc, &argv); // inicjalizacja MPI
    int size, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status status;

    if (argc < 5) finish("Not enough arguments, 4 integers needed\n", rank);
    const int NUMBER_OF_GOOD_GUYS = atoi(argv[1]); // stałe
    const int NUMBER_OF_BAD_GUYS = atoi(argv[2]);
    const int NUMBER_OF_TOILETS = atoi(argv[3]);
    const int NUMBER_OF_PLANTS = atoi(argv[4]);
    const int NUMBER_OF_RESOURCES = NUMBER_OF_TOILETS + NUMBER_OF_PLANTS;
    const int PROCESS_TYPE = rank < NUMBER_OF_GOOD_GUYS ? GOOD_GUY : BAD_GUY;
    const int OPPOSITE_TYPE = rank < NUMBER_OF_GOOD_GUYS ? BAD_GUY : GOOD_GUY;
    char* PROCESS_TYPE_STRING = PROCESS_TYPE == GOOD_GUY ? "Dobrodziej" : "Zlodziej";
    char* ACTION_STRING = PROCESS_TYPE == GOOD_GUY ? "naprawia" : "psuje";
    const char TOILET[] = "toalete";
    const char PLANT[] = "doniczke";
    char* chosenResourceTypeString = malloc(10);
    
    if (!areArgumentsCorrect(NUMBER_OF_GOOD_GUYS, NUMBER_OF_BAD_GUYS, // sprawdzenie poprawności argumentów
                            NUMBER_OF_PLANTS, NUMBER_OF_TOILETS, size)) {
        finish("Wrong arguments passed\n", rank);
    }

    int lamportClock = 0;
    char nieWiemJakNazwacTeZmienna = 1;
    char isOnTopOfQueue, isAcceptedByEveryone;
    char awaitingCriticalSection;
    int receivedMessageBuffer[2];
    srand(time(0) + rank);

    struct heap* requestQueues[2]; // inicjalizacja kolejek żądań
    for (int i = 0; i < 2; i++) {
        requestQueues[i] = (struct heap *)malloc(NUMBER_OF_RESOURCES * sizeof(struct heap));
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            requestQueues[i][j] = initializeHeap(10);
        }
    }

    char* resourcesStates = malloc(NUMBER_OF_RESOURCES); // inicjalizacja tablicy stanu zasobów
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        resourcesStates[i] = i % 2;
    }
    char* ackList = malloc(size); // deklaracja tablicy ACK

    struct list* releaseQueues[2]; // inicjalizacja kolejek wiadomości RELEASE
    for (int i = 0; i < 2; i++) {
        releaseQueues[i] = (struct list *)malloc(NUMBER_OF_RESOURCES * sizeof(struct list));
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            releaseQueues[i][j] = initializeList(10);
        }
    }

    while(1) {
        for (int i = 0; i < size; i++) ackList[i] = 0;
        ackList[rank] = 1;
        isOnTopOfQueue = isAcceptedByEveryone = 0;
        int chosenResource;
        if (nieWiemJakNazwacTeZmienna) {
            nieWiemJakNazwacTeZmienna = 0;
            chosenResource = chooseResourceFirstTime(resourcesStates, NUMBER_OF_RESOURCES, PROCESS_TYPE);
        }
        else {
            chosenResource = chooseResource(requestQueues, NUMBER_OF_RESOURCES, PROCESS_TYPE, OPPOSITE_TYPE);
        }
        if (chosenResource < NUMBER_OF_TOILETS) strcpy(chosenResourceTypeString, TOILET);
        else strcpy(chosenResourceTypeString, PLANT);

        //printf("%s %d ubiega sie o zasob %d\n", PROCESS_TYPE_STRING, rank, chosenResource);
        lamportClock = incrementLamportClock(lamportClock);
        awaitingCriticalSection = 1;
        insertRequest(&requestQueues[PROCESS_TYPE][chosenResource], lamportClock, rank);
        broadcastMessage(rank, chosenResource, REQ, size, lamportClock);
        while(awaitingCriticalSection) {
            MPI_Recv(receivedMessageBuffer, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_TAG == 3) printf("%s %d otrzymal wiadomosc release dotyczaca zasobu %d od procesu %d z zegarem %d\n", PROCESS_TYPE_STRING, rank, receivedMessageBuffer[1], status.MPI_SOURCE, receivedMessageBuffer[0]);
            //printf("Proces %d otrzymal wiadomosc typu %d od procesu %d\n", rank, status.MPI_TAG, status.MPI_SOURCE);
            int incomingClockValue = receivedMessageBuffer[0];
            int requestedResourceId;
            int releasedResourceId;
            int senderType;
            lamportClock = updateLamportClock(lamportClock, incomingClockValue);
            switch (status.MPI_TAG) {
            case REQ:
                requestedResourceId = receivedMessageBuffer[1];
                senderType = status.MPI_SOURCE < NUMBER_OF_GOOD_GUYS ? GOOD_GUY : BAD_GUY;
                insertRequest(&requestQueues[senderType][requestedResourceId], incomingClockValue, status.MPI_SOURCE);
                sendAcceptMessage(status.MPI_SOURCE, lamportClock);
                lamportClock = incrementLamportClock(lamportClock);
                break;
            case ACK:
                ackList[status.MPI_SOURCE] = 1;
                isAcceptedByEveryone = checkIfAcceptedByEveryone(ackList, size);
                isOnTopOfQueue = checkIfRequestIsOnTopOfQueue(requestQueues, chosenResource, rank, PROCESS_TYPE);
                //printf("ID procesu: %d Czy na szczycie :%d Czy zaakceptowany: %d Stan procesu: %d\n", rank, isOnTopOfQueue, isAcceptedByEveryone, resourcesStates[chosenResource]);
                if (isAcceptedByEveryone && isOnTopOfQueue && resourcesStates[chosenResource] == PROCESS_TYPE) {
                    enterCriticalSection(rank, PROCESS_TYPE_STRING, chosenResource, &lamportClock, chosenResourceTypeString, ACTION_STRING);
                    awaitingCriticalSection = 0;
                    broadcastMessage(rank, chosenResource, RELEASE, size, lamportClock);
                    removeRoot(&requestQueues[PROCESS_TYPE][chosenResource]);
                    resourcesStates[chosenResource] = PROCESS_TYPE == GOOD_GUY ? REPAIRED : BROKEN;
                }
                break;
            case RELEASE:
                releasedResourceId = receivedMessageBuffer[1];
                senderType = status.MPI_SOURCE < NUMBER_OF_GOOD_GUYS ? GOOD_GUY : BAD_GUY;
                if (resourcesStates[releasedResourceId] == senderType
                        && checkIfRequestIsOnTopOfQueue(requestQueues, releasedResourceId, status.MPI_SOURCE, senderType)) {
                    removeRoot(&requestQueues[senderType][releasedResourceId]);
                    resourcesStates[releasedResourceId] = senderType == GOOD_GUY ? REPAIRED : BROKEN; 
                    int nextProcessId = removePendingReleases(releasedResourceId, resourcesStates, releaseQueues, requestQueues);
                    if (chosenResource == releasedResourceId && nextProcessId == rank && isAcceptedByEveryone) {
                        enterCriticalSection(rank, PROCESS_TYPE_STRING, chosenResource, &lamportClock, chosenResourceTypeString, ACTION_STRING);
                        awaitingCriticalSection = 0;
                        broadcastMessage(rank, chosenResource, RELEASE, size, lamportClock);
                        removeRoot(&requestQueues[PROCESS_TYPE][chosenResource]);
                        resourcesStates[chosenResource] = PROCESS_TYPE == GOOD_GUY ? REPAIRED : BROKEN;
                    }
                }
                else {
                    struct listItem newPendingRelease;
                    newPendingRelease.processId = status.MPI_SOURCE;
                    newPendingRelease.resourceId = releasedResourceId;
                    insert(&releaseQueues[senderType][releasedResourceId], newPendingRelease);
                }
                break;
            }
        }
    }
}