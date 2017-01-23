#include "includes.h"
#include "queue.h"

struct mutex {
    int uzywany;
    int numer;
    int wlasciciel;
    queue_t *kolejka;
} 
M[MAX_MUTEX];

struct conditionVariable {
    int uzywany;
    int numer;
    int procesy[512];
    int muteksy[512];
    int size
}
CV[MAX_CV];

int i, j;
int aktualnyProces;

int main(int argc, char *argv[]) {
    FOR(i, 0, MAX_MUTEX - 1) {
        M[i].kolejka = utworzKolejke();
    }

    FOR(i, 0, MAX_CV - 1) {
        CV[index].uzywany = CV[index].size = 0;
    }

    env_setargs(argc, argv);       
    sef_local_startup();           
    message m; 

    while (1) {
        if (sef_receive(ANY, &m) == OK) {
            obslugaWiadomosci(m); 
        } else {
            printf("CV: sef_receive failed\n");
        }
    }

    return 0; 
}

void obslugaWiadomosci(message m) {
    aktualnyProces = m.m_source;

    int typ = m.m_typ;
    int numerMutexu = m.m1_i1;
    int numerCv = m.m1_i2;
    int proces = m.PM_PROC;
    int result;

    if (typ < 1 || typ > 6) {
        printf("CV: %d sent bad message typ\n", aktualnyProces);
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
    mess.m_typ = typ;
    int response = send(to, &mess);

    if (response != OK) {
        printf("CV: sorry, couldn't send to %d\n", aktualnyProces);
    }
}

int indeksUzywanegoMutexa(int numerMutexu) {
    FOR(i, 0, MAX_MUTEX - 1) {
        if (M[i].uzywany && M[i].numer == numerMutexu) {
            return i;
        }
    }
    return -1;
}

int indeksUzywanegoCv(int numerCv) {
    FOR(i, 0, MAX_CV - 1) {
        if (CV[i].uzywany && CV[i].numer == numerCv) {
            return i;
        }
    }
    return -1;
}

int cs_lock(int numerMutexu) {
    if ((i = indeksUzywanegoMutexa(numerMutexu)) == -1) {
        FOR(i, 0, MAX_MUTEX - 1) {
            if (M[i].uzywany == 0) {
                break;
            }
        }

        M[i].numer = numerMutexu;
        M[i].wlasciciel = aktualnyProces;
        M[i].uzywany = 1;
        return OK;
    } else {
        if (M[i].wlasciciel == aktualnyProces) {
            return EPERM;
        }

        wstaw(M[i].kolejka, aktualnyProces);

        return EDONTREPLY;
    }
}

int cs_unlock(int numerMutexu) {
    if ((i = indeksUzywanegoMutexa(numerMutexu)) == -1 || M[i].wlasciciel != aktualnyProces) {
        return EPERM;
    }

    if (czyPusto(M[i].kolejka) == 1) {
        M[i].uzywany = 0;
    }
    else {
        int new_wlasciciel = pierwszyKolejka(M[i].kolejka);
        M[i].wlasciciel = new_wlasciciel;
        wyslijWiadomosc(0, new_wlasciciel);
    }

    return OK;
}

int cs_wait(int numerCv, int numerMutexu) {
    if ((i = indeksUzywanegoMutexa(numerMutexu)) == -1 || M[i].wlasciciel != aktualnyProces) {
        return -EINVAL;
    }

    cs_unlock(numerMutexu);

    if ((i = indeksUzywanegoCv(numerCv)) == -1) { 
        FOR(i, 0, MAX_CV - 1) {
            if (CV[i].uzywany == 0) {
                break;
            }
        }

        CV[i].size = 0;
        CV[i].uzywany = 1;
        CV[i].numer = numerCv;
    }

    CV[i].procesy[CV[i].size] = aktualnyProces;
    CV[i].muteksy[CV[i].size] = numerMutexu;
    CV[i].size++;

    return EDONTREPLY;
}


int cs_broadcast(int numerCv) {
    if ((i = indeksUzywanegoCv(numerCv)) != -1) {
        int original = aktualnyProces;
        FOR(j, 0, CV[i].size - 1) {
            aktualnyProces = CV[i].procesy[j];
            int response = cs_lock(aktualnyProces);

            if (response != EDONTREPLY) {
                wyslijWiadomosc(0, aktualnyProces);
            }
        }

        aktualnyProces = original;
        CV[i].uzywany = 0;
    }

    return OK;
}

int usunProcesKolejkiMutexowEINTR(int proces) {
    FOR(i, 0, MAX_MUTEX - 1) {
        if (czyPusto(M[i].kolejka) == 0) {
            FOR_EACH(item, M[i].kolejka) {     
                if (item->proces == proces) {
                    wyslijWiadomosc(EINTR, proces);
                    usunElement(M[i].kolejka, item);
                    return 1;
                }
            }
        }
    }

    return 0;
}

void usunZakonczonyProcesCV(int i, int proces_i) {
    if (CV[i].size == 1) {
        CV[i].uzywany = 0;
        return;
    }
    FOR(j, proces_i + 1, CV[i].size - 1) {
        CV[i].procesy[j - 1] = CV[i].procesy[j];
        CV[i].muteksy[j - 1] = CV[i].muteksy[j];
    }

    CV[i].size--;
}

void wyslijEINTR(int proces) {
    if (usunProcesKolejkiMutexowEINTR(proces)) {
        return;
    }

    FOR(i, 0, MAX_CV - 1) {
        if (CV[i].uzywany == 1) {
            FOR(j, 0, CV[i].size - 1) {
                if (CV[i].procesy[j] == proces) {
                    wyslijWiadomosc(EINTR, proces);
                    usunZakonczonyProcesCV(i, j);
                    break;
                }
            }
        }
    }
}

void usunZakonczonyProces(int proces) {
    int original = aktualnyProces;
    aktualnyProces = proces;

    FOR(i, 0, MAX_MUTEX - 1) {
        if (M[i].uzywany && (M[i].wlasciciel == proces)) {
            cs_unlock(M[i].numer);
        }
    }

    FOR(i, 0, MAX_MUTEX - 1) {
        if (czyPusto(M[i].kolejka) == 0) {
            FOR_EACH (item, M[i].kolejka) {
                if (item->proces == proces) {
                    usunElement(M[i].kolejka, item);
                    break;
                }
            }
        }
    }

    FOR(i, 0, MAX_CV - 1) {
        if (CV[i].uzywany == 1) {
            FOR(j, 0, CV[i].size - 1) {
                if (CV[i].procesy[j] == proces) {
                    usunZakonczonyProcesCV(i, j);
                    break;
                }
            }
        }
    }

    aktualnyProces = original; 
}