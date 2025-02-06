/*
asl.c: Manages an Active Semaphore List (ASL).

- Maintains an ASL for semaphores and a free list for semaphore allocation.
- Provides functions for inserting, removing, and accessing processes.
- Each semaphore descriptor contains a process queue for processes.
- Manages semaphore creation and cleanup
*/ 

#include "../h/asl.h"
#include "../h/pcb.h" 

/*define integer semaphores for sentinel nodes*/
#define ZERO 0
#define MAXINT 2147483647

/*define a ASL and a list of free semaphore*/  
HIDDEN semd_t *semd_h;          /*pointer to dummy node - head of ASL*/
HIDDEN semd_t *semd_tail;       /*pointer to dummy node - tail of ASL*/
HIDDEN semd_t *semdFree_h;      /*pointer to list of free semaphore*/

/*
 * Function:  traverseToAddress  
 * --------------------
 * traverse through the ASL to find a specific semaphore address
 *    
 *  int* semdAdd: integer pointer pointing to the semaphore's physical address
 *  semd_t **prev : helper pointer for traversal
 *  semd_t **curr : helper pointer for traversal
 *
 *  On return, curr points to sem with semAdd, 
 *             prev points to semaphore prior to the found semaphore
 */
HIDDEN void traverseToAddress(int* semdAdd, semd_t **prev, semd_t **curr){

    /*two pointers to keep track of the semaphores and reconnect after remove*/
    *curr = semd_h->s_next;      
    *prev = semd_h;

    while(*curr != NULL){
        if ((*curr)->s_semAdd == (semdAdd)){
            /*if semAdd in ASL*/
            return;
        }
        (*prev) = (*curr);
        (*curr) = (*curr)->s_next;
    }

    /*set both to NULL if not found semAdd*/
    (*curr) = NULL;
    (*prev) = NULL;

    return;
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
            /*connect semaphores*/
            semdTable[i].s_next = &semdTable[i + 1];
        }
        else{
            /*set the last one s_next to NULL*/
            semdTable[i].s_next = NULL;
        }
    }

    semdFree_h = &semdTable[0];    /*point head point of free list to the semaphore linked list*/

    /*create dummy head  node*/
    semd_t* freeSemdHead = semdFree_h;
    semdFree_h = semdFree_h->s_next;

    /*create dummy tail node*/
    semd_t *freeSemdTail = semdFree_h;
    semdFree_h = semdFree_h->s_next;

    semd_h = freeSemdHead;
    semd_tail = freeSemdTail;

    /*connect head and tail dummy node*/
    semd_h->s_next = semd_tail;
    semd_tail->s_next = NULL;

    /*assign address for head and tail dummy node*/
    semd_h->s_semAdd = ZERO;
    semd_tail->s_semAdd = MAXINT;

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
    semd_t* prevSemd = NULL;
    semd_t* currSemd = NULL;
    traverseToAddress(semAdd, &prevSemd, &currSemd);    /*find semaphore with semAdd physical address*/
    prevSemd = NULL;  /*set unused pointer to NULL*/
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

    /*if the semaphore is not in ASL but the free list is not empty, allocate a new semaphore*/
    if (currSemd == NULL && semdFree_h != NULL){

        /*take out 1 free semaphore form free list*/
        semd_t *freeSemd = semdFree_h;
        semdFree_h = semdFree_h->s_next;    /*point head of free list to next semaphore in the list*/

        freeSemd->s_semAdd = semAdd;
        freeSemd->s_procQ = mkEmptyProcQ(); /*initialize a process queue for the semaphore*/
        p->p_semAdd = semAdd;
        insertProcQ(&(freeSemd->s_procQ), p);

        /*find the right place for the next semaphore*/
        semd_t *next = semd_h->s_next;
        semd_t *prev = semd_h;
        while (next != NULL){
            if ((prev->s_semAdd) < (semAdd) && (next->s_semAdd) > (semAdd)){
                /*if semdAdd in is between 2 node by numerical value*/
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
 * Function:  removeBlocked 
 * --------------------
 * Search the ASL for a descriptor of semaphore with physical address semAdd
 *    remove the first pcb from the process queue of the found semaphore descriptor, 
 *    set that pcb’s address to NULL, then return a pointer to it.
 *
 *  int* semdAdd: pointer to the semaphore's physical adress
 *
 *  returns: If semaphore is found,remove and returns the pointer points to the first pcb in the semaphore found
 *           returns NULL if there no semaphore with such address in the ASL
 */
pcb_PTR removeBlocked (int *semdAdd){
    semd_t* currSemd=NULL;
    semd_t* prev = NULL;
    traverseToAddress(semdAdd, &prev, &currSemd);   /*find semaphore with semAdd physical address*/

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
        /*remove sem from ASL if procQ empty*/
        prev->s_next = currSemd->s_next; 
        currSemd->s_next = NULL;

        /*put sem back into free list*/
        currSemd->s_next = semdFree_h;          
        semdFree_h = currSemd;
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
    semd_t *currSemd=NULL;
    semd_t* prev=NULL;
    traverseToAddress(semAdd, &prev, &currSemd); /*find semaphore with semAdd physical address*/
    prev=NULL;  /*set unused pointer to NULL*/

    /*if semAdd not in ASL*/
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
    int* semAdd = p->p_semAdd;
    semd_t* currSem = NULL;
    semd_t* prev = NULL;

    traverseToAddress(semAdd, &prev, &currSem); /*find semaphore with semAdd physical address*/
    prev = NULL;  /*set unused pointer to NULL*/

    /*return NULL if semAdd not*/
    if (currSem == NULL)
        return NULL;

    /*return pointer to the process p pointed to by p in the semaphore*/
    pcb_PTR removedProc = outProcQ(&(currSem->s_procQ), p);     /*if p is not found in the process queue, outProcQ returns NULL*/
    return removedProc;
}
