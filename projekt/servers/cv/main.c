#include "includes.h"
#include "queue.h"

struct mutex {
    // struktura opisujaca jeden konkretny mutex
    int uzywany; // czy uzywany
    int numer; // numer mutexu podany przez uzytkwownika do zarezerwowania
    int wlasciciel; // wlasciciel muteksu ustawiony jezeli mutex jest uzywany
    kolejka_t *kolejka; // kolejka oczekiwania na mutex
}
M[MAX_MUTEX];

struct conditionVariable {
    // struktuta opisujaca jedno konkretne zdarzenie
    int uzywany; // czy uzywane aktualnie
    int numer; // numer zdarzenia podany przez uzytkownika
    int procesy[512]; // procesy czekajace na dane zdarzenie
    int muteksy[512]; // numer muteksu z ktorym przyszedl proces oczekiwac na dane zdarzenie
    int size; // ile procesow oczekuje na dane zdarzenie 
}
CV[MAX_CV];

int aktualnyProces;

int main(int argc, char *argv[]) {
    int i = 0;
    // utworzenie pustej tablicy mutexow
    FOR(i, 0, MAX_MUTEX - 1) {
        M[i].uzywany = 0;
        M[i].kolejka = utworzKolejke();
    }

    // utworzenie pustej struktury do trzymania cv
    FOR(i, 0, MAX_CV - 1) {
        CV[i].uzywany = CV[i].size = 0;
    }

    env_setargs(argc, argv);
    sef_local_startup();
    message m;

    while (1) {
        if (sef_receive(ANY, &m) == OK) {
            // jak przyjdzie wiadomosc to obsluz
            obslugaWiadomosci(m);
        } else {
            // a jak cos nie tak to wypisz blad
            printf("CV: sef_receive failed\n");
        }
    }

    return 0;
}

void obslugaWiadomosci(message m) {
    // kto wyslal wiadomosc
    aktualnyProces = m.m_source;

    // taka jest umowa z funkcjami napisanymi po stronie uzytkownika
    // ze zapisuje typ w m_type, numer cv jezeli byl przekazany w m1_i2 oraz numer mutexu w m1_i1
    int typ = m.m_type;
    int numerMutexu = m.m1_i1;
    int numerCv = m.m1_i2;
    // m7_i1 w message tak de facto ustawiane makro w pliku com.h
    int proces = m.PM_PROC;
    int result;

    if (typ < 1 || typ > 6) {
        // jezeli ktos wyslal do cv wiadomosc z blednym typem zdarzenia to nie obslugujemy i wypisuejmy blad 
        printf("CV: %d sent bad message typ\n", aktualnyProces);
        // informuje proces ze podal mi zla wiadomosc wysylajac mu EINVAL
        wyslijWiadomosc(EINVAL, aktualnyProces);
        return;
    }

    switch (typ) {
        case CS_LOCK:
            result = cs_lock(numerMutexu);
            break;
        case CS_UNLOCK:
            result = cs_unlock(numerMutexu);
            break;
        case CS_WAIT:
            result = cs_wait(numerCv, numerMutexu);
            break;
        case CS_BROADCAST:
            result = cs_broadcast(numerCv);
            break;
        case CS_EINTR:
            wyslijEINTR(proces);
            break;
        case CS_EXIT:
            usunZakonczonyProces(proces);
            break;
    }

    if (typ < 5 && result != EDONTREPLY) {
        // jezeli wiadomosc jest taka ze powinnismy na nia wyslac odpowiedz uzytkownikowi to mu ja wysylamy
        wyslijWiadomosc(result, aktualnyProces);
    }
}

static void sef_local_startup() {
    sef_setcb_init_fresh(sef_cb_init_fresh);
    sef_setcb_init_restart(sef_cb_init_fresh);
    sef_setcb_signal_handler(sef_cb_signal_handler);
    sef_startup();
}

static int sef_cb_init_fresh(int typ, sef_init_info_t *info) {
    return OK;
}

static void sef_cb_signal_handler(int signo) {

}

void wyslijWiadomosc(int typ, int to) {
    message mess;
    // tworze wiadomosc i zapisuje  typ w m_type
    mess.m_type = typ;
    // wysylam wiadomosc komu trzeba
    int response = send(to, &mess);
    // jezeli nie ok to wypisuje komunikat stosowny
    if (response != OK) {
        printf("CV: sorry, couldn't send to %d\n", aktualnyProces);
    }
}

int indeksUzywanegoMutexa(int numerMutexu) {
    int i = 0;
    // szukamy w strukturze w ktore mamy mutexy czy jest mutex o numerze z zapytania
    // jezeli jest zwracamy jeo index a jak nie to -1
    FOR(i, 0, MAX_MUTEX - 1) {
        if (M[i].uzywany && M[i].numer == numerMutexu) {
            return i;
        }
    }
    return -1;
}

int indeksUzywanegoCv(int numerCv) {
    int i = 0;
    // szukamy w strukturzze do cv czy jest gdzies zmienna cv uzywana taka o jaka pytamy
    // jak jest zwroc index a jak nie ma zwroc -1
    FOR(i, 0, MAX_CV - 1) {
        if (CV[i].uzywany && CV[i].numer == numerCv) {
            return i;
        }
    }
    return -1;
}

