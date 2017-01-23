#include <cv.h>
#include <errno.h>
#include <lib.h>
#include <minixrs.h>

static int get_cv() {             
  int cv;
  minix_rs_lookup("cv", &cv);     
  return cv;     
}

int send_message(int type, int mutex_id, int cv_id) {               
  int cv = get_cv();              
  message m;     
  m.m1_i1 = mutex_id;             
  m.m1_i2 = cv_id;                
  return _syscall(cv, type, &m);  
}

int cs_lock(int mutex_id) {       
  int result;
  while (-1 == (result = send_message(CS_LOCK, mutex_id, 0)))       
    if (EINTR == errno)           
      continue;  
    else
      return -1; 
  return result; 
}

int cs_unlock(int mutex_id) {     
  return send_message(CS_UNLOCK, mutex_id, 0);     
}

int cs_wait(int cv_id, int mutex_id) {             
  int result = send_message(CS_WAIT, mutex_id, cv_id);              
  if (-1 == result)               
    if (EINTR == errno)           
      return (-1 == cs_lock(mutex_id)) ? -1 : 0;   
    else         
      return -1; 
  return result; 
}

int cs_broadcast(int cv_id) {     
  return send_message(CS_BROADCAST, 0, cv_id);     
}