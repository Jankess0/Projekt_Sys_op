#ifndef SHARED_H
#define SHARED_H

#define MAX_PERSONS 120
#define MAX_TICKETS 100
#define SHM_KEY 1234
#define SEM_KEY 5678

typedef struct 
{
    int id;
    char type[10];
    int discount;
    time_t bought_time;

} Ticket;

typedef struct
{
    int num_people;
    int lift_status;
    Ticket tickets[MAX_TICKETS];
    int num_tickets;
} SharedData;

#endif
