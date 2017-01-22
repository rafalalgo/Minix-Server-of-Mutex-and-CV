#include <cv.h>
#include <errno.h>
#include <lib.h>
#include <minix/rs.h>

static int get_cv() {                                               // musimy wiedziec dokad robic syscalle
  int cv;
  minix_rs_lookup("cv", &cv);                                       // jesli proces uzywa sef inicjalizacji, to mozna z tego korzystac
  return cv;                                                        // zwracamy znaleziony numer procesu
}

int send_message(int type, int mutex_id, int cv_id) {               // wysle wiadomosc do serwera cv i zwroci co odpowiedzial
  int cv = get_cv();                                                // znajduje proces serwera cv
  message m;                                                        // wiadomosc dla serwera cv
  m.m1_i1 = mutex_id;                                               // serwer cv na podstawie kodu type odczyta ta wartosc jao mutex_id
  m.m1_i2 = cv_id;                                                  // serwer cv na podstawie kodu type odczyta ta wartosc jao cv_id
  return _syscall(cv, type, &m);                                    // zwracamy to, co zwrocil syscall do serwera cv
}

/*===========================================================================*
 *                         LOCK AND UNLOCK MUTEXES                           *
 *===========================================================================*/

int cs_lock(int mutex_id) {                                         // probuje zarezerwowac mutex o numerze przekazanym w argumencie
  int result;
  while (-1 == (result = send_message(CS_LOCK, mutex_id, 0)))       // wysylamy syscall do serwera cv i tak dlugo jak syscall zwraca -1
    if (EINTR == errno)                                             // to w zaleznosci od tego co siedzi w errno
      continue;                                                     // albo blokowane jest EINTR i wtedy powtarzamy
    else
      return -1;                                                    // albo jest inny blad i konczymy
  return result;                                                    // zwracamy to, co zwrocil syscall do serwera cv
}

int cs_unlock(int mutex_id) {                                       // zwlania mutex o wkazanym identyfikatorze
  return send_message(CS_UNLOCK, mutex_id, 0);                      // zwracamy to, co zwrocil syscall do serwera cv
}

/*===========================================================================*
 *                           WAIT AND BROADCAST                              *
 *===========================================================================*/

int cs_wait(int cv_id, int mutex_id) {                              // zawiesza biezacy proces w oczekiwaniu na zdarzenie z cv_id
  int result = send_message(CS_WAIT, mutex_id, cv_id);              // wysylamy syscall do serwera cv
  if (-1 == result)                                                 // jezeli syscall zwrocil -1
    if (EINTR == errno)                                             // to sprawdzamy co siedzi w errno
      return (-1 == cs_lock(mutex_id)) ? -1 : 0;                    // albo otrzymal EINTR i probujemy zarezerwowac znowu mutex (sukces to 0)
    else                                                            // jesli udalo sie go odzyskac, to spurious wakeup
      return -1;                                                    // albo jest inny blad i konczymy
  return result;                                                    // zwracamy to, co zwrocil syscall do serwer cv
}

int cs_broadcast(int cv_id) {                                       // oglasza zdarzenie identyfikowane przez cv_id
  return send_message(CS_BROADCAST, 0, cv_id);                      // zwracamy to, co zwrocil syscall do serwera cv
}
