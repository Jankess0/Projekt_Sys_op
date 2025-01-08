#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "shared.h"

#define SHM_KEY 1234


int shm_id;
SharedData *shm_ptr;

void sell_ticket(int years, const char *type);
void raport();
void start_shmemory();

int main()
{
    srand(time(NULL));
    start_shmemory();

    if (shm_ptr -> num_tickets == 0) {
        shm_ptr -> num_tickets = 0;
    }

    printf("Kasjer uruchomiony\n");

    while (1) {
        int years = rand() % 80;
        //const char *type = (rand() % 2 == 0) ? "czasowy" : "dzienny";
        const char *type;
        int num = rand() % 4 + 1;
        switch(num){
            case 1:
                type = "czasowy2h";
                break;
            case 2:
                type = "czasowy4h";
                break;
            case 3:
                type = "czasowy6h";
                break;
            case 4:
                type = "dzienny";
                break; 
        }
        sell_ticket(years, type);

        // if (shm_ptr -> num_tickets % 10 == 0 && shm_ptr -> num_tickets > 0) {
        //     raport();
        // }
        //printf("koniec raportu\n");
        sleep(rand() % 3 + 1);
    }
    return 0;
}

void sell_ticket(int years, const char *type) {
    if (shm_ptr -> num_tickets >= MAX_TICKETS) {
        printf("osiagnieto limit sprzedarzy biletow");
        return;
    }

    Ticket *new_ticket = &shm_ptr -> tickets[shm_ptr -> num_tickets];
    new_ticket -> id = shm_ptr -> num_tickets +1;
    strncpy(new_ticket -> type, type, sizeof(new_ticket -> type) - 1);
    new_ticket -> type[sizeof(new_ticket -> type) -1] = '\0'; // zabezpiecznie przed przeplnieniem
    new_ticket ->discount = (years < 12 || years > 65) ? 1 : 0;
    new_ticket -> bought_time = time(NULL);

    shm_ptr -> num_tickets++;
    
    printf("Sprzedano karnet ID: %d, Typ: %s, Znizka: %s, Czas kupna: %s\n",
    new_ticket->id, new_ticket->type, new_ticket-> discount ? "tak" : "nie",
    ctime(&new_ticket->bought_time));
}
void raport() {
    printf("raport sprzedarzy\n");
    printf("Liczba karnetów w pamięci: %d\n", shm_ptr->num_tickets);
    
    for (int i = 0; i < shm_ptr -> num_tickets; i++) {
        
        printf("Karnet ID: %d; typ: %s; znizka: %s; czas kupna: %s;\n",
        shm_ptr -> tickets[i].id, shm_ptr -> tickets[i].type,
        shm_ptr -> tickets[i].discount ? "tak" : "nie",
        ctime(&shm_ptr -> tickets[i].bought_time));
    }
}

void start_shmemory() {
    //tworzenie pamieci
    shm_id = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Nie mozna utworzyc pamieci wspoldzielonej");
        exit(EXIT_FAILURE);
    }
    //dolaczdenie do pamieci
    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie mozna dolaczyc do pamieci wspoldzielonej");
        exit(EXIT_FAILURE);
    }
}