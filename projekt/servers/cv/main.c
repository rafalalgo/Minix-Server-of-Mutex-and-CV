#include "includes.h"
#include "queue.h"

/*===========================================================================*
 *                          STRUCTS AND FIELDS                               *
 *===========================================================================*/

struct mutex {
  bool used;                                                        // true, jesli mutex jest uzywany
  int id;                                                           // id mutexu, czasem w kodzie zapisywany jako mutex_id
  int owner;                                                        // proces, ktowy w danej chwili jest wlascicielem mutexu
  queue_t* q;                                                       // kolejka procesow, ktore czekaja na zwolnienie mutexu
};

struct condition_variable {
  bool used;                                                        // true, jesli istnieja procesy oczekujace na zdarzenie
  int id;                                                           // id condition variable, czasem w kodzie zapisywany jako cv_id
  int processes[512];                                               // tablica oczekujacych procesow
  int mutexes[512];                                                 // posiadane mutexy, odpowiadajace indeksom procesow
  int size;                                                         // ile obecnie znajduje sie zawieszonych procesow
};

struct mutex M[MUTEXES];                                            // tablice mutexow
struct condition_variable CV[CONDITION_VARIABLES];                  // tablica condition variables
int who_e;                                                          // zmienna przechowujaca biezacy proces, ktory wywolal funkcje

void create_fresh_cv(int index) {                                   // przebudowuje dany cv i inicjalizuje pola na nowo
  CV[index].used = false;                                           // domyslnie cv nie jest uzywany
  CV[index].size = 0;                                               // na poczatku nie ma zadnych procesow, wiec 0
}

/*===========================================================================*
 *                                  MAIN                                     *
 *===========================================================================*/

int main(int argc, char* argv[])
{
  initialize_mutexes_cv();                                          // inicjalizuje tablice mutexow i condition variables
  env_setargs(argc, argv);                                          // sef local startup, tak jak w ipc
  sef_local_startup();                                              // sef local startup, tak jak w ipc
  message m;                                                        // wiadomosc, do ktorej wczytamy poczte
  
  while (true)                                                      // glowna petla serwera condition variables
    if (sef_receive(ANY, &m) == OK)                                 // tez jest sprawdzane w ipc, gdzie korzysta sie z sef
      handle_message(m);                                            // obsluzona zostaje wiadomosc
    else
      printf("CV: sef_receive failed\n");

  return 0;                                                         // powinien nigdy nie wyjsc z petli while(true)
}

void initialize_mutexes_cv() {                                      // inicjalizuje tablice mutexow i condition variables
  for (int i = 0; i < MUTEXES; ++i)                                 // dla wszystkich mutexow
    M[i].q = queue_create();                                        // tworzy pusta kolejke
  
  for (int i = 0; i < CONDITION_VARIABLES; ++i)                     // dla wszystkich cv
    create_fresh_cv(i);                                             // przebudowuje dany cv i inicjalizuje pola na nowo
}

void handle_message(message m) {                                    // obsluguje wiadomosc
  who_e = m.m_source;                                               // od kogo przyszla wiadomosc
  int type = m.m_type;                                              // jakiego typu jest wiadomosc i co mamy zrobic
  int mutex_id = m.m1_i1;                                           // domyslnie w tym miejscu znajduje sie id mutexu
  int cv_id = m.m1_i2;                                              // domyslnie w tym miejscu znajduje sie id zdarzenia
  int process = m.PM_PROC;                                          // proces, ktory dostalismy w wiadomosci od serwera pm
  int result;                                                       // co bedziemy wysylac w wiadomosci zwrotnej
  
  if (type < 1 || type > 6) {                                       // jezeli wiadomosc jest innego typu niz obslugiwane
    printf("CV: %d sent bad message type\n", who_e);                // to informujemy uzytkownika
    send_message(EINVAL, who_e);                                    // i proces
    return;                                                         // i nie idziemy dalej
  }
  
  switch (type) {                                                   // w zaleznosci od akcji, ktora mamy wykonac...
    case CS_LOCK :      result = cs_lock(mutex_id);       break;    // rezerwuj wskazany mutex dla procesu who_e
    case CS_UNLOCK :    result = cs_unlock(mutex_id);     break;    // odblokuj wskazany mutex, wywoluje who_e
    case CS_WAIT :      result = cs_wait(cv_id, mutex_id);break;    // zawies biezacy proces w oczekiwaniu
    case CS_BROADCAST : result = cs_broadcast(cv_id);     break;    // oglos zdarzenie identyfikowane przez cv_id
    case CS_EINTR :     handle_eintr(process);            break;    // powiadom proces o eintr
    case CS_EXIT:       delete_process(process);          break;    // proces sie zakonczyl, wiec wyczysc serwer z danych o nim
  }
  
  if (type < 5 && result != EDONTREPLY)                             // dla wiadomosci o eintr i zakonczeniu procesu nic nie odpowiadamy
    send_message(result, who_e);                                    // wysyla wiadomosc do procesu who_e
}

