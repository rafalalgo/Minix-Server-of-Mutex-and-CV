#ifndef _INCLUDES_H
#define _INCLUDES_H

#define _POSIX_SOURCE       1
#define _SYSTEM             1
#define _MINIX              1

#include <stdlib.h>
#include <errno.h>
#include <minix/sysutil.h>
#include <minix/syslib.h>

/*===========================================================================*
 *                            PROGRAMMING UTILS                              *
 *===========================================================================*/

                                                                    // makro imituje wysokopoziomowa funkcje foreach z (np) C#
#define for_each(item, q) \
for (queue_item_t* item = q->front; \
item != q->end; item = item->next)

typedef enum { false, true } bool;                                  // w C nie ma typu bool, wiec tworzymy

/*===========================================================================*
 *                                 CONSTANTS                                 *
 *===========================================================================*/


#define MUTEXES                   1024                              // te liczby sa stale i dzieki nim alokuje sie stala ilosc
#define CONDITION_VARIABLES       512                               // pamieci na struktury dla mutexow i condition variables

#define CS_LOCK                   1
#define CS_UNLOCK                 2
#define CS_WAIT                   3
#define CS_BROADCAST              4
#define CS_EINTR                  5
#define CS_EXIT                   6

/*===========================================================================*
 *                               MAIN FUNCTIONS                              *
 *===========================================================================*/

// main
void initialize_mutexes_cv();                                       // inicjalizuje tablice mutexow i condition variables
void handle_message(message m);                                     // obsluguje wiadomosc

// SEF functions from IPC server
static void sef_local_startup(void);                                // sef startup znajduje sie w wielu serwerach i driverach
static int sef_cb_init_fresh(int type, sef_init_info_t *info);      // inicjalizacja zawsze sie udaje (OK), zeby odpalil
static void sef_cb_signal_handler(int signo);                       // ustawiamy pustego handlera sygnalow

// sending messages
void send_message(int type, int to);                                // wysyla wiadomosc okreslonego typu do okreslonego procesu

// lock and unlock mutexes
int get_free_mutex();                                               // zwraca indeks pierwszego wolnego miejsca na mutex
int find_used_mutex(int mutex_id);                                  // zwraca indeks uzywanego mutexu o wskazanym id
int cs_lock(int mutex_id);                                          // rezerwuje mutex o numerze mutex_id
int cs_unlock(int mutex_id);                                        // zwlania mutex o wkazanym identyfikatorze

// wait and broadcast
int get_free_cv();                                                  // zwraca indeks pierwszej wolnej condition variable
int find_used_cv(int cv_id);                                        // zwraca indeks uzywanej cv o wskazanym id
int cs_wait(int cv_id, int mutex_id);                               // zawiesza biezacy proces w oczekiwaniu na zdarzenie z cv_id
int cs_broadcast(int cv_id);                                        // oglasza zdarzenie identyfikowane przez cv_id

// handling eintr
bool eintr_delete_from_queues(int process);                         // usuwa proces z kolejki po mutex, jezeli gdzies czeka
void delete_process_from_cv(int i, int process_i);                  // usuwa wskazany proces z cv, zaklada, ze sie tam znajduje
void handle_eintr(int process);                                     // wysyla do procesu EINTR (interrupted by signal)

// removing processes
void unlock_all_mutexes(int process);                               // odblokowuje wszystkie mutexy zablokowane przez proces
void delete_from_queues(int process);                               // usuwa proces z kolejek, jezeli gdzies czekal na mutex
void delete_process(int process);                                   // usuwa proces po tym jak sie zakonczyl (exit)

#endif
