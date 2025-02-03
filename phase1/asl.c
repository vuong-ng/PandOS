#include "../h/asl.h"
#include "../h/pcb.h" 

int ZERO = 0;
int MAXINT = 2147483647;

/*define a ASL and a list of free semaphore*/  
HIDDEN semd_t *semd_h;
HIDDEN semd_t *semd_tail;
HIDDEN semd_t *semdFree_h;

HIDDEN semd_t* traverseToAddress(int* semdAdd){
    semd_t* curr = semd_h;
    while (curr != NULL)
    {
        if (*(curr->s_semAdd) == *(semdAdd)) {
            return curr;
        }
        curr = curr->s_next;
    }
    return NULL;
}

void initASL()
{
    static semd_t semdTable[MAXPROC+2];

    /*connect semd*/
    int i;
    for (i = 0; i < MAXPROC+2; i++){
        if (i != MAXPROC+1){
            semdTable[i].s_next = &semdTable[i + 1];
        }
        else{
            semdTable[i].s_next = NULL;
        }
    }

    semdFree_h = &semdTable[0];

    semd_t* freeSemdHead = semdFree_h;
    semdFree_h = semdFree_h->s_next;

    semd_t *freeSemdTail = semdFree_h;
    semdFree_h = semdFree_h->s_next;

    semd_h = freeSemdHead;
    semd_tail = freeSemdTail;

    semd_h->s_next = semd_tail;
    semd_tail->s_next = NULL;

    semd_h->s_semAdd = &ZERO;
    semd_tail->s_semAdd = &MAXINT;

};

int insertBlocked(int *semAdd, pcb_PTR p) {
    /*find semaphore with semAdd*/
    semd_t* currSemd = traverseToAddress(semAdd);
    if (currSemd != NULL) {          /*found semaphore with semAdd*/
        p->p_semAdd = semAdd;
        insertProcQ(&(currSemd->s_procQ), p); 
 
        return FALSE;
    }

    if (currSemd == NULL && semdFree_h == NULL){
        return TRUE;
    }

    if (currSemd == NULL && semdFree_h != NULL){

        /*allocate 1 free semd form free list*/
        semd_t *freeSemd = semdFree_h;
        semdFree_h = semdFree_h->s_next;

        freeSemd->s_semAdd = semAdd;
        freeSemd->s_procQ = mkEmptyProcQ();
        p->p_semAdd = semAdd;
        insertProcQ(&(freeSemd->s_procQ), p);

        /*find the right place for the next semaphore*/
        semd_t *next = semd_h->s_next;
        semd_t *prev = semd_h;
        while (next != NULL){
            if (*(prev->s_semAdd) < *(semAdd) && *(next->s_semAdd) > *(semAdd)){
                prev->s_next = freeSemd;
                freeSemd->s_next = next;
                return FALSE;
            }
            prev = next;
            next = next->s_next;
            
        }
        return FALSE;
    }
    return FALSE;
};

HIDDEN semd_t* removeSemd(int* semdAdd){
    semd_t* curr = semd_h->s_next;
    semd_t* prev = semd_h;
    while(curr != NULL){
        if (*(curr->s_semAdd) == *(semdAdd)){
            prev->s_next = curr->s_next;
            curr->s_next = NULL;
            return curr;
        }
        prev = curr;
        curr = curr->s_next;
    }
    return NULL;
}
pcb_PTR removeBlocked (int *semdAdd){
    semd_t* currSemd;
    currSemd = traverseToAddress(semdAdd);
    /*if semdAdd not found, return NULL*/
    if (currSemd == NULL)
        return NULL;
    
    /*headProcQ(currSemd->s_procQ);*/
    pcb_PTR firstProc = removeProcQ(&(currSemd->s_procQ));

    if(firstProc != NULL)   /*if process queue not empty, set removed process's semAdd to NULL*/
        firstProc->p_semAdd = NULL;

    
    if (emptyProcQ(currSemd->s_procQ) == 1)
    {   
        semd_t* desSemd = removeSemd(semdAdd);  /*remove from active list*/
        desSemd->s_next = semdFree_h;
        semdFree_h = desSemd;
    }
    return firstProc;
}

pcb_PTR headBlocked (int *semAdd){
    semd_t *currSemd;
    currSemd = traverseToAddress(semAdd);
    if (currSemd == NULL)
        return NULL;
    pcb_PTR headProc = headProcQ(currSemd->s_procQ);
    return headProc;
}

pcb_PTR outBlocked(pcb_PTR p){
    int *semAdd = p->p_semAdd;
    semd_t *currSem;
    currSem = traverseToAddress(semAdd);
    if (currSem == NULL)
        return NULL;
    pcb_PTR removedProc = outProcQ(&(currSem->s_procQ), p);
    return removedProc;
}
