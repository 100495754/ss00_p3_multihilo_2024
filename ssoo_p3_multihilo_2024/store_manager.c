//SSOO-P3 23/24

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include "queue.h"


queue* sharedQueue;
sem_t sem_full, sem_empty;
pthread_mutex_t mutex;

void *producir(int fd){
    while (1) {  // Cambiar por una condición real de parada
        sem_wait(&sem_empty);
        pthread_mutex_lock(&mutex);

        // Producir un elemento y encolarlo
        struct element* newElem = ;  // Suponiendo que esto es lo que se encola
        // Configurar newElem según sea necesario
        queue_put(sharedQueue, newElem);

        pthread_mutex_unlock(&mutex);
        sem_post(&sem_full);
    }
    return NULL;
}

void *consumir(){
    while (1) {  // Cambiar por una condición real de parada
        sem_wait(&sem_full);
        pthread_mutex_lock(&mutex);

        // Desencolar un elemento y consumirlo
        struct element* item = queue_get(sharedQueue);
        // Consumir el elemento item

        pthread_mutex_unlock(&mutex);
        sem_post(&sem_empty);
    }
    return NULL;
}


int main (int argc, const char * argv[])
{
  int profits = 0;
  int product_stock [5] = {0};

  int fd = open(argv[0], O_RDONLY);
  if (fd == -1) {
      perror("Error opening file");
      return 1;
  }

  int new_fd = open("resultado.txt", O_WRONLY | O_CREAT, 0644);
  if (new_fd == -1) {
      perror("Error creating file");
      return 1;
    }


  int n_productores = atoi(argv[1]), n_consumidores = atoi(argv[2]), tam_buffer = atoi(argv[3]);
  //inicializamos los hilos
  pthread_t productores[n_productores], consumidores[n_consumidores];


  sem_init(&sem_full, 0, 0);
  sem_init(&sem_empty, 0, tam_buffer);  // Suponiendo un tamaño de cola de 10
  pthread_mutex_init(&mutex, NULL);

  sharedQueue = queue_init(tam_buffer);

  // creamos todos los hilos productores
  for (int i = 0; i < n_productores; i++){
      if (pthread_create(&productores[i], NULL, producir(fd), NULL) != 0) {
          perror("Error al crear el hilo");
          return 1;
      }
  }

  //creamos los hilos consumidores
  for (int i = 0; i < n_productores; i++){
      if (pthread_create(&consumidores[i], NULL, consumir, NULL) != 0) {
          perror("Error al crear el hilo");
          return 1;
      }
  }

  // Esperamos a que todos los hilos productores terminen
  for (int i = 0; i < n_productores; i++) {
      pthread_join(productores[i], NULL);
  }
  // Esperamos a que todos los hilos consumidores terminen
  for (int i = 0; i < n_consumidores; i++) {
      pthread_join(consumidores[i], NULL);
  }

  //Limpiamos
  queue_destroy(sharedQueue);
  sem_destroy(&sem_full);
  sem_destroy(&sem_empty);
  pthread_mutex_destroy(&mutex);





  // Output
  printf("Total: %d euros\n", profits);
  printf("Stock:\n");
  printf("  Product 1: %d\n", product_stock[0]);
  printf("  Product 2: %d\n", product_stock[1]);
  printf("  Product 3: %d\n", product_stock[2]);
  printf("  Product 4: %d\n", product_stock[3]);
  printf("  Product 5: %d\n", product_stock[4]);

  return 0;
}
