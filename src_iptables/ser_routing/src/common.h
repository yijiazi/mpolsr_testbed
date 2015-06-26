/************************************************************************
*             Projet SEREADMO - N° ANR-05-RNRT-028-01
*              Securite des Reseaux Ad hoc & Mojette
*************************************************************************
*
* AUTHORS    : P.Lesage <pascal.lesage@keosys.com>
* VERSION    : 1.0
*
=========================================================================
*
* TODO
*
*************************************************************************
* Changes :
* --------
*
*************************************************************************
* Used and modified 2015
*   by      Benjamin Mollé (engineering student, Polytech Nantes, University of Nantes, France) benjamin.molle@gmail.com
*           Denis Souron (engineering student, Polytech Nantes, Université of Nantes, France) denis.souron@laposte.net
*************************************************************************/
#ifndef _GLOBAL_H_
#define _GLOBAL_H_

//--------------------------
// define compilation option
//--------------------------

// define ALLOC_WITH_CACHE enable cached allocation for speed optimisation
// (only in kernel level)
#undef  ALLOC_WITH_CACHE

// define APP_TEST enable to use TC files / functions in a user application
#undef APP_TEST
#define APP_TEST


//---------------------------------
// define lock / semaphore handling
//---------------------------------
#ifndef APP_TEST

    // for kernel module
    #define READ_UNLOCK(lock)           read_unlock_bh(&lock)
    #define READ_LOCK(lock)             read_lock_bh(&lock)

    #define WRITE_UNLOCK(lock)          write_unlock_bh(&lock)
    #define WRITE_LOCK(lock)            write_lock_bh(&lock)

#else
      void kprintf(const char *format, ...);
    // for user level application
    #define printk kprintf
    #define KERN_ALERT

    #define READ_UNLOCK(lock)           pthread_mutex_unlock(&lock)
    #define READ_LOCK(lock)             pthread_mutex_lock(&lock)

    #define WRITE_UNLOCK(lock)          pthread_mutex_unlock(&lock)
    #define WRITE_LOCK(lock)            pthread_mutex_lock(&lock)

#endif


//-----------------------------------------------------------------------
// definition of module parameters (same as /etc/modprobe.d/ser_iptables)
//-----------------------------------------------------------------------
extern int g_act_hook;
extern int g_act_device;
extern int g_act_recovery;

extern int g_dbg_hook;
extern int g_dbg_device;
extern int g_dbg_module;
extern int g_dbg_djk;

extern int g_conf_path;
extern int g_conf_fe;
extern int g_conf_fp;
extern int g_conf_udp_port;

char* ipToString(unsigned long ipAddr);

// just to avoid 'unsigned long' and '4' everywhere
typedef unsigned long ip_v4_addr;
#define ip_v4_size  4

#define BUFFER_SIZE 4096
#define MAX_MPOLSR_PATH 2048

//------------------------------
// define debug / trace handling
//------------------------------

#define DBG_MOD(level,fmt, args...) if(g_dbg_module>= level) printk(KERN_ALERT fmt, ##args)
#define DBG_DEV(level,fmt, args...) if(g_dbg_device>= level) printk(KERN_ALERT fmt, ##args)
#define DBG_HK(level,fmt, args...) if(g_dbg_hook>= level) printk(KERN_ALERT fmt, ##args)
#define DBG_DJK(level,fmt, args...) if(g_dbg_djk>= level) printk(KERN_ALERT fmt, ##args)

#define MSG_FAILED(fmt, args...) printk(KERN_ALERT fmt, ##args)


#endif //_GLOBAL_H_

