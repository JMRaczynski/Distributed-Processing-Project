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

struct heap initializeHeap(const int size);

struct request removeRoot(struct heap *heap);

void topDownRebuild(struct heap *heap, const int parentIndex);

void insertRequest(struct heap* heap, const int clockValue, const int processId);

void bottomUpRebuild(struct heap *heap, const int nodeIndex);

void swap (struct request *element1, struct request *element2);

void printHeap(const struct heap *heap);

struct list initializeList(const int size);

void shift(struct list *list, const int from, const int to);

int getIndexOf(const struct list *list, const int processId, const int resourceId);

void removeByIndex(struct list *list, const int index);

void removeByObject(struct list *list, const int processId, const int resourceId);

void insert(struct list *list, const struct listItem message);

void printList(const struct list *list);