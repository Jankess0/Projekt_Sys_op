#ifndef SHARED_H
#define SHARED_H

#define MAX_PERSONS 120
#define MAX_TICKETS 100
#define SHM_KEY 1235
#define SEM_KEY 5678
#define MAX_CHAIRS 40
#define MAX_QUEUE_SIZE 100

typedef struct {
    int ticket_id;     
    int num_children;  
} FifoEntry;

typedef struct {
    int front; 
    int rear;  
    FifoEntry queue[MAX_QUEUE_SIZE]; 
} FifoQueue;

typedef struct 
{
    int id;
    char type[10];
    int discount;
    time_t bought_time;
    int age;
    int num_uses;
    int is_in_use;

} Ticket;

typedef struct
{
    int num_people_lower;
    int num_people_upper;
    int num_chairs;
    int ready_lower;      
    int ready_upper;
    int lift_status;
    Ticket tickets[MAX_TICKETS];
    int num_tickets;
    FifoQueue fifo_queue; 
} SharedData;


#endif
