#include "../h/asl.h"
#include "../h/pcb.h"

int MAXINT = 214748364;
typedef struct semd_t
{
    struct semd_t  *s_next;
    int            *s_semAdd;
    pcb_PTR         s_procQ;
} semd_t;

/*define a ASL and a list of free semaphore*/  
HIDDEN semd_t *semd_h;
HIDDEN semd_t *semdFree_h;

void initASL()
{
    /*initlialize ASL with head and tail*/
    static semd_t semdTable[MAXPROC];

    semd_t head;
    semd_t tail;
    semd_h = &head;
    semd_t* asl_tail = &tail;

    /*no asl yet -> head points to tail*/ 
    semd_h->s_next = asl_tail;
    semd_h->s_semAdd = 0;
    asl_tail->s_semAdd = &MAXINT;

    semdFree_h = semdTable; /* initialize sendFree list to contain all the elements of the array semdTable*/
};

int insertBlocked (int *semAdd, pcb_PTR p) {

    semd_t *curr = semd_h;

    while (curr->s_next) {
        /*if the sem is in asl*/ 
        if (curr->s_semAdd == semAdd){
            insertProcQ (&(curr->s_procQ), p);
        }
    }

    /*if the semaphore address is not in the list*/ 
    if(curr->s_semAdd != semAdd && !semdFree_h){
        return TRUE;
    }

    else if (curr->s_semAdd != semAdd) {
        /*allocate from semdFree_h*/ 
        semd_t *freeSemd = semdFree_h;  /*new free semd from the free list*/
        semdFree_h = semdFree_h->s_next;

        /*set properties for free semd*/
        freeSemd->s_semAdd = semAdd;
        freeSemd->s_procQ = mkEmptyProcQ();

        /*add the process p into the new free semd*/
        freeSemd->s_procQ->p_next = p;

        /*insert the semaphore into active list and sort*/ 
        curr = semd_h->s_next;
        semd_t *prev = semd_h;
        while (curr->s_next)
        {
            if(prev->s_semAdd < freeSemd->s_semAdd && curr->s_semAdd > freeSemd->s_semAdd){
                prev->s_next = freeSemd;
                freeSemd->s_next = curr;
            }
        }
    }

    p->p_semAdd = semAdd;
    return FALSE;
}

pcb_PTR removeBlocked (int *semAdd){
    /*search for semAdd in asl*/
    semd_t* curr_semd = semd_h;
    semd_t *prev_semd = NULL;
    while (curr_semd->s_next)
    {
        if (curr_semd->s_semAdd == semAdd){
            /*remove the first pcb in s-procQ*/
            pcb_PTR removed_pcb = headProcQ(curr_semd->s_procQ);
            curr_semd->s_procQ = removed_pcb->p_next;

            removed_pcb->p_semAdd = NULL;

            if (emptyProcQ(curr_semd->s_procQ)){
                prev_semd->s_next = prev_semd->s_next->s_next;
                curr_semd->s_next = semdFree_h;
                semdFree_h = curr_semd;
            }
            return removed_pcb;
        }
        else {
            prev_semd = curr_semd;
            curr_semd = curr_semd->s_next;
        }
    }

    return NULL;
};

pcb_PTR outBlocked (pcb_PTR p){

    /*find the semophore with desired address*/
    semd_t *curr_sem = semd_h;
    while (curr_sem->s_next) {
        if (curr_sem->s_semAdd == p->p_semAdd){
            pcb_PTR foundPcb = outProcQ (&(curr_sem->s_procQ), p);
            if (!foundPcb){
                return NULL;
            }
            return foundPcb;
        }
    }
    return NULL;
}

pcb_PTR headBlocked (int *semAdd){
    semd_t *curr_sem = semd_h;
    while (curr_sem->s_next){
        if (curr_sem->s_semAdd == semAdd){
            pcb_PTR headPcb = headProcQ(curr_sem->s_procQ);
            return headPcb;
        }
    }
    return NULL;
}
