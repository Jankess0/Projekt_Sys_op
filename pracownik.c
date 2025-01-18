#include "struct.h"

int sem_id;
int shm_id;
SharedData *shm_ptr;
int blocked = 0;

typedef struct {
    int chair_array[MAX_CHAIRS];
    pthread_mutex_t mutex;
} ChairStruct;

ChairStruct chair;

void start_shm();
void start_sem();
void* gate(void *arg);
void* lower(void *arg);
void* upper(void *arg);

int main() {
    pthread_t gate_thread[NUM_GATES], lower_thread, upper_thread;
    int gate_id[NUM_GATES];

    start_sem();
    start_shm();

    pthread_mutex_init(&chair.mutex, NULL);
    
    for (int i = 0; i < MAX_CHAIRS; i++) {
            chair.chair_array[i] = 0;
    }
    
    for(int i = 0; i < NUM_GATES; i++) {
        gate_id[i] = i;
        pthread_create(&gate_thread[i], NULL, gate, &gate_id[i]);
    }
    pthread_create(&lower_thread, NULL, lower, NULL);
    pthread_create(&upper_thread, NULL, upper, NULL);
    
    for(int i = 0; i < NUM_GATES; i++) {
        pthread_join(gate_thread[i], NULL);
    }
    pthread_join(lower_thread, NULL);
    pthread_join(upper_thread, NULL);
    pthread_mutex_destroy(&chair.mutex);

    return 0;

}
void* lower(void *arg){ // dodac semafor dla narciarza inicjowac wartoscia 0 pozniejnjak zwiekszamy na 3
    int index_i = 0;
    //int index_j = 0;
    int people_on_chair;
    int sem_val;
    while(shm_ptr->is_running) {
       
        //sem_op(sem_id, SEM_RIDE_UP, 0);
        //printf("Pracownik_dol: Krzeselko %d pojawilo sie na peronie\n", index_i);
        
        sleep(SYNC_CHAIR);
        sem_val = semctl(sem_id, SEM_QUEUE, GETVAL);
        if (sem_val > 0) {
            // Jeśli zostały jakieś niewykorzystane miejsca, zabierz je
            sem_op(sem_id, SEM_QUEUE, -sem_val);
        }

       // printf("wartosc sem %d\n", sem_val);
        people_on_chair = 3 - sem_val;

        pthread_mutex_lock(&chair.mutex);
        chair.chair_array[index_i] = people_on_chair;
        printf("Pracwonik_dol: zaladowano %d narcairzy na krzeselko %d\n", people_on_chair, index_i);
        pthread_mutex_unlock(&chair.mutex);

    
        sem_op(sem_id, SEM_QUEUE, 3);
        usleep(100000);

        index_i = (index_i + 1) % MAX_CHAIRS;
        
        

    }

    return NULL;

}

void* upper(void *arg) {
    int index = 10;
    int people_on_chair;
    int sem_val;

    while(shm_ptr->is_running) {
        //printf("Pracwonik_gora: Krzeselko %d dotarlo na gorna stacje\n", index);
        //sem_op(sem_id, SEM_RIDE_UP, 0);
        sleep(SYNC_CHAIR);

        pthread_mutex_lock(&chair.mutex);
        if (chair.chair_array[index] > 0) {
            people_on_chair = chair.chair_array[index];
            chair.chair_array[index] = 0;
            printf("Pracownik_gora: rozladowano krzeselko %d\n", index);
           // sem_val = semctl(sem_id, SEM_QUEUE, GETVAL);
            //printf("wartosc sem %d\n", sem_val);
            sem_op(sem_id, SEM_RIDE_UP, people_on_chair);

        }
        pthread_mutex_unlock(&chair.mutex);
         usleep(100000);  // 0.1s
        
        // Zresetuj semafor do 0 dla następnego krzesełka
        sem_val = semctl(sem_id, SEM_RIDE_UP, GETVAL);
        if(sem_val > 0) {
            sem_op(sem_id, SEM_RIDE_UP, -sem_val);
        }
        index = (index + 1) % MAX_CHAIRS;
       // sem_val = semctl(sem_id, SEM_QUEUE, GETVAL);
       // printf("wartosc sem %d\n", sem_val);

    }
    return NULL;
}

void* gate(void *arg) {
    int gate_num = *(int*)arg;

    while(shm_ptr->is_running) {
        if(shm_ptr->people_on_platform >= MAX_PLATFORM && !blocked) {
            // Jeśli przekroczono limit i bramka nie była jeszcze zablokowana
            printf("Osiagnieto maksymalną liczbę osób (%d). Blokuje bramke %d.\n", MAX_PLATFORM, gate_num);
            sem_op(sem_id, SEM_GATE1, -1);
            blocked = 1;
        } 
        else if(shm_ptr->people_on_platform < MAX_PLATFORM && blocked) {
            // Jeśli liczba osób spadła poniżej limitu i bramka była zablokowana
            printf("Liczba osob spadla ponizej maksimum (%d). Odblokowuje bramke %d.\n", 
                   shm_ptr->people_on_platform, gate_num);
            sem_op(sem_id, SEM_GATE1, 1);
            blocked = 0;
        }
        
        // Wyświetl aktualny stan
       // printf("Aktualna liczba osób na peronie: %d/%d  bramka: %d\n", 
         //      shm_ptr->people_on_platform, MAX_PLATFORM, gate_num);
        
        sleep(1);
        
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

void start_sem() {
    sem_id = semget(SEM_KEY, TOTAL_SEMS, 0666);
    if (sem_id == -1) {
        perror("Nie można uzyskać dostępu do semaforów");
        exit(EXIT_FAILURE);
    }
}