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
//sem_t sem_full, sem_empty;
pthread_mutex_t mutex;
pthread_cond_t no_lleno;
pthread_cond_t no_vacio;
int total = 0, operaciones = 0, retiradas = 0;//variable global para los profits (evita que los hilos den diferentes profits)


/* Esta funcion recoge la primera linea del fichero entrante para guardar el numero maximo de operaciones a realizar*/
int read_line(int fd) {
    int count = 0;
    char buff[1];
    int salto = 0, max = 0;
    char number[5] = {0};

    while (!salto) {
        read(fd, buff, 1);
        if (buff[0] == '\n') {
            salto = 1; // Rompe el bucle si se encuentra un salto de línea
        }
        else{
            number[count]= buff[0];
        }
        count++;
    }

    //read(fd, number, count);
    number[count] = '\0';
    max = atoi(number);
    // Terminar la cadena correctamente
    return max; // Retornar el número de caracteres leídos
}


/* Esta funcion almacena los datos del fichero entrante en un array de struct element que sigue la distribucion de cola*/
void almacenar(int fd){
    char units[4], buff[1];
    ssize_t nread;
    struct element e;
    int salto = 0, temporal = 0;

    // Inicia un bucle infinito que se rompe solo si 'read' falla
    while((nread = read(fd, buff, 1)) > 0) {
        if (temporal==0){e.product_id = 1;temporal++;}
        else{e.product_id = atoi(buff);}
        //e.product_id = atoi(buff);

        read(fd, buff, 1);//BLANCO
        read(fd, buff, 1);

        if (buff[0] == 'P') {
            e.op = 1; // PURCHASE
            lseek(fd, 7, SEEK_CUR); // Saltar "URCHASE"
        } else if (buff[0] == 'S') {
            e.op = 2; // SALE
            lseek(fd, 3, SEEK_CUR); // Saltar "ALE"
        }
        read(fd, buff, 1);
        salto = 0;
        int i = 0;
        while (!salto) {
            read(fd, buff, 1);
            if (buff[0] == '\r') {
                read(fd, buff, 1);
                if (buff[0] == '\n') {
                    salto = 1; // Rompe el bucle si se encuentra un salto de línea
                }
            }
            else if (buff[0] == '\n') {
                salto = 1; // Rompe el bucle si se encuentra un salto de línea
            }
            else{
                units[i] = atoi(buff);
                i++;
            }
        }
        int unidades = 0;
        if (i==3){
            unidades = units[0]*100 + units[1]*10 + units[2];
        }
        else if (i==2){
            unidades = units[0]*10 + units[1];
        }
        else if (i==1){
            unidades = units[0];
        }
        e.units = unidades;
        queue_put(elemsQueue, &e);

    }
}


/* Esta funcion coge datos de la cola que guarda los elementos del fichero y almacena de uno en uno en el buffer compartido*/
void *producir(void* arg){
    if (arg == NULL) {
        printf("Productor recibió argumento NULL.\n");
        return NULL;
    }
    int id = *(int*)arg;
    printf("Productor %d iniciado.\n", id);
    while(operaciones < elemsQueue->capacity){
        //sem_wait(&sem_empty);
        pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
        printf("Entro en seccion critica %d\n", id);
        //seccion critica
        while (sharedQueue->capacity == sharedQueue->count){
            printf("Soy %d esperando a tener hueco para escribir\n ",id);
            printf("\n");
            pthread_cond_wait(&no_lleno, &mutex);
            //printf("Soy %d dejo de esperar\n ",id);
        }
        if (operaciones<elemsQueue->capacity){
            struct element *newElem = queue_get(elemsQueue);
            if (newElem == NULL) {
                printf("Failed to get new element.\n");
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            queue_put(sharedQueue, newElem);
            operaciones++;
            pthread_cond_signal(&no_vacio);// esto manda la señal de que el buffer ya no esta vacio
            //salida de seccion critica
            printf("Voy a salir de critica soy: %d \n ",id);
            printf("Productor dice que quedan %d items por coger de elemsqueue \n", elemsQueue->count);
            pthread_mutex_unlock(&mutex);
            //sem_post(&sem_full);
            printf("Ya fuera de critica soy: %d \n ",id);
            printf("\n ");
        }
        else{
            printf("PRODUCTOR %d TERMINA DEBIDO A QUE NO HAY MAS OPERACIONES\n", id);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

    }
    printf("Productor %d terminado \n", id);
    return NULL;
}

void *consumir(int product_stock [5], void* arg){
    int id = *(int*)arg;
    printf("Consumidor %d iniciado.\n", id);
    while(retiradas < elemsQueue->capacity){
        //sem_wait(&sem_full);
        pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
        printf("Consumidor %d entra en critica.\n", id);
        //seccion critica
        while (sharedQueue->count == 0){
            printf("Soy el consumidor %d esperando a tener hueco para escribir\n ",id);
            pthread_cond_wait(&no_vacio, &mutex);
        }
        struct element *getElement = queue_get(sharedQueue);
        retiradas++;
        if (getElement == NULL) {
            printf( "Failed to get new element.\n");
            printf("Consumidor %d terminado \n", id);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        pthread_cond_signal(&no_lleno);// esto manda la señal de que el buffer ya no esta vacio

        //operaciones
        if (( getElement->product_id) == 1){
            if(getElement->op == 1){//PURCHASE
                int coste = 0;
                coste = getElement->units * 2;
                product_stock[0] = product_stock[0] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){//SALE
                int beneficio = 0;
                beneficio = getElement->units * 3;
                product_stock[0] = product_stock[0] - getElement->units;
                total = total + beneficio;
            }
        }
        else if (( getElement->product_id) == 2){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 3;
                product_stock[1] = product_stock[1] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 5;
                product_stock[1] = product_stock[1] - getElement->units;
                total = total + beneficio;
            }
        }
        else if ((getElement->product_id) == 3){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 15;
                product_stock[2] = product_stock[2] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 20;
                product_stock[2] = product_stock[2] - getElement->units;
                total = total + beneficio;
            }
        }
        else if ((getElement->product_id) == 4){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 25;
                product_stock[3] = product_stock[3] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 40;
                product_stock[3] = product_stock[3] - getElement->units;
                total = total + beneficio;
            }
        }
        else if ((getElement->product_id) == 5){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 100;
                product_stock[4] = product_stock[4] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 125;
                product_stock[4] = product_stock[4] - getElement->units;
                total = total + beneficio;
            }
        }

        //salida de seccion critica
        printf("Consumidor dice que quedan %d items por coger y retiradas totales: %d\n", sharedQueue->count, retiradas);
        printf("Consumidor %d saliendo de seccion critica.\n", id);
        pthread_mutex_unlock(&mutex);
        printf("Consumidor %d fuera de critica \n", id);
        printf("\n ");
        //sem_post(&sem_empty);
    }
    printf("Consumidor %d terminado \n", id);
    return NULL;
}


