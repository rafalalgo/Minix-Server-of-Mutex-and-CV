#ifndef _INCLUDES_H
#define _INCLUDES_H

#define _POSIX_SOURCE       1
#define _SYSTEM             1
#define _MINIX              1

#include <stdlib.h>
#include <errno.h>
#include <minix/sysutil.h>
#include <minix/syslib.h>

#define FOR_EACH(item, q) for (element_t* item = q->start; item != q->koniec; item = item->nastepny)
#define FOR(i, a, b) for((i) = (a); (i) <= (b); (i)=(i)+1)
#define FOR_D(i, a, b) for((i) = (a); (i) >= (b); (i) = (i) - 1)	

#define MAX_MUTEX                1024
#define MAX_CV                   512

#define CS_LOCK                   1
#define CS_UNLOCK                 2
#define CS_WAIT                   3
#define CS_BROADCAST              4
#define CS_EINTR                  5
#define CS_EXIT                   6

static void sef_local_startup(void);
static int sef_cb_init_fresh(int type, sef_init_info_t *info);
static void sef_cb_signal_handler(int signo);

int cs_lock(int mutex_id);
int cs_unlock(int mutex_id);
int cs_wait(int cv_id, int mutex_id);
int cs_broadcast(int cv_id); 

void obslugaWiadomosci(message m);
void wyslijWiadomosc(int typ, int to);

int indeksUzywanegoMutexa(int numerMutexu); 
int indeksUzywanegoCv(int numerCv);

void wyslijEINTR(int proces);

void usunZakonczonyProcesCV(int i, int proces_i);
void usunZakonczonyProces(int proces);

#endif
