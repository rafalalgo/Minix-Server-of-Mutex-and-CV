#ifndef _kolejka_H
#define _kolejka_H

#include "includes.h"

typedef struct element element_t;
typedef struct kolejka kolejka_t;

struct element {
    struct element *nastepny;
    int proces;
};

struct kolejka {
    element_t *start;
    element_t *koniec;
};

kolejka_t *utworzKolejke() {
    kolejka_t *q = malloc(sizeof(kolejka_t));
    q->start = malloc(sizeof(element_t));
    q->koniec = malloc(sizeof(element_t));
    q->start->nastepny = q->koniec;
    q->koniec->nastepny = NULL;
    return q;
}

void wstaw(kolejka_t *q, int proces) {
    element_t *last = q->start;
    while (last->nastepny != q->koniec) {
        last = last->nastepny;
    }
    element_t *nowy_element = malloc(sizeof(element_t));
    nowy_element->proces = proces;
    last->nastepny = nowy_element;
    nowy_element->nastepny = q->koniec;
}

int pierwszyKolejka(kolejka_t *q) {
    element_t *first = q->start->nastepny;
    int proces = first->proces;
    q->start->nastepny = first->nastepny;
    free(first);
    return proces;
}

int czyPusto(kolejka_t *q) {
    return q->start->nastepny == q->koniec;
}

void usunElement(kolejka_t *q, element_t *item) {
    FOR_EACH(i, q) {
        if (i->nastepny == item) {
            i->nastepny = item->nastepny;
            free(item);
            return;
        }
    }
}

#endif