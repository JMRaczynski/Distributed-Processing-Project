#include <math.h>
#include <stdio.h>
#include <stdlib.h>

struct request {
    int clockValue;
    int processId;
};

struct heap {
    struct request *array;
    int size;
    int allocatedMemory;
};

struct listItem {
    int processId;
    int resourceId;
};

struct list {
    struct listItem *array;
    int size;
    int allocatedMemory;
};

struct heap initializeHeap(const int size) {
    struct heap heap;
    heap.array = malloc(sizeof(struct request) * size);
    heap.allocatedMemory = size;
    heap.size = 0;
    return heap;
}

void swap (struct request *element1, struct request *element2) {
    int tmpId = element1->processId;
    int tmpClock = element1->clockValue;
    element1->processId = element2->processId;
    element1->clockValue = element2->clockValue;
    element2->processId = tmpId;
    element2->clockValue = tmpClock;
}

void topDownRebuild(struct heap *heap, const int parentIndex) {
    int leftChildIndex = parentIndex * 2;
    int rightChildIndex = leftChildIndex + 1;
    if (heap->size >= rightChildIndex) { // wierzchołek ma dwoje dzieci
        if (heap->array[parentIndex].clockValue < heap->array[leftChildIndex].clockValue
        && heap->array[parentIndex].clockValue < heap->array[rightChildIndex].clockValue) return;
        if (heap->array[leftChildIndex].clockValue <= heap->array[rightChildIndex].clockValue) {
            if (heap->array[leftChildIndex].clockValue < heap->array[parentIndex].clockValue
                    || heap->array[leftChildIndex].processId < heap->array[parentIndex].processId) {
                swap(&heap->array[leftChildIndex], &heap->array[parentIndex]);
                topDownRebuild(heap, leftChildIndex);
            }
        }
        else {
            if (heap->array[rightChildIndex].clockValue < heap->array[parentIndex].clockValue
                    || heap->array[rightChildIndex].processId < heap->array[parentIndex].processId) {
                swap(&heap->array[rightChildIndex], &heap->array[parentIndex]);
                topDownRebuild(heap, rightChildIndex);
            }
        }
    }
    if (heap->size == leftChildIndex) { // wierzchołek ma tylko jedno dziecko
        if (heap->array[leftChildIndex].clockValue < heap->array[parentIndex].clockValue
            || heap->array[leftChildIndex].processId < heap->array[parentIndex].processId) {
            swap(&heap->array[leftChildIndex], &heap->array[parentIndex]);
        }
    }
    
}

void bottomUpRebuild(struct heap *heap, const int nodeIndex) {
    int parentIndex = nodeIndex / 2;
    if (parentIndex > 0) {
        if (heap->array[parentIndex].clockValue > heap->array[nodeIndex].clockValue
                || (heap->array[parentIndex].processId > heap->array[nodeIndex].processId
                && heap->array[parentIndex].clockValue == heap->array[nodeIndex].clockValue)) {
            swap(&heap->array[nodeIndex], &heap->array[parentIndex]);
            bottomUpRebuild(heap, parentIndex);
        }
    }
}

struct request removeRoot(struct heap *heap) {
    struct request result = heap->array[1];
    heap->array[1] = heap->array[heap->size--];
    topDownRebuild(heap, 1);
    return result;
}

void insertRequest(struct heap* heap, const int clockValue, const int processId) {
    struct request newRequest;
    newRequest.clockValue = clockValue;
    newRequest.processId = processId;
    heap->array[++heap->size] = newRequest;
    if (heap->allocatedMemory - heap->size < 10) {
        heap->allocatedMemory += 100;
        heap->array = realloc(heap->array, sizeof(struct request) * heap->allocatedMemory);
    }
    bottomUpRebuild(heap, heap->size);
}

void printHeap(const struct heap *heap) {
    double heapHeight = log(heap->size) / log(2) + 1;
    for (int i = 1; i <= heapHeight; i++) {
        for (double j = pow(2, i - 1); j < pow(2, i); j++) {
            printf("%d %d\t", heap->array[(int) j].clockValue, heap->array[(int) j].processId);
            if (j == heap->size) break;
        }
        printf("\n");
    }
}

struct list initializeList(const int size) {
    struct list list;
    list.array = malloc(size * sizeof(struct listItem));
    list.allocatedMemory = size;
    list.size = 0;
    return list;
}

void shift(struct list *list, const int from, const int to) {
    for (int i = from; i < to;) {
        list->array[i].processId = list->array[i + 1].processId;
        list->array[i].resourceId = list->array[i + 1].resourceId;
        i++;
    }
}

int getIndexOf(const struct list *list, const int processId, const int resourceId) {
    for (int i = 0; i < list->size; i++) {
        if (list->array[i].processId == processId && list->array[i].resourceId == resourceId) return i;
    }
    return -1;
}

void removeByIndex(struct list *list, const int index) {
    shift(list, index, list->size - 1);
    list->size--;
}

void removeByObject(struct list *list, const int processId, const int resourceId) {
    removeByIndex(list, getIndexOf(list, processId, resourceId));
}

void insert(struct list *list, const struct listItem message) {
    list->array[list->size].processId = message.processId;
    list->array[list->size++].resourceId = message.resourceId;
    if (list->allocatedMemory - list->size < 10) {
        list->allocatedMemory += 100;
        list->array = realloc(list->array, sizeof(struct request) * list->allocatedMemory);
    }
}

void printList(const struct list *list) {
    printf("LISTA:\n");
    for (int i = 0; i < list->size; i++) {
        printf("INDEKS: %d ID PROCESU: %d ID ZASOBU: %d\n", i, list->array[i].processId, list->array[i].resourceId);
    }
    printf("\n");
}