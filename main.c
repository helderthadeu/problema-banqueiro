#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#define NUMBER_OF_CUSTOMERS 3
#define NUMBER_OF_RESOURCES 3

int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

bool verifica_estado_seguro() {
    int work[NUMBER_OF_RESOURCES];
    bool finish[NUMBER_OF_CUSTOMERS];

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        work[i] = available[i];

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        finish[i] = false;

    int count = 0;
    while (count < NUMBER_OF_CUSTOMERS) {
        bool found = false;
        for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
            if (!finish[i]) {
                bool possible = true;
                for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
                    if (need[i][j] > work[j]) {
                        possible = false;
                        break;
                    }
                }

                if (possible) {
                    for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                        work[j] += allocation[i][j];
                    finish[i] = true;
                    found = true;
                    count++;
                }
            }
        }

        if (!found) return false;
    }

    return true;
}

int request_resources(int customer_num, int request[]) {
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > need[customer_num][i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (request[i] > available[i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] -= request[i];
        allocation[customer_num][i] += request[i];
        need[customer_num][i] -= request[i];
    }

    if (!verifica_estado_seguro()) {
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            available[i] += request[i];
            allocation[customer_num][i] -= request[i];
            need[customer_num][i] += request[i];
        }
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    pthread_mutex_unlock(&mutex);
    return 0;
}

int release_resources(int customer_num, int release[]) {
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        if (release[i] > allocation[customer_num][i]) {
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
        available[i] += release[i];
        allocation[customer_num][i] -= release[i];
        need[customer_num][i] += release[i];
    }

    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    return 0;
}

void *customer(void *arg) {
    int customer_id = *((int *)arg);
    while (true) {
        int request[NUMBER_OF_RESOURCES];

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++) {
            request[i] = (need[customer_id][i] == 0) ? 0 : rand() % (need[customer_id][i] + 1);
        }
        printf("\nCliente %d solicitando recursos: ", customer_id);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
            printf("%d ", request[i]);
        printf("\n");
        pthread_mutex_unlock(&mutex);

        if (request_resources(customer_id, request) == 0) {
            pthread_mutex_lock(&mutex);
            printf("Solicitacao ACEITA para o cliente %d. Alocacao atual: ", customer_id);
            for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
                printf("%d ", allocation[customer_id][j]);
            printf("\n");
            pthread_mutex_unlock(&mutex);
        } else {
            pthread_mutex_lock(&mutex);
            printf("Solicitacao NEGADA para o cliente %d.\n", customer_id);
            pthread_mutex_unlock(&mutex);
        }

        sleep(1);

        int release[NUMBER_OF_RESOURCES];
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
            release[i] = allocation[customer_id][i];
        printf("Cliente %d liberando recursos.\n", customer_id);
        pthread_mutex_unlock(&mutex);
        release_resources(customer_id, release);

        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != NUMBER_OF_RESOURCES + 1) {
        printf("Uso: %s <rec1> <rec2> <rec3>\n", argv[0]);
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    for (int i = 0; i < NUMBER_OF_RESOURCES; i++)
        available[i] = atoi(argv[i + 1]);

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++) {
            maximum[i][j] = rand() % (available[j] + 1);
            allocation[i][j] = 0;
            need[i][j] = maximum[i][j];
        }
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        printf("Cliente %d - Demanda maxima: ", i);
        for (int j = 0; j < NUMBER_OF_RESOURCES; j++)
            printf("%d ", maximum[i][j]);
        printf("\n");
    }

    pthread_t threads[NUMBER_OF_CUSTOMERS];
    int ids[NUMBER_OF_CUSTOMERS];

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, customer, &ids[i]) != 0) {
            perror("Erro ao criar thread");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        pthread_join(threads[i], NULL);

    return 0;
}