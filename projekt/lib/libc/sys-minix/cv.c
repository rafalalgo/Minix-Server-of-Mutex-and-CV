#include <cv.h>
#include <errno.h>
#include <lib.h>
#include <minix/rs.h>

// funkcja pobieraja identyfikator serwera cv
static int get_cv() {
    int cv;
    // serwer otrzymuje rozne idetyfikatory wiec potrzebujemy
    // znac ten aktualny aby byc w stanie wysylac wiadomosci do serwera
    minix_rs_lookup("cv", &cv);
    return cv;
}

int send_message(int type, int mutex_id, int cv_id) {
    int cv = get_cv();
    // pobieramy numer endpoint servera cv
    message m;
    // konstruujemy wiadomosc do wyslania
    // w zalaznosci od typu wiadomosci ustawiamy numer mutexa lub numer cv
    m.m1_i1 = mutex_id;
    m.m1_i2 = cv_id;
    // no i majac juz numer cv i skonstruowana wiadomosc wysylamy wiadomosc
    // uzywajac _syscall ktory zawiesza sie w oczekiwaniu na odpowiedz
    // bo w implementacji uzywa do wyslania sendreca
    return _syscall(cv, type, &m);
}

int cs_lock(int mutex_id) {
    // chcemy zarezerwowac mutex o konkretnym numerze 
    int result;
    // oczekujemy ze serwer zwroci 0 jak sie uda zarezerwowac mutexa
    // a nie to zawiesza nasz proces w oczekiwaniu na mutexa
    while (-1 == (result = send_message(CS_LOCK, mutex_id, 0)))
        if (EINTR == errno)
            continue;
        else
            return -1;
    return result;
}

int cs_unlock(int mutex_id) {
    // no to poprostu bedzie go odblokowywal jezeli go posiada
    // zawraca 0 jezeli proces posiadal mutex i udalo sie go oddac
    // a jezeli proces nie mial mutexu ktory chce zwolnic to zwraca -1
    // oraz ustawia errno na EPERM
    return send_message(CS_UNLOCK, mutex_id, 0);
}

int cs_wait(int cv_id, int mutex_id) {
    // zawieszamy biezacy proces w oczekiwaniu na zdarzenie cv_id
    // pod warunkiem ze proces ktory chce czekac na zdarzenie jest w posiadaniu mutexa o przekazanym numerze
    int result = send_message(CS_WAIT, mutex_id, cv_id);
    // jezeli proces wolajacy nie ma odpowiedniego mutexu to
    // funkcja zwraca -1 i ustawia errno na EINVAL
    // jezeli proces wolajacy posiada mutex no to wtedy serwer zwalnia ten mutex 
    // nastepnie zawiesza proces az ktos oglosi zdarzenie za pomoca cs_broadcast
    // no i gdy zdarzenie zostanie ogloszone to ten proces co sie zglaszal z mutexem staje w kolejce po to mutex
    // i jak go otrzyma to zwraca kulturalnie 0
    if (-1 == result)
        if (EINTR == errno)
            return (-1 == cs_lock(mutex_id)) ? -1 : 0;
        else
            return -1;
    return result;
}

int cs_broadcast(int cv_id) {
    // oglasza zdarzenie
    // procesy ktore oczekiwaly na to zdarzenie zostaja odblokowane
    // a kazdy z nich gdy otrzymu mutex z ktorym byl zablokowaany
    // zostaje wznowiony
    return send_message(CS_BROADCAST, 0, cv_id);
}