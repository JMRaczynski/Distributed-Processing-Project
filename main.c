#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
    if (t < 0 || g < 0 || o < 0 || b < 0) return 0;
    if (o + t < 1) return 0;
    return 1;
}

void finish(const char message[], const int rank) {
    if (rank == 0) printf(message);
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
    
    if (!areArgumentsCorrect(NUMBER_OF_GOOD_GUYS, NUMBER_OF_BAD_GUYS, // sprawdzenie poprawności argumentów
                            NUMBER_OF_PLANTS, NUMBER_OF_TOILETS, size)) {
        finish("Wrong arguments passed\n", rank);
    }

    int lamportClock = 0;
    char nieWiemJakNazwacTeZmienna = 1;
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
        int chosenResource;
        if (nieWiemJakNazwacTeZmienna) {
            nieWiemJakNazwacTeZmienna = 0;
            chosenResource = chooseResourceFirstTime(resourcesStates, NUMBER_OF_RESOURCES, PROCESS_TYPE);
        }
        else {
            chosenResource = chooseResource(requestQueues, NUMBER_OF_RESOURCES, PROCESS_TYPE, OPPOSITE_TYPE);
                    printf("ZASOB: %d\n", chosenResource);

        }
        lamportClock = incrementLamportClock(lamportClock);
        insertRequest(&requestQueues[PROCESS_TYPE][chosenResource], lamportClock, rank);
        broadcastMessage(rank, chosenResource, REQ, size, lamportClock);

        MPI_Recv(receivedMessageBuffer, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        printf("Proces %d odebrał żądanie procesu %d. Proces %d prosi o zasob nr %d z zegarem %d\n",
                    rank, status.MPI_SOURCE, status.MPI_SOURCE, receivedMessageBuffer[1], receivedMessageBuffer[0]);
        
    }
    


    /*if (rank == 0) {
        insertRequest(&requestQueues[1][1], 1, 4);
        insertRequest(&requestQueues[1][1], 8, 4);
        insertRequest(&requestQueues[1][1], 4, 4);
        insertRequest(&requestQueues[1][1], 3, 4);
        removeRoot(&requestQueues[1][1]);
        insertRequest(&requestQueues[1][1], 9, 4);
        insertRequest(&requestQueues[1][1], 6, 4);
        insertRequest(&requestQueues[1][1], 2, 4);
        insertRequest(&requestQueues[1][1], 3, 2);
        removeRoot(&requestQueues[1][1]);
        insertRequest(&requestQueues[1][1], 5, 4);
        insertRequest(&requestQueues[1][1], 7, 4);
        insertRequest(&requestQueues[1][1], 1, 2);
        removeRoot(&requestQueues[1][1]);

        printHeap(&requestQueues[1][1]);

        struct listItem m;
        m.processId = 1;
        m.resourceId = 1;
        insert(&releaseQueues[1][1], m);
        printList(&releaseQueues[1][1]);
        m.processId = 99;
        m.resourceId = 356;
        insert(&releaseQueues[1][1], m);
        m.processId = -1124;
        m.resourceId = 35;
        insert(&releaseQueues[1][1], m);
        m.processId = -11;
        m.resourceId = 33;
        insert(&releaseQueues[1][1], m);
        removeByObject(&releaseQueues[1][1], -1124, 35);
        printList(&releaseQueues[1][1]);
        m.resourceId = 39;
        m.processId = -132;
        insert(&releaseQueues[1][1], m);
        removeByObject(&releaseQueues[1][1], -132, 39);
        m.resourceId = 3413;
        m.processId = 112;
        insert(&releaseQueues[1][1], m);
        printList(&releaseQueues[1][1]);
    }*/
    MPI_Finalize();
}