/*===========================================================================*
 *                       SEF FUNCTIONS FROM IPC SERVER                       *
 *===========================================================================*/

static void sef_local_startup() {                                   // sef startup podobnie jak w wielu serwerach i driverach
  sef_setcb_init_fresh(sef_cb_init_fresh);                          // register init callbacks
  sef_setcb_init_restart(sef_cb_init_fresh);                        // register init callbacks
                                                                    // ommitted live update support
  sef_setcb_signal_handler(sef_cb_signal_handler);                  // register signal callbacks
  sef_startup();                                                    // let sef perform startup
}

static int sef_cb_init_fresh(int type, sef_init_info_t *info) {
  return OK;                                                        // inicjalizacja zawsze sie udaje (OK), zeby odpalil
}

static void sef_cb_signal_handler(int signo) { }                    // ustawiamy pustego handlera sygnalow

/*===========================================================================*
 *                           SENDING MESSAGES                                *
 *===========================================================================*/

void send_message(int type, int to) {                               // wysyla wiadomosc okreslonego typu do wskazanego procesu
  message mess;                                                     // nowa wiadomosc
  mess.m_type = type;                                               // ustalenie typu
  int response = send(to, &mess);                                   // wyslanie wiadomosci i zapisanie odpowiedzi
  if (response != OK)                                               // jezeli nie zakonczylo sie sukcesem
    printf("CV: sorry, couldn't send to %d\n", who_e);              // informacja dla uzytkownika
}

/*===========================================================================*
 *                         LOCK AND UNLOCK MUTEXES                           *
 *===========================================================================*/
int get_free_mutex() {                                              // zwraca indeks pierwszego wolnego miejsca na mutex
  for (int i = 0; i < MUTEXES; ++i)                                 // przeszukanie calej tablicy
    if (M[i].used == false)                                         // znalezione zostalo wolne miejsce
      return i;                                                     // zwraca indeks
  return -1;                                                        // wszystkie mutexy sa zajete (zakladamy, ze nigdy)
}

int find_used_mutex(int mutex_id) {                                 // zwraca indeks uzywanego mutexu o wskazanym id
  for (int i = 0; i < MUTEXES; ++i)                                 // przeszukanie calej tablicy
    if (M[i].used && M[i].id == mutex_id)                           // znaleziony zostaje element o wskazanym id
      return i;                                                     // zwraca indeks
  return -1;                                                        // mutex nie jest w posiadaniu zadnego procesu
}

int cs_lock(int mutex_id) {                                         // rezerwuje mutex o numerze mutex_id
  int i = find_used_mutex(mutex_id);                                // probuje znalezc mutex o wskazanym id, ktory jest juz uzywany
  
  if (i == -1) {                                                    // mutex nie jest w posiadaniu zadnego procesu
    i = get_free_mutex();                                           // znajduje pierwsze wolne miejsce na mutex
    M[i].id = mutex_id;
    M[i].owner = who_e;                                             // przydzielenie mutexu procesowi, ktory wywolal funkcje
    M[i].used = true;                                               // mutex jest od teraz uzywany
    return OK;                                                      // zwraca 0 (sukces)
  }
  else {                                                            // jakis proces jest juz w posiadaniu mutexu
    if (M[i].owner == who_e)                                        // wlasciciel chce mutex ponownie (nieprawidlowo!)
      return EPERM;                                                 // zwrocenie bledu
    
    queue_push(M[i].q, who_e);                                      // proces bedzie czekal na kolejce az bedzie mogl dostac mutex
    return EDONTREPLY;                                              // proces nie otrzyma mutexu, wiec jeszcze mu nie odpowiadamy
  }
}

int cs_unlock(int mutex_id) {                                       // zwlania mutex o wkazanym identyfikatorze
  int i = find_used_mutex(mutex_id);                                // probuje znalezc mutex o wskazanym id, ktory jest juz uzywany
  
  if (i == -1)                                                      // mutex nie jest w posiadaniu zadnego procesu
    return EPERM;                                                   // czyli bardzo zle
  
  if (M[i].owner != who_e)                                          // proces wolajacy nie jest wlascicielem tego mutexu
    return EPERM;                                                   // czyli mamy jakiegos klamczuszka ;)
  
  if (queue_is_empty(M[i].q))                                       // zaden proces nie czeka w kolejce
    M[i].used = false;                                              // wiec teraz mutex juz jest wolny
  else {                                                            // jakis proces czeka w kolejce
    int new_owner = queue_pop(M[i].q);                              // nowym wlascicielem mutexu bedzie pierwszy w kolejce
    M[i].owner = new_owner;                                         // zmienia sie wlasciciel mutexu
    send_message(0, new_owner);                                     // proces otrzymuje mutex, wiec dostaje sygnal 0
  }
  
  return OK;                                                        // wolajacy proces byl w posiadaniu mutexu, obsluzone, czyli OK
}

