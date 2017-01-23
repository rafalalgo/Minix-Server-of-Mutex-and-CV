#ifndef _QUEUE_H
#define _QUEUE_H

#include "includes.h"

typedef struct queue_item queue_item_t; 
typedef struct queue queue_t;

struct queue_item {
    struct queue_item *next;
    int process;
};

struct queue {
    queue_item_t *front;
    queue_item_t *end;
};

queue_t *utworzKolejke() {
    queue_t *q = malloc(sizeof(queue_t));
    q->front = malloc(sizeof(queue_item_t));
    q->end = malloc(sizeof(queue_item_t));
    q->front->next = q->end;
    q->end->next = NULL;
    return q;
}

void wstaw(queue_t *q, int process) {
    queue_item_t *last = q->front;
    while (last->next != q->end) {
        last = last->next;
    }
    queue_item_t *new_item = malloc(sizeof(queue_item_t));            
    new_item->process = process;
    last->next = new_item;
    new_item->next = q->end;
}

int pierwszyKolejka(queue_t *q) {
    queue_item_t *first = q->front->next;
    int process = first->process;
    q->front->next = first->next;
    free(first);
    return process;
}

int czyPusto(queue_t *q) {
    return q->front->next == q->end;
}

void usunElement(queue_t *q, queue_item_t *item) {            
    FOR_EACH(i, q) {
        if (i->next == item) {
            i->next = item->next;
            free(item);
            return;
        }
    }
}

#endif