int main (int argc, const char * argv[])
{
    int profits = 0;
    int product_stock [5] = {0};

    //ABRIMOS EL FICHERO ENTRANTE
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }
    //CREAMOS EL FICHERO RESULTANTE
    int new_fd = open("resultado.txt", O_WRONLY | O_CREAT, 0644);
    if (new_fd == -1) {
        perror("Error creating file");
        return 1;
    }
    int max;

    //LEEMOS LA PRIMERA LINEA
    //ssize_t nread;
    //buffer por defecto de 64 bytes, no es necesario tanto
    max = read_line(fd);
    if (max  == 0) {
        perror("Error al leer la línea de max operations.\n");
    }

    //ALMACENAMOS SOLO LAS OPERACIONES INDICADAS
    elemsQueue = queue_init(max);
    printf("  capacity: %d\n", elemsQueue->capacity);
    almacenar(fd);
    printf("%d", elemsQueue->count);

    //INICIALIZAMOS LAS VARIABLES DE LOS HILOS Y SUS VALORES
    int n_productores = atoi(argv[2]), n_consumidores = atoi(argv[3]), tam_buffer = atoi(argv[4]);
    //inicializamos los hilos
    pthread_t productores[n_productores], consumidores[n_consumidores];

    //INICIALIZAMOS SEMAFOROS, MUTEX, ETC.
    pthread_cond_init(&no_lleno, NULL);
    pthread_cond_init(&no_vacio, NULL);
    //sem_init(&sem_empty, 0, tam_buffer);
    //sem_init(&sem_full, 0, 0);
    pthread_mutex_init(&mutex, NULL);
    //inicio de la cola compartida
    sharedQueue = queue_init(tam_buffer);


    //int reparto = max / n_productores;// repartimos las operaciones a los hilos productores

    // creamos todos los hilos productores
    for (int i = 0; i < n_productores; i++){
        int* id_p = malloc(sizeof(int));
        *id_p = i;
        printf("Creando hilo prod %d \n", i);
        if (pthread_create(&productores[i], NULL, producir, id_p) != 0) {
            printf("Error al crear el hilo");
            free(id_p); // Liberar memoria en caso de falla
            return 1;
        }
        free(id_p);
    }

    //creamos los hilos consumidores
    for (int i = 0; i < n_consumidores; i++){
        int* id = malloc(sizeof(int));
        *id = i;
        printf("Creando hilo consumidor %d \n", i);
        if (pthread_create(&consumidores[i], NULL, consumir(product_stock, id), NULL) != 0) {
            printf("Error al crear el hilo");
            free(id); // Liberar memoria en caso de falla
            return 1;
        }
    }

    // Esperamos a que todos los hilos productores terminen
    for (int i = 0; i < n_productores; i++) {
        printf("Productor %d volvio a casa por navidad\n", i);
        printf("\n");
        pthread_join(productores[i], NULL);
        printf("Productor %d ha terminado y se ha unido correctamente.\n", i);
        printf("\n");
    }
    // Esperamos a que todos los hilos consumidores terminen
    for (int i = 0; i < n_consumidores; i++) {
        printf("Consumidor %d volvio a casa por navidad\n", i);
        printf("\n");
        pthread_join(consumidores[i], NULL);
        printf("Consumidor %d ha terminado y se ha unido correctamente.\n", i);
        printf("\n");
    }

    //Limpiamos
    queue_destroy(sharedQueue);
    queue_destroy(elemsQueue);
    pthread_cond_destroy(&no_vacio);
    pthread_cond_destroy(&no_lleno);
    //sem_destroy(&sem_full);
    //sem_destroy(&sem_empty);
    pthread_mutex_destroy(&mutex);



    profits = total;
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