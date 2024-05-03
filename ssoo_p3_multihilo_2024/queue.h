//SSOO-P3 23/24

#ifndef HEADER_FILE
#define HEADER_FILE


struct element {
  int product_id; //Product identifier
  int op;         //Operation
  int units;      //Product units
};

typedef struct queue {
  // Define the struct yourself
    struct element *items; // Array de elementos
    int capacity;   // Capacidad máxima de la cola
    int front;      // Índice del primer elemento
    int rear;       // Índice del último elemento
    int count;      // Número de elementos en la cola
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