int cs_lock(int numerMutexu) {
    int i = 0;
   // printf("%d chce mutex %d.\n", aktualnyProces, numerMutexu);
    // jakis koles chce dostac mutex
    // sprawdzam czy kto ma mutex o tym numerzze juz czy moze akurat nie
    if ((i = indeksUzywanegoMutexa(numerMutexu)) == -1) {
        // nikt nie ma mutexu o tym numerze
        // w mojej strukturzr mutexow szukam wolnej pozycji zeby sobie stworzyc mutex dla goscia
        FOR(i, 0, MAX_MUTEX - 1) {
            if (M[i].uzywany == 0) {
                break;
            }
        }
        // no i tworze mutex i zwracam OK ze sie powiodlo
        M[i].numer = numerMutexu;
        M[i].wlasciciel = aktualnyProces;
        M[i].uzywany = 1;
        return OK;
    } else {
        // mutex o ktory poproszono generalnie jest zajety
        // jezeli to wlasciciel poprosil o dany mutex
        // no to mowie kulturalnie ze EPERM nie wolno
        if (M[i].wlasciciel == aktualnyProces) {
            return EPERM;
        } else {
     //       printf("dorzucam do kolejki.\n", numerMutexu);
            // a jak poprosil ktos nowy to kulturalnie wstawiam go do kolejki
            // i oczekuje na swoja kolej zas proces zawieszamy do tego momentu bo nie dostal odpowiedzi
            wstaw(M[i].kolejka, aktualnyProces);
            return EDONTREPLY;
        }
    }
}

int cs_unlock(int numerMutexu) {
    int i = 0;
    if ((i = indeksUzywanegoMutexa(numerMutexu)) == -1 || M[i].wlasciciel != aktualnyProces) {
        // jezeli chcemy odblokowac mutex ktory nie byl uzywany albo probuje odblokowac nie wlasciciel
        // to wtedy zwracamy EPERM
        return EPERM;
    }

    if (czyPusto(M[i].kolejka) == 1) {
       // printf("%d sie zwolnil.\n", numerMutexu);
        // ktos zwolnil mutex, ale jednoczesnie nie ma nikogo w kolejce 
        // mozemy oznaczyc mutex jako nieuzywany
        M[i].uzywany = 0;
    } else {
        // jako nowego wlasciciela mutexa dajemy typka co byl pierwszy w kolejce i usuwamy go z kolejki
        // temu co dostal mutexa wysylamy 0 ze sie udalo i go dostal
        int new_wlasciciel = pierwszyKolejka(M[i].kolejka);
        M[i].wlasciciel = new_wlasciciel;
        wyslijWiadomosc(0, new_wlasciciel);
        //printf("%d ma nowego wlasciciela %d\n", numerMutexu, new_wlasciciel);
    }
    // wszystko sie powiodlo wiec zwracamy OK
    return OK;
}

int cs_wait(int numerCv, int numerMutexu) {
    int i = 0;
    if ((i = indeksUzywanegoMutexa(numerMutexu)) == -1 || M[i].wlasciciel != aktualnyProces) {
        // jezeli ten muteks z ktorym sie ktos zglosil nie jest uzywany albo wlasciciel jest inny to 
        // zwracamy kulturalnie blad
        return -EINVAL;
    }

    // mutex wiec zwalniamy
    cs_unlock(numerMutexu);

    if ((i = indeksUzywanegoCv(numerCv)) == -1) {
        // jezeli nikt nie uzywa tego cv no to znajduje 
        FOR(i, 0, MAX_CV - 1) {
            if (CV[i].uzywany == 0) {
                break;
            }
        }
        // znajduje pierwsze miejsce w strukturze gdzie moge go zapisac no i go tam tworze jako nowego cv

        CV[i].size = 0;
        CV[i].uzywany = 1;
        CV[i].numer = numerCv;
    }

    // mam sobie w strukturze cv pole odpowiedzialne za moje cv
    // no to tam dopisuje proces wraz z mutexem ktory mam mu oddac gdy sie skonczy zabawa
        
    CV[i].procesy[CV[i].size] = aktualnyProces;
    CV[i].muteksy[CV[i].size] = numerMutexu;
    CV[i].size++;
    //printf("%d z mutexem %d chce %d\n", aktualnyProces, numerMutexu, numerCv);
    // no i z racji ze proces sie zawiesza oczekujac na ogloszenie zdarzenia no to teraz 
    // nie zwracam nic w odpowiedzi to sie zawiesi na sendrecu
    return EDONTREPLY;
}


