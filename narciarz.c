#include "struct.h"

int msg_id;
int sem_id;
int shm_id;
SharedData *shm_ptr;

void start_msg();
void start_sem();
void start_shm();
void *child_thread(void* arg);
void raport(int ticket_id, int usage_count);


int main() {
    srand(time(NULL) ^ getpid());
    start_msg();
    start_sem();
    start_shm();

      // Generuj losowy wiek i liczbę dzieci
    int age = rand() % MAX_AGE;
    int children_count = 0;
    pthread_t* children = NULL;
    int is_vip = (rand() % 100) < 5;  // 5% szansa na VIP-a

    // Jeśli to dorosły, może mieć dzieci
    if(age >= ADULT_AGE_MIN) {
        children_count = rand() % (MAX_CHILDREN + 1);
        if(children_count > 0) {
            children = malloc(children_count * sizeof(pthread_t));
            for(int i = 0; i < children_count; i++) {
                pthread_create(&children[i], NULL, child_thread, NULL);
            }
        }
    }

    TicketRequest req = {
        .mtype = 1,
        .age = age,
        .children_count = children_count,
        .skier_pid = getpid(),
        .ticket_type = rand() % 4 + 1,  // Losowy typ karnetu
        .is_vip = is_vip
    };
    TicketResponse resp;
    printf("Narcairz: wiek: %d, dzieci: %d, pid: %d typ: %d vip: %d\n",
        req.age, req.children_count, req.skier_pid, req.ticket_type, req.is_vip);
    msgsnd(msg_id, &req, sizeof(TicketRequest) - sizeof(long), 0);
    msgrcv(msg_id, &resp, sizeof(TicketResponse) - sizeof(long), getpid(), 0);
    usleep(100000);
    //printf("Narcairz: cena: %.2f, id_biletu: %d\n", resp.price, resp.ticket_id[0]);

    // for (int i = 1; i < children_count + 1; i++) {
    //     printf("dziecko %d: cena: %.2f, id_biletu: %d\n", 
    //        i, resp.price, resp.ticket_id[i]);
    // }
    int ticket_usage = 0;
    int total_people = 1 + children_count;
    while(1) {
        time_t current_time = time(NULL);  // Aktualizacja czasu w każdej iteracji
        if(current_time > resp.valid_until) break;
        if(!shm_ptr->is_running) break;

        int gate = rand() % NUM_GATES;
        printf("Narciarz %d (VIP: %d) próbuje przejść przez bramkę %d\n", getpid(), is_vip, gate);
        sem_op(sem_id, gate, -1);
        ticket_usage++;

        shm_ptr->people_on_platform += total_people;
        sem_op(sem_id, gate, 1);
        printf("Narciarz %d przeszedlem przez bramke liczba osob na platformie %d\n",getpid(), shm_ptr->people_on_platform);
        shm_ptr->people_on_platform -= total_people;
        
        sem_op(sem_id, SEM_QUEUE, -total_people);

        printf("Narcairz %d: Wsiadlem na krzeselko jade do gory\n", getpid());

        sem_op(sem_id, SEM_RIDE_UP, -total_people);

        int track = rand() % 3;
        int ride_time;
        switch(track) {
            case 0: ride_time = T1; break;
            case 1: ride_time = T2; break;
            case 2: ride_time = T3; break;
        }
        printf("Narciarz %d zjeżdża trasą %d\n", getpid(), track);
        sleep(ride_time);

    }
    if(children) {
        for(int i = 0; i < children_count; i++) {
            pthread_join(children[i], NULL);
        }
        free(children);
    }
    for(int i = 0; i < total_people; i++) {
        raport(resp.ticket_id[i], ticket_usage);
       // printf("Bilet ID: %d, Liczba użyć: %d\n", resp.ticket_id[i], ticket_usage);
    }
    return 0;
}

void *child_thread(void* arg) {
    return NULL;  // Dzieci są tylko reprezentowane przez licznik
}
void raport(int ticket_id, int usage_count) {
    FILE *file = fopen("ticket_usage.txt", "a");
    if (file == NULL) {
        perror("Nie można otworzyć pliku");
        return;
    }
    
    fprintf(file, "Bilet ID: %d, Liczba użyć: %d\n", ticket_id, usage_count);
    fclose(file);
}


void start_msg() {
    msg_id = msgget(MSG_KEY, 0666);
    if (msg_id == -1) {
        perror("Nie udało się utworzyć kolejki komunikatów");
        exit(EXIT_FAILURE);
    }
}
void start_sem() {
    sem_id = semget(SEM_KEY, TOTAL_SEMS, 0666);
    if (sem_id == -1) {
        perror("Nie można uzyskać dostępu do semaforów");
        exit(EXIT_FAILURE);
    }
}
void start_shm() {
    shm_id = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shm_id == -1) {
        perror("Nie można uzyskać dostępu do pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("Nie można dołączyć do pamięci współdzielonej");
        exit(EXIT_FAILURE);
    }
}