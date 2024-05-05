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
int total = 0;//variable global para los profits (evita que los hilos den diferentes profits)


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
    int salto = 0;

    // Inicia un bucle infinito que se rompe solo si 'read' falla
    while((nread = read(fd, buff, 1)) > 0) {

        e.product_id = atoi(buff);
        read(fd, buff, 1);
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
void *producir(int operations){
    //sem_wait(&sem_empty);

    pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
    //seccion critica
    while (sharedQueue->capacity == sharedQueue->count){
        pthread_cond_wait(&no_lleno, &mutex);
    }
    for(int i = 0; i < operations; i++ ){
        struct element *newElem;
        newElem = queue_get(elemsQueue);
        queue_put(sharedQueue, newElem);
        pthread_cond_signal(&no_vacio);// esto manda la señal de que el buffer ya no esta vacio
    }

    //salida de seccion critica
    pthread_mutex_unlock(&mutex);
    //sem_post(&sem_full);
    return NULL;
}

void *consumir(int product_stock [5]){
    //sem_wait(&sem_empty);

    pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
    //seccion critica
    while (sharedQueue->capacity == 0){
        pthread_cond_wait(&no_vacio, &mutex);
    }
    struct element *getElement;
    getElement = queue_get(sharedQueue);
    pthread_cond_signal(&no_lleno);// esto manda la señal de que el buffer ya no esta vacio
    //salida de seccion critica
    pthread_mutex_unlock(&mutex);

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

    //sem_post(&sem_full);
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

    //INICIALIZAMOS LAS VARIABLES DE LOS HILOS Y SUS VALORES
    int n_productores = atoi(argv[2]), n_consumidores = atoi(argv[3]), tam_buffer = atoi(argv[4]);
    printf("  mAX: %d\n", n_productores);
    //inicializamos los hilos
    pthread_t productores[n_productores], consumidores[n_consumidores];

    //INICIALIZAMOS SEMAFOROS, MUTEX, ETC.
    pthread_cond_init(&no_lleno, NULL);
    pthread_cond_init(&no_vacio, NULL);
    //sem_init(&sem_full, 0, 0);
    //sem_init(&sem_empty, 0, tam_buffer);
    pthread_mutex_init(&mutex, NULL);
    printf("  mAX: %d\n", n_productores);
    //inicio de la cola compartida
    sharedQueue = queue_init(tam_buffer);
    printf("  mAX: %d\n", n_productores);


    int reparto = max / n_productores;// repartimos las operaciones a los hilos productores

    // creamos todos los hilos productores
    for (int i = 0; i < n_productores; i++){
        if (pthread_create(&productores[i], NULL, producir(reparto), NULL) != 0) {
            perror("Error al crear el hilo");
            return 1;
        }
    }

    //creamos los hilos consumidores
    for (int i = 0; i < n_productores; i++){
        if (pthread_create(&consumidores[i], NULL, consumir(product_stock), NULL) != 0) {
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