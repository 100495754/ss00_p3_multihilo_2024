//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

//To create a queue
queue* queue_init(int size)
{

  queue * q = (queue *)malloc(sizeof(queue));
  if (q == NULL) {
      printf("Memory queue allocation failed.\n");
      return NULL;
  }
  q->items = (struct element *)malloc(sizeof(element) * size);
  if (q == NULL) {
      printf("Memory items allocation failed.\n");
      return NULL;
  }
  q->capacity = size;
  q->front = 0;
  q->rear = -1;
  q->count = 0;
  return q;
}

// To Enqueue an element
int queue_put(queue *q, struct element* x)
{
    if (queue_full(q)) return -1;
    q->rear = (q->rear + 1) % q->capacity;
    q->items[q->rear] = *x;
    q->count++;
  
  return 0;
}

// To Dequeue an element.
struct element* queue_get(queue *q)
{
  struct element* element;
  element = &q->items[q->front];
  q->front = (q->front + 1) % q->capacity;
  q->count--;
  
  return element;
}

//To check queue state
int queue_empty(queue *q)
{
  if (q->count == 0){
      return 1;
  }
  return 0;
}

int queue_full(queue *q)
{
    if(q->count == q->capacity) {
        return 1;
    }
  return 0;
}

//To destroy the queue and free the resources
int queue_destroy(queue *q)
{
    free(q->items);
    free(q);
    return 0;
}