/*===========================================================================*
 *                           WAIT AND BROADCAST                              *
 *===========================================================================*/

int get_free_cv() {                                                 // zwraca indeks pierwszej wolnej condition variable
  for (int i = 0; i < CONDITION_VARIABLES; ++i)                     // przeszukanie calej tablicy
    if (CV[i].used == false)                                        // znalezione zostalo wolne miejsce
      return i;                                                     // zwraca indeks
  return -1;                                                        // wszystkie condition variables sa zajete (zakladamy, ze nigdy)
}

int find_used_cv(int cv_id) {                                       // zwraca indeks uzywanej cv o wskazanym id
  for (int i = 0; i < CONDITION_VARIABLES; ++i)                     // przeszukanie calej tablicy
    if (CV[i].used && CV[i].id == cv_id)                            // znaleziony zostaje element o wskazanym id
      return i;                                                     // zwraca indeks
  return -1;                                                        // cv o wskazanym id nie jest uzywany
}

int cs_wait(int cv_id, int mutex_id) {                              // zawiesza biezacy proces w oczekiwaniu na zdarzenie z cv_id
  int i = find_used_mutex(mutex_id);                                // probuje znalezc mutex o wskazanym id, ktory jest juz uzywany
  
  if (i == -1)                                                      // mutex nie jest w posiadaniu zadnego procesu
    return -EINVAL;                                                 // czyli bardzo zle (- jest bo minix robi jakies zamiany znakow)
  
  if (M[i].owner != who_e)                                          // proces wolajacy nie jest wlascicielem tego mutexu
    return -EINVAL;                                                 // czyli rowniez bardzo zle (- jak wyzej)
  
  cs_unlock(mutex_id);                                              // proces wolajacy jest wlascicielem mutexu i go zwalnia
  
  i = find_used_cv(cv_id);                                          // szuka cv o wskazanym id
  
  if (i == -1) {                                                    // nie istnieja jeszcze procesy oczekujace na zdarzenie, wiec tworzymy
    i = get_free_cv();                                              // znajduje indeks pierwszej wolnej condition variable
    create_fresh_cv(i);                                             // przebudowuje dany cv i inicjalizuje pola na nowo
    CV[i].used = true;                                              // cv jest juz uzywany, wiec zaznaczamy
    CV[i].id = cv_id;                                               // jakiego id dotyczy
  }
  
  CV[i].processes[CV[i].size] = who_e;                              // zapisuje jaki proces bedzie oczekiwal
  CV[i].mutexes[CV[i].size] = mutex_id;                             // zapisuje posiadany mutex
  CV[i].size++;                                                     // zbior oczekujacych sie zwiekszyl
  return EDONTREPLY;                                                // proces dopiero po otrzymaniu mutexu z powrotem dostanie 0 (sukces)
}

int cs_broadcast (int cv_id) {                                      // oglasza zdarzenie identyfikowane przez cv_id
  int i = find_used_cv(cv_id);                                      // zwraca indeks uzywanej cv o wskazanym id
  if (i != -1) {                                                    // istnieja procesy oczekujace na zdarzenie, wiec oglaszamy
    int original = who_e;                                           // zapamietujemy autora wywolania, bo bedziemy go zmieniac za chwile
   
    for (int j = 0; j < CV[i].size; ++j) {                          // dla wszystkich procesow, ktore zawiesily sie w oczekiwaniu na zdarzenie
      who_e = CV[i].processes[j];                                   // ustawiamy tymczasowo proces jak gdyby to on wywolal funkcje cs_lock()
      int response = cs_lock(CV[i].mutexes[j]);                     // probujemy im oddac z powrotem mutexy
      if (response != EDONTREPLY)                                   // jezeli proces odzyskal mutex
        send_message(0, who_e);                                     // to zostaje poinformowany, aby wznowil dzialanie
    }
    
    who_e = original;                                               // przywracamy zapamietanego autora wywolania
    CV[i].used = false;
  }
  
  return OK;
}

/*===========================================================================*
 *                              HANDLING EINTR                               *
 *===========================================================================*/

