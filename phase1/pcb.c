#include "../h/pcb.h"


pcb_PTR pcbFree_h = NULL;

void freePcb (pcb_PTR p)
{
    if (pcbFree_h == NULL){
        pcbFree_h = p;
        pcbFree_h->p_next = NULL;
    }

    else{
        pcbFree_h->p_prev = p;
        p->p_next = pcbFree_h;
        pcbFree_h = p;
        pcbFree_h->p_prev = NULL;
    }
    return;
}

pcb_PTR allocPcb ()
{
    if(!pcbFree_h)
        return NULL;
    
    pcb_PTR p = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next;
    pcbFree_h->p_prev = NULL;

    p->p_next = NULL;
    p->p_prev = NULL;
    p->p_prnt = NULL;
    p->p_child = NULL;
    p->p_sib = NULL;
    p->p_s = (struct state_t){};
    p->p_time = 0;
    p->psemAdd = 0;

    return p;
}

void initPcbs ()
{
    pcb_PTR static_array [MAXPROC];
    pcbFree_h = static_array[MAXPROC - 1];
    int i = MAXPROC - 1;
    while (i > 0){
        pcbFree_h->p_prev = static_array[i-1];
        pcbFree_h = static_array[i-1];
        pcbFree_h->p_next = static_array[i];
    }
    return;
    
}