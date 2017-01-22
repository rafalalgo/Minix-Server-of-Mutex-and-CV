#ifndef _CONDITION_VARIABLE_H_
#define _CONDITION_VARIABLE_H_

#define CS_LOCK             1
#define CS_UNLOCK           2
#define CS_WAIT             3
#define CS_BROADCAST        4

// utils
static int get_cv();
int send_message(int type, int mutex_id, int cv_id);

// lock and unlock mutexes
int cs_lock(int mutex_id);
int cs_unlock(int mutex_id);

// wait and broadcast
int cs_wait(int cv_id, int mutex_id);
int cs_broadcast(int cv_id);

#endif