int cs_broadcast(int numerCv) {
    int i = 0;
    int j = 0;
    // sprawdzamy czy cv ktore chce ktos oglosic jest wgl powszechnie uzywane
    if ((i = indeksUzywanegoCv(numerCv)) != -1) {
        int original = aktualnyProces;
        //printf("        %d rozglasza %d\n", original, numerCv);
        // jezeli jest, to zapisuje aktualny proces ktory oglosic chce wydarzenie
        FOR(j, 0, CV[i].size - 1) {
            // dla wszystjkich procesow czekajacych na zdarzenie
            // zwracam im mutex z ktorym przyszli
            aktualnyProces = CV[i].procesy[j];
            int response = cs_lock(CV[i].muteksy[j]);

            // jezeli jest wymagana odpowiedz no to wysylamy procesowi czekajacemy na ogloszenie CV
            // odpowiedz 0 zeby mogl byc wznowiony
            if (response != EDONTREPLY) {
                wyslijWiadomosc(0, aktualnyProces);
            }
            CV[i].procesy[j] = 0;
            CV[i].muteksy[j] = 0;
        }
        aktualnyProces = original;
        // przywracam numer procesu i oznaczam CV jako nie uzywane
        CV[i].uzywany = 0;
        CV[i].size = 0;
        CV[i].numer = 0;
    }

    return OK;
}

void wyslijEINTR(int proces) {
    int i = 0;
    int j = 0;
    // wysylamy do procesu EINTR ze przerwany przez sygnal zostal
    // usuwa proces z kolejki jezeli gdzies czeka 
    // dla kazdefo mutexa jezeli kolejka po mutex nie jest pusta no to
    FOR(i, 0, MAX_MUTEX - 1) {
        if (czyPusto(M[i].kolejka) == 0) {
            // przegladamy ja cala i jak gdzies jest nasz proces
            // to wysylamy mu EINTR , i usuwamy z kolejki
            FOR_EACH(item, M[i].kolejka) {
                if (item->proces == proces) {
                    wyslijWiadomosc(EINTR, proces);
                    usunElement(M[i].kolejka, item);
                    break;
                }
            }
        }
    }

    // a jezeli nasz proces gdzies czeka w kolejce po CV a dostal sygnal 
    FOR(i, 0, MAX_CV - 1) {
        // to my sobie przegladamy tablice cv i jak ktores uzywane sprawdzamy
        // czy nasz proces gdzies nie czeka a jak czeka
        if (CV[i].uzywany == 1) {
            FOR(j, 0, CV[i].size - 1) {
                // wysylamy wiadomosc EINTR
                // i usuwamy z tabeli czekajacych na ten
                if (CV[i].procesy[j] == proces) {
                    wyslijWiadomosc(EINTR, proces);
                    usunZakonczonyProcesCV(i, j);
                    break;
                }
            }
        }
    }
}

void usunZakonczonyProcesCV(int i, int proces_i) {
    int j = 0;
    // dostaje numer indeksu pod ktorym jest jakis tam CV oaz indeks procesu ktory powinien usnac z listy
    if (CV[i].size == 1) {
        // jezeli byl to jedyny proces na liscie tego cv to wtedy cv jest zwalniane
        CV[i].uzywany = 0;
        return;
    }

    // no wiec jak jest wiecej i usunie to reszte przepisuje o jedna pozycje w tyl zeby blok byl spojny
    FOR(j, proces_i + 1, CV[i].size - 1) {
        CV[i].procesy[j - 1] = CV[i].procesy[j];
        CV[i].muteksy[j - 1] = CV[i].muteksy[j];
    }
    // i zmniejsza liczbe oczekujacych
    CV[i].size--;
}

void usunZakonczonyProces(int proces) {
    int i = 0;
    int j = 0;
    // chcemy oczyscic tabelki mutexow i cv z konkretnego procesu bo go juz nie ma 
    int original = aktualnyProces;
    // zapisujemy informacje o aktualnym procesie i wracamy do naszego do usuniecia
    aktualnyProces = proces;

    // w tablicy mutexow, jezeli ktorys mutex jest uzywany
    FOR(i, 0, MAX_MUTEX - 1) {
        // i ten proces jest jego wlascicielem
        // to odblokowywujemy mutexa
        if (M[i].uzywany && (M[i].wlasciciel == proces)) {
            cs_unlock(M[i].numer);
        }
    }

    // dla kazdego mutexa sprawdzamy czy moze przypadkiem proces nie czeka w kolejce po niego
    FOR(i, 0, MAX_MUTEX - 1) {
        // jezeli kolejka nie jest pusta no to wtedy
        if (czyPusto(M[i].kolejka) == 0) {
            // dla kazdego elementu w kolejce 
            FOR_EACH (item, M[i].kolejka) {
                // jezeli to nasz proces
                if (item->proces == proces) {
                    // to go usun i zabreakuj bo proces jest maks raz w kolejce
                    usunElement(M[i].kolejka, item);
                    break;
                }
            }
        }
    }

    // dla kazdego cv jezeli jest uzywany
    FOR(i, 0, MAX_CV - 1) {
        if (CV[i].uzywany == 1) {
            FOR(j, 0, CV[i].size - 1) {
                // przejrzyj jego liste procesow czekajacych na niego i jak gdzies czeka nasz no to wez i usun 
                if (CV[i].procesy[j] == proces) {
                    usunZakonczonyProcesCV(i, j);
                    break;
                }
            }
        }
    }

    // przywroc aktualnyproces
    aktualnyProces = original;
}