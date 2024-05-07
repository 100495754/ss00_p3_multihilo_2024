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
#include <math.h>


queue* sharedQueue;
queue* elemsQueue;
sem_t sem_prod, sem_cons;
pthread_mutex_t mutex;
pthread_cond_t no_lleno;
pthread_cond_t no_vacio;
int total = 0, operaciones = 0, retiradas = 0;//variable global para los profits (evita que los hilos den diferentes profits)
int product_stock_consumir [5] = {0};


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
    return max; // Retorna el numero de operaciones maximas
}


/* Esta funcion almacena los datos del fichero entrante en un array de struct element que sigue la distribucion de cola*/
void almacenar(int fd){
    char units[6], buff[1];
    ssize_t nread;
    struct element e;
    int salto = 0;

    // Inicia un bucle infinito que se rompe solo si 'read' falla
    while((nread = read(fd, buff, 1)) > 0) {
        buff[1] = '\0';//añadimos caracter nulo para evitar problemas
        e.product_id = atoi(buff);//Numero de ID

        read(fd, buff, 1);//BLANCO

        read(fd, buff, 1);//Primera letra
        if (buff[0] == 'P') {
            e.op = 1; // PURCHASE
            lseek(fd, 7, SEEK_CUR); // Saltar "URCHASE"
        } else if (buff[0] == 'S') {
            e.op = 2; // SALE
            lseek(fd, 3, SEEK_CUR); // Saltar "ALE"
        }

        read(fd, buff, 1);//BLANCO

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
                units[i] = atoi(buff);//Añade cada digito a un array
                i++;
            }
        }
        int unidades = 0;
        //BUCLE QUE CALCULA LAS UNIDADES
        for (int j = 0; j < i; j++) {
            unidades = unidades * 10 + units[j];
        }
        e.units = unidades;

        queue_put(elemsQueue, &e);
    }
}


/* Esta funcion coge datos de la cola que guarda los elementos del fichero y almacena de uno en uno en el buffer compartido*/
void *producir(){
    sem_wait(&sem_prod);
    while(operaciones < elemsQueue->capacity){
        pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
        //seccion critica
        while (sharedQueue->capacity == sharedQueue->count){//BUFFER LLENO
            pthread_cond_wait(&no_lleno, &mutex);
        }
        if (operaciones<elemsQueue->capacity){
            struct element *newElem = queue_get(elemsQueue);//coge de la cola auxiliar
            queue_put(sharedQueue, newElem);// introduce en el buffer compartido
            operaciones++;
            pthread_cond_signal(&no_vacio);// esto manda la señal de que el buffer ya no esta vacio
            pthread_mutex_unlock(&mutex);//salida de seccion critica
        }
        else{
            pthread_mutex_unlock(&mutex);//salida de seccion critica
            return NULL;//ya no hay mas que producir, por tanto termina el hilo
        }
    }
    sem_post(&sem_prod);
    return NULL;
}

