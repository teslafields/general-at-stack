#include "at-interface.h"


ATQueue* createQueue(unsigned capacity)
{
    unsigned int i;
    ATQueue* queue = (ATQueue*) malloc(sizeof(ATQueue));
    queue->capacity = capacity;
    queue->front = 0;
    queue->size = 0;
    queue->rear = capacity - 1;
    queue->cmd = (char**) malloc(queue->capacity * sizeof(char *));
    for(i=0; i<capacity; i++)
        queue->cmd[i] = NULL;
    return queue;
}

int isFull(ATQueue* queue)
{
    return (queue->size == queue->capacity);
}

int isEmpty(ATQueue* queue)
{
    return (queue->size == 0);
}

void enqueue(ATQueue* queue, char* item)
{
    if (isFull(queue) || strlen(item) < 1 || strlen(item) > MAX_SIZE)
        return;
    int item_sz = strlen(item) + 1;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->cmd[queue->rear] = (char*) malloc(item_sz);
    strncpy(queue->cmd[queue->rear], item, item_sz);
    queue->cmd[queue->rear][item_sz-1] = '\0';
    queue->size = queue->size + 1;
#ifdef QDEBUG
    printf("+enq %p: \"%s\"\n", queue, item);
#endif
}

char* dequeue(ATQueue* queue)
{
    if (isEmpty(queue))
        return NULL;
    static char item[MAX_SIZE];
    strcpy(item, queue->cmd[queue->front]);
    free(queue->cmd[queue->front]);
    queue->cmd[queue->front] = NULL;
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
#ifdef QDEBUG
    printf("-deq %p: \"%s\"\n", queue, item);
#endif
    return item;
}

void clearQueue(ATQueue* queue)
{
    while(queue->size)
        dequeue(queue);
}

char* front(ATQueue* queue)
{
    if (isEmpty(queue))
        return NULL;
    return queue->cmd[queue->front];
}

char* rear(ATQueue* queue)
{
    if (isEmpty(queue))
        return NULL;
    return queue->cmd[queue->rear];
}

void destroyQueue(ATQueue* queue)
{
    clearQueue(queue);
    free(queue->cmd);
    free(queue);
}

void displayQueue(ATQueue* queue)
{
    clearQueue(queue);
    free(queue->cmd);
    free(queue);
}
