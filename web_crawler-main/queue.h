#include<stdio.h>
#include<stdlib.h>

/*
  Linked List implementation of a queue
  
  Supports enqueue, dequeue, len, isEmpty

  To use a new queue initialize in the following way
    queue *q;
    q = malloc(sizeof(queue));
    initQueue(q);
*/

// node data structure
typedef struct node{
    char *data;
    struct node *next;
} node;

// queue data structure
typedef struct queue{
    unsigned int size;
    node *front;
    node *back;
} queue;

// return 1 if empty, 0 if not
int isEmpty(queue *q){
    if(q->size == 0){
        return 1;
    }
    return 0;
}

// inserts item into queue, returns 1 if queue or data is null, 0 on successful insertion
int enqueue(char *data, queue *q){
    if(q == NULL){
        printf("Null queue pointer\n");
        return 1;
    }
    if(data == NULL){
        printf("Null data pointer\n");
        return 1;
    }
    node *tmp = (node *)malloc(sizeof(node));
    tmp->data = data;
    tmp->next = NULL;
    if(q->size == 0){
        q->front = tmp;
    }
    else if(q->size == 1){
    	q->back = tmp;
    	q->front->next = q->back;
    }
    else{
        q->back->next = tmp;
        q->back = q->back->next;
    }
    q->size++;
    return 0;
}

// returns first item in queue, if queue is empty returns a null pointer
char *dequeue(queue *q){
    if(isEmpty(q) == 1){
        printf("Empty Queue\n");
        return NULL;
    }
    node *tmp;
    char *returnString = q->front->data;
    tmp = q->front;
    q->front = q->front->next;
    q->size--;
    free(tmp);
    return returnString;
}

// returns number of elements in queue
int len(queue *q){
    return q->size;
}

// initializes queue
void initQueue(queue *q){
    q->front = NULL;
    q->back = NULL;
    q->size = 0;
}