void *consumir(){
    sem_wait(&sem_cons);
    while(retiradas < elemsQueue->capacity){
        pthread_mutex_lock(&mutex);//bloqueo por entrar en seccion critica
        //seccion critica
        while (sharedQueue->count == 0){//BUFFER VACIO
            pthread_cond_wait(&no_vacio, &mutex);
        }
        struct element *getElement = queue_get(sharedQueue);
        retiradas++;
        if (getElement == NULL) {
            printf( "Failed to get new element.\n");
            pthread_mutex_unlock(&mutex);
            return NULL;
        }

        //operaciones
        if (( getElement->product_id) == 1){
            if(getElement->op == 1){//PURCHASE
                int coste = 0;
                coste = getElement->units * 2;
                product_stock_consumir[0] = product_stock_consumir[0] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){//SALE
                int beneficio = 0;
                beneficio = getElement->units * 3;
                product_stock_consumir[0] = product_stock_consumir[0] - getElement->units;
                total = total + beneficio;
            }
        }
        else if (( getElement->product_id) == 2){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 5;
                product_stock_consumir[1] = product_stock_consumir[1] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 10;
                product_stock_consumir[1] = product_stock_consumir[1] - getElement->units;
                total = total + beneficio;
            }
        }
        else if ((getElement->product_id) == 3){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 15;
                product_stock_consumir[2] = product_stock_consumir[2] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 20;
                product_stock_consumir[2] = product_stock_consumir[2] - getElement->units;
                total = total + beneficio;
            }
        }
        else if ((getElement->product_id) == 4){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 25;
                product_stock_consumir[3] = product_stock_consumir[3] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 40;
                product_stock_consumir[3] = product_stock_consumir[3] - getElement->units;
                total = total + beneficio;
            }
        }
        else if ((getElement->product_id) == 5){
            if(getElement->op == 1){
                int coste = 0;
                coste = getElement->units * 100;
                product_stock_consumir[4] = product_stock_consumir[4] + getElement->units;
                total = total - coste;
            }
            else if (getElement->op == 2){
                int beneficio = 0;
                beneficio = getElement->units * 125;
                product_stock_consumir[4] = product_stock_consumir[4] - getElement->units;
                total = total + beneficio;
            }
        }
        //salida de seccion critica
        pthread_cond_signal(&no_lleno);// esto manda la señal de que el buffer ya no esta vacio
        pthread_mutex_unlock(&mutex);
    }
    sem_post(&sem_cons);
    return NULL;
}


int main (int argc, const char * argv[])
{
    int profits = 0;
    int product_stock [5] = {0};

    //COMPROBAMOS QUE NO HAYA MAS ARGUMENTOS DE LOS NECESARIOS
    if (argv[5]){
        perror("Demasiados argumentos");
        return -1;
    }

    //ABRIMOS EL FICHERO ENTRANTE
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }
    int max;

    //LEEMOS LA PRIMERA LINEA
    max = read_line(fd);
    if (max  == 0) {
        perror("Error al leer la línea de max operations.\n");
    }

    //ALMACENAMOS SOLO LAS OPERACIONES INDICADAS
    elemsQueue = queue_init(max);
    almacenar(fd);

    //INICIALIZAMOS LAS VARIABLES DE LOS HILOS Y SUS VALORES
    int n_productores = atoi(argv[2]), n_consumidores = atoi(argv[3]), tam_buffer = atoi(argv[4]);
    //inicializamos los hilos
    pthread_t productores[n_productores], consumidores[n_consumidores];

    //INICIALIZAMOS SEMAFOROS, MUTEX, ETC.
    pthread_cond_init(&no_lleno, NULL);
    pthread_cond_init(&no_vacio, NULL);
    sem_init(&sem_prod, 0, 1);
    sem_init(&sem_cons, 0, 1);
    pthread_mutex_init(&mutex, NULL);
    //inicio de la cola compartida
    sharedQueue = queue_init(tam_buffer);


    // creamos todos los hilos productores
    for (int i = 0; i < n_productores; i++){
        if (pthread_create(&productores[i], NULL, producir, NULL) != 0) {
            printf("Error al crear el hilo");
            return 1;
        }
    }

    //creamos los hilos consumidores
    for (int i = 0; i < n_consumidores; i++){
        if (pthread_create(&consumidores[i], NULL, consumir, NULL) != 0) {
            printf("Error al crear el hilo");
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
    sem_destroy(&sem_prod);
    sem_destroy(&sem_cons);
    pthread_mutex_destroy(&mutex);

    //METEMOS LOS VALORES EN EL ARRAY DE SALIDA
    product_stock[0] = product_stock_consumir[0];
    product_stock[1] = product_stock_consumir[1];
    product_stock[2] = product_stock_consumir[2];
    product_stock[3] = product_stock_consumir[3];
    product_stock[4] = product_stock_consumir[4];

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