#include "struct.h"

int msg_id;
int next_ticket_id = 1;
int shm_id;
SharedData *shm_ptr;

void start_msg();
void start_shm();
void calculate_price(TicketRequest* req, TicketResponse* resp);

int main() {
    start_msg();
    start_shm();
    srand(time(NULL));
    while(shm_ptr->is_running) {
        TicketRequest req;
        TicketResponse resp;
        if(msgrcv(msg_id, &req, sizeof(TicketRequest) - sizeof(long), 1, 0) == -1) {
                if(errno == EIDRM) break; // Kolejka została usunięta
                continue;
            }

        resp.mtype = req.skier_pid;
        for (int i = 0; i < req.children_count + 1; i++) {
            resp.ticket_id[i] = next_ticket_id++;
            //printf("Przydzielono bilet ID: %d dla osoby %d\n", resp.ticket_id[i], i);  // dodajemy debug
        }

        time_t current_time = time(NULL);
        switch(req.ticket_type) {
            case TICKET_TYPE_TK1:
                resp.valid_until = current_time + 3600;    // 1 godzina
                break;
            case TICKET_TYPE_TK2:
                resp.valid_until = current_time + 7200;    // 2 godziny
                break;
            case TICKET_TYPE_TK3:
                resp.valid_until = current_time + 14400;   // 4 godziny
                break;
            case TICKET_TYPE_DAILY:
                resp.valid_until = current_time + 43200;   // 12 godzin
                break;
            default:
                resp.valid_until = current_time + 3600; 
                break;   // domyślnie 1 godzina

        }
        calculate_price(&req, &resp);
        msgsnd(msg_id, &resp, sizeof(TicketResponse) - sizeof(long), 0);

       // printf("Sprzedano bilet dla narciarza %d (wiek: %d, dzieci: %d, typ: %d, cena: %.2f)\n", 
         //   req.skier_pid, req.age, req.children_count, req.ticket_type, resp.price);
    }

    return 0;
}

void calculate_price(TicketRequest* req, TicketResponse* resp) {
    double base_price;
    switch(req->ticket_type) {
        case TICKET_TYPE_TK1:
            base_price = 50;
            break;
        case TICKET_TYPE_TK2:
            base_price = 80;
            break;
        case TICKET_TYPE_TK3:
            base_price = 100;
            break;
        case TICKET_TYPE_DAILY:
            base_price = 150;
            break;
        default:
            base_price = 50;
    }
    
    // Zniżka dla dzieci i seniorów
    if(req->age < ADULT_AGE_MIN || req->age >= SENIOR_AGE_MIN) {
        base_price *= 0.75;  // 25% zniżki
    }
    if(req->children_count) {
        double children_price = base_price * 0.75;
        base_price += children_price * req->children_count;
    }
    
    resp->price = base_price;
}


void start_msg() {
    msg_id = msgget(MSG_KEY, 0666);
    if (msg_id == -1) {
        perror("Nie udało się utworzyć kolejki komunikatów");
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