bool eintr_delete_from_queues(int process) {                        // usuwa proces z kolejki po mutex, jezeli gdzies czeka
  for (int i = 0; i < MUTEXES; ++i)                                 // dla kazdego pojemnika na mutex
    if (queue_is_empty(M[i].q) == false)                            // jezeli kolejka po mutex jest niepusta
      for_each(item, M[i].q)                                        // przegladamy cala kolejke po mutex
        if (item->process == process) {                             // znalezlismy proces w jednej z kolejek
          send_message(EINTR, process);                             // wysylanie do procesu EINTR
          queue_remove_item(M[i].q, item);                          // usuwanie procesu z kolejki
          return true;                                              // usunieto z kolejki po mutex, wiec zwraca true
        }
  return false;                                                     // zwraca informacje, ze nie znalazl procesu w zadnej kolejce
}

void delete_process_from_cv(int i, int process_i) {                 // usuwa wskazany proces z cv, zaklada, ze sie tam znajduje
  if (CV[i].size == 1) {                                            // zostal jeden proces w cv, wiec jest tam nasz proces
    CV[i].used = false;                                             // wiec wylaczamy calosc
    return;
  }
                                                                    // procesow w cv jest wiecej niz jeden
  for (int j = process_i + 1; j < CV[i].size; ++j) {                // iterujemy po wszystkich procesach po usuwanym
    CV[i].processes[j-1] = CV[i].processes[j];                      // i przepisujemy je w przod (jak w sortowaniu przez wstawianie)
    CV[i].mutexes[j-1] = CV[i].mutexes[j];
  }
  
  CV[i].size--;                                                     // liczba oczekujacych procesow zmniejszona o jeden
}

void handle_eintr(int process) {                                    // wysyla do procesu EINTR (interrupted by signal)
  if (eintr_delete_from_queues(process))                            // jezeli wystapil w jakiejs kolejce po mutex, to nie
    return;                                                         // bedzie czekal na zdarzenie, wiec mozna zakonczyc
  
  for (int i = 0; i < CONDITION_VARIABLES; ++i)                     // dla wszystkich cv
    if (CV[i].used)                                                 // uzywanych cv
      for (int j = 0; j < CV[i].size; ++j)                          // przeszukujemy liste oczekujacych procesow
        if (CV[i].processes[j] == process) {                        // jezeli wsrod nich jest usuwany proces
          send_message(EINTR, process);                             // wysylanie do procesu EINTR
          delete_process_from_cv(i, j);                             // wiec zlecamy jego usuniecie
          break;                                                    // moze czekac tylko na jedno zdarzenie w cv
        }
 }

/*===========================================================================*
 *                             REMOVING PROCESSES                            *
 *===========================================================================*/

void unlock_all_mutexes(int process) {                              // odblokowuje wszystkie mutexy zablokowane przez proces
  for (int i = 0; i < MUTEXES; ++i)                                 // dla kazdego pojemnika na mutex
    if (M[i].used && (M[i].owner == process))                       // jezeli jest uzywany przez wskazany proces
      cs_unlock(M[i].id);                                           // odblokuj
}

void delete_from_queues(int process) {                              // usuwa proces z kolejek, jezeli gdzies czekal na mutex
  for (int i = 0; i < MUTEXES; ++i)                                 // dla kazdego pojemnika na mutex
    if (queue_is_empty(M[i].q) == false)                            // jezeli ktos czeka do niego w kolejce
      for_each (item, M[i].q)                                       // to dla kazdego takiego procesu w kolejce
        if (item->process == process) {                             // jezeli natrafimy na proces, ktory chcemy usunac
          queue_remove_item(M[i].q, item);                          // to go usuwamy z kolejki
          break;                                                    // i idziemy do nastepnego pojemnika na mutex
        }
}

void delete_process(int process) {                                  // usuwa proces po tym jak sie zakonczyl (exit)
  int original = who_e;                                             // zapamietujemy autora wywolania, bo bedziemy go zmieniac za chwile
  who_e = process;                                                  // ustawiamy tymczasowo proces jak gdyby to on wywolal funkcje cs_unlock()
  
  unlock_all_mutexes(process);                                      // odblokowuje wszystkie mutexy zablokowane przez proces
  delete_from_queues(process);                                      // usuwa proces z kolejek, jezeli gdzies czekal na mutex
  
  for (int i = 0; i < CONDITION_VARIABLES; ++i)                     // dla wszystkich cv
    if (CV[i].used)                                                 // uzywanych cv
      for (int j = 0; j < CV[i].size; ++j)                          // przeszukujemy liste oczekujacych procesow
        if (CV[i].processes[j] == process) {                        // jezeli wsrod nich jest usuwany proces
          delete_process_from_cv(i, j);                             // wiec zlecamy jego usuniecie
          break;                                                    // moze czekac tylko na jedno zdarzenie w cv
        }
  
  who_e = original;                                                 // przywracamy zapamietanego autora wywolania
}