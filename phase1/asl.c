#include "../h/asl.h"
#include "../h/pcb.h" 

int ZERO = 0;
int MAXINT = 2147483647;

/*define a ASL and a list of free semaphore*/  
HIDDEN semd_t *semd_h;          /*pointer to dummy node - head of ASL*/
HIDDEN semd_t *semd_tail;       /*pointer to dummy node - tail of ASL*/
HIDDEN semd_t *semdFree_h;      /*pointer to list of free semaphore*/

/*
 * Function:  traverseToAddress 
 * --------------------
 * traverse through the ASL to find a specific semaphore address
 *    
 *  int* semdAdd: pointer to the semaphore's integer
 *
 *  returns: the pointer points to the semaphore with the put in address
 *           returns NULL if there no semaphore with such address in the ASL
 */
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

/*
 * Function:  initASL
 * --------------------
 * Initialize semdTable with MAXPROC+2 semaphores
 *   connect all semaphores into free list of semaphores
 *   Create head and tail dummy node for the ASL 
 *
 * no returns
 */
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

/*
 * Function:  insertBlocked
 * --------------------
 * insert a process pointed to by p into semaphore with physical address is semAdd
 *    
 *  int* semdAdd: pointer to the semaphore's integer
 *  pcb_PTR p : pointer points to the process that needs added to the semaphore
 *
 *  returns: TRUE if the semaphore is not in ASL and free list is empty
 *           FALSE if we can add process into the ASL
 */

int insertBlocked(int *semAdd, pcb_PTR p) {
    /*find semaphore with semAdd in the ASL*/
    semd_t* currSemd = traverseToAddress(semAdd);
    if (currSemd != NULL) {          /*found semaphore with semAdd*/

        /*set p's semaphore address to semAdd and add p to procQ of that semaphore*/
        p->p_semAdd = semAdd;
        insertProcQ(&(currSemd->s_procQ), p); 
        return FALSE;
    }

    /*if the semaphore is not in the ASL and the free list is empty, return TRUE*/
    if (currSemd == NULL && semdFree_h == NULL){
        return TRUE;
    }

    /*if the semaphore is not in ASL (not active) but the free list is not empty, allocate a new semaphore*/
    if (currSemd == NULL && semdFree_h != NULL){

        /*take out 1 free semd form free list*/
        semd_t *freeSemd = semdFree_h;
        semdFree_h = semdFree_h->s_next;    /*point head of free list to next semaphore in the list*/

        freeSemd->s_semAdd = semAdd;        /*set semaphore address to semAdd*/
        freeSemd->s_procQ = mkEmptyProcQ(); /*initialize a process queue for the semaphore*/
        p->p_semAdd = semAdd;               /*assign p's semaphore address to semAdd*/
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

/*
 * Function:  removeSemd 
 * --------------------
 * traverse through the ASL to find a specific semaphore address
 *      and remove that semaphore if found
 *    
 *  int* semdAdd: integer pointer pointing to the semaphore's physical address
 *
 *  returns: the removed semaphore if it is found in the ASL
 *           returns NULL if there no semaphore with such address in the ASL
 */
HIDDEN semd_t* removeSemd(int* semdAdd){
    semd_t* curr = semd_h->s_next;      /*two pointers to keep track of the semaphores and rreconnect after remove*/
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

/*
 * Function:  removeBlocked 
 * --------------------
 * Search the ASL for a descriptor of semaphore with physical address semAdd
 *    remove the first pcb from the process queue of the found semaphore descriptor, 
 *    set that pcb’s address to NULL, then return a pointer to it.
 *
 *  int* semdAdd: pointer to the semaphore's physical adress
 *
 *  returns: If semaphore is found, returns the pointer points to the first (removed) pcb in the semaphore found
 *           returns NULL if there no semaphore with such address in the ASL
 */
pcb_PTR removeBlocked (int *semdAdd){
    semd_t* currSemd;
    currSemd = traverseToAddress(semdAdd);
    /*if semdAdd not found, return NULL*/
    if (currSemd == NULL)
        return NULL;
    
    /*remove first pcb*/
    pcb_PTR firstProc = removeProcQ(&(currSemd->s_procQ));

    if(firstProc != NULL)   /*if process queue not empty, set removed process's semAdd to NULL*/
        firstProc->p_semAdd = NULL;

    /*if the process queue of semaphore get empty*/
    if (emptyProcQ(currSemd->s_procQ) == 1)
    {   
        semd_t* desSemd = removeSemd(semdAdd);  /*remove semaphore from active list*/
        desSemd->s_next = semdFree_h;           /*add removed semaphore to free list*/
        semdFree_h = desSemd;
    }
    return firstProc;
}

/*
 * Function:  headBlocked 
 * --------------------
 *  Return a pointer to the pcb that is at the head of the process queue associated 
 *      with the semaphore semAdd
 *
 *  int* semdAdd: pointer to the semaphore's physical adress
 *
 *  returns: If semaphore is found, returns the pointer points to the first pcb in the semaphore found
 *              without removing it 
 *           returns NULL if there no semaphore with such address in the ASL
 */

pcb_PTR headBlocked (int *semAdd){
    semd_t *currSemd;
    currSemd = traverseToAddress(semAdd);
    if (currSemd == NULL)
        return NULL;
    pcb_PTR headProc = headProcQ(currSemd->s_procQ);
    return headProc;
}

/*
 * Function:  outBlocked 
 * --------------------
 * Search the ASL for a descriptor of semaphore with physical address semAdd
 *    remove the first pcb from the process queue of the found semaphore descriptor, 
 *      then return a pointer to it (keeping p sem address as is).
 *
 *  int* semdAdd: pointer to the semaphore's physical adress
 *
 *  returns: If semaphore is found, returns the pointer points to the first (removed) pcb in the semaphore found
 *           returns NULL if there no semaphore with such address in the ASL
 */

pcb_PTR outBlocked(pcb_PTR p){
    int *semAdd = p->p_semAdd;
    semd_t *currSem;
    currSem = traverseToAddress(semAdd); /*find semaphore with semAdd physical address*/
    if (currSem == NULL)
        return NULL;
    /*return pointer to the process p pointed to by p in the semaphore*/
    pcb_PTR removedProc = outProcQ(&(currSem->s_procQ), p);     /*if p is not found in the process queue, outProcQ returns NULL*/
    return removedProc;
}
