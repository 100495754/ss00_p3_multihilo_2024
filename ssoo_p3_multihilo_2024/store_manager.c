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
queue* elemsQueue;
sem_t sem_full, sem_empty;
pthread_mutex_t mutex;


int read_line(int fd, char *buffer, int buffer_size) {
    int count = 0;
    char ch;
    ssize_t read_bytes;

    while (count < buffer_size - 1) { // Dejamos espacio para el '\0' final
        read_bytes = read(fd, &ch, 1); // Leer un carácter a la vez

        if (read_bytes == 1) { // Si se leyó un carácter
            if (ch == '\n') { // Si es un salto de línea, terminamos la línea
                break;
            }
            buffer[count++] = ch; // Guardar el carácter en el buffer
        } else if (read_bytes == 0) { // No más datos (EOF)
            if (count == 0) { // Si no hemos leído nada, retornamos -1
                return -1;
            }
            break; // Si ya hemos leído algo, simplemente terminamos
        } else { // Error al leer
            perror("Error reading from file descriptor");
            return -1;
        }
    }
    buffer[count] = '\0'; // Terminar la cadena correctamente
    return count; // Retornar el número de caracteres leídos
}

void almacenar(int fd){
    char id[1], action[8], units[2], blanco;
    ssize_t nread;
    struct element e;
    int final = 0;

    // Inicia un bucle infinito que se rompe solo si 'read' falla
    while(!final) {
        // Leer product_id
        nread = read(fd, id, 1);
        if (nread != 1) {
            // No hay más datos para leer, fin del archivo
            final = 1;
        }
        //Solo sigue si no se encuentra final
        if (!final) {
        e.product_id = atoi(id);

        // Leer espacio blanco después de product_id
        if (read(fd, &blanco, 1) != 1) {
            perror("Failed to read space after product_id");
        }

        // Leer operación
        nread = read(fd, action, 8);
        if (nread != 1) {
            perror("Failed to read operation");
        }
        e.op = atoi(action);

        // Leer espacio blanco después de operación
        if (read(fd, &blanco, 1) != 1) {
            perror("Failed to read space after operation");
        }

        // Leer unidades
        nread = read(fd, units, 2);
        if (nread != 1) {
            perror("Failed to read units");
        }
        e.units = atoi(units);

        // añadimos al array la estrcutura e de la linea "contador"
        queue_put(elemsQueue, &e);
        }
    }
}

void *producir(){
    //sem_wait(&sem_empty);

    pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
    //seccion critica
    struct element *newElem;
    newElem = queue_get(elemsQueue);
    queue_put(sharedQueue, newElem);

    //salida de seccion critica
    pthread_mutex_unlock(&mutex);
    //sem_post(&sem_full);
    return NULL;
}

void *consumir(){
    //sem_wait(&sem_empty);

    pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
    //seccion critica
    struct element *getElement;
    getElement = queue_get(sharedQueue);
    //salida de seccion critica
    pthread_mutex_unlock(&mutex);

    //operaciones
    //añadir código de operaciones
    //sem_post(&sem_full);
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
    int max;
    ssize_t nread;
    char buffer[64];//buffer por defecto de 64 bytes, no es necesario tanto

    if (read_line(0, buffer, sizeof(buffer)) >= 0) {
        max = atoi(buffer); // Convertir la cadena leída a un entero
    } else {
        printf("Error al leer la línea de max operations.\n");
    }
    elemsQueue = queue_init(max);
    almacenar(fd);

  int n_productores = atoi(argv[1]), n_consumidores = atoi(argv[2]), tam_buffer = atoi(argv[3]);
  //inicializamos los hilos
  pthread_t productores[n_productores], consumidores[n_consumidores];


  sem_init(&sem_full, 0, 0);
  sem_init(&sem_empty, 0, tam_buffer);  // Suponiendo un tamaño de cola de 10
  pthread_mutex_init(&mutex, NULL);

  sharedQueue = queue_init(tam_buffer);

  // creamos todos los hilos productores
  for (int i = 0; i < n_productores; i++){
      if (pthread_create(&productores[i], NULL, producir(), NULL) != 0) {
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
