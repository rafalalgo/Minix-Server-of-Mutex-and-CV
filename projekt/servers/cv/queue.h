#ifndef _QUEUE_H
#define _QUEUE_H

#include "includes.h"

typedef struct queue_item queue_item_t;                             // typ elementu kolejki
typedef struct queue queue_t;                                       // typ kolejki

struct queue_item {
  struct queue_item *next;                                          // kolejny element w kolejce
  int process;                                                      // proces, ktory stoi w kolejce po mutex
};

struct queue {
  queue_item_t *front;                                              // straznik, pierwszy element
  queue_item_t *end;                                                // straznik, ostatni element
};

queue_t* queue_create() {                                           // tworzy i zwraca nowa kolejke
  queue_t* q = malloc(sizeof(queue_t));                             // alokacja nowej kolejki
  q->front = malloc(sizeof(queue_item_t));                          // alokacja pierwszego...
  q->end = malloc(sizeof(queue_item_t));                            // ...i ostatniego elementu
  q->front->next = q->end;                                          // ustawienie wskaznikow
  q->end->next = NULL;                                              // ostatni element nie wskazuje juz na nic dalej
  return q;
}

void queue_push(queue_t* q, int process) {                          // dodaje nowy proces do kolejki
  queue_item_t* last = q->front;                                    // obiekt, ktory bedzie wskazywal na ostatni element
  while (last->next != q->end)                                      // idziemy wzdluz calej kolejki do samego konca
    last = last->next;
  
  queue_item_t* new_item = malloc(sizeof(queue_item_t));            // nowy element dowiazany na koncu
  new_item->process = process;                                      // bedzie przechowywal wskazany numer procesu
  last->next = new_item;                                            // przepiecie wskaznikow i doczepienie na koncu
  new_item->next = q->end;
}

int queue_pop(queue_t* q) {                                         // sciaga pierwszy element z kolejki i zwraca go
  queue_item_t* first = q->front->next;                             // znalezienie pierwszego elementu w kolejce
  int process = first->process;                                     // zapisanie procesu, ktorego dotyczy
  q->front->next = first->next;                                     // nastepny element bedzie teraz pierwszy
  free(first);                                                      // zwlania pamiec usunietego elementu
  return process;                                                   // zwraca proces, ktory teraz dostanie mutex
}

bool queue_is_empty(queue_t* q) {                                   // zwraca informacje czy kolejka jest pusta
  return q->front->next == q->end;                                  // kolejka jest pusta, gdy straznicy sa kolo siebie
}

void queue_remove_item(queue_t* q, queue_item_t* item) {            // usuwa z kolejki wskazany element
  for_each(i, q)                                                    // dla kazdego elementu z kolejki
    if (i->next == item) {                                          // jezeli znajdziesz szukany element
      i->next = item->next;                                         // przepnij wskazniki
      free(item);                                                   // i zwolnij pamiec usunietego elementu
      return;
    }
}

#endif