/*
Long Pham
*/

#include "../h/pcb.h"

HIDDEN pcb_PTR pcbFree_h = NULL;

void freePcb (pcb_PTR p)
/* 
Insert the element pointed to by p onto the pcbFree list. 
PARAMETERS: pointer to pcb p
RETURN VALUES: void
*/
{
    
    if (pcbFree_h == NULL){
    /*pcbFree list empty, point its head to p*/
        pcbFree_h = p;
        pcbFree_h->p_next = NULL;
    }

    else{
    /*pcbFree list not empty, point its head to p, update p->p_next to the previous head*/
        pcb_PTR head = pcbFree_h;
        pcbFree_h = p;
        pcbFree_h->p_next = head;
    }
    return;
}

pcb_PTR allocPcb ()
/* Return NULL if the pcbFree list is empty. Otherwise, remove
an element from the pcbFree list, provide initial values for ALL
of the pcbs fields (i.e. NULL and/or 0) and then return a pointer
to the removed element. pcbs get reused, so it is important that
no previous value persist in a pcb when it gets reallocated.
PARAMETERS: None
RETURN VALUES: NULL if the pcbFree list is empty. Otherwise return a pointer
to the removed element
 */
{
    if(pcbFree_h == NULL)
    /*pcbFree list empty, return NULL*/
        return NULL;

    /*extract first pcb in pcbFree list*/
    pcb_PTR p = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next;

    /*provide initial values for ALL of the pcbs fields*/
    p->p_next = NULL;
    p->p_prev = NULL;
    p->p_prnt = NULL;
    p->p_child = NULL;
    p->p_sib = NULL;

    p->p_s.s_entryHI = 0;
    p->p_s.s_cause = 0;
    p->p_s.s_status = 0;
    p->p_s.s_pc = 0;
    int i;
    for (i = 0; i < STATEREGNUM; i++)
        p->p_s.s_reg[i] = 0;

    p->p_time = 0;
    p->p_semAdd = 0;

    return p;
}

void initPcbs ()
/* 
Initialize the pcbFree list to contain all the elements of the
static array of MAXPROC pcbs. This method will be called only
once during data structure initialization.
PARAMETERS: None
RETURN VALUES: void
*/
{
    /*static memory for MAXPROC number of pcbs*/
    static pcb_t static_array [MAXPROC]; 

    /*go through each element in static array
    and set p_next of memory block i to block i+1.
    pcbFree_h points to static_array[0] at the end.*/
    pcbFree_h = &(static_array[MAXPROC - 1]);
    pcbFree_h->p_next = NULL;
    int i = MAXPROC - 1;
    while (i > 0){
        pcbFree_h = &static_array[i-1];
        pcbFree_h->p_next = &static_array[i];
        i -= 1;
    }
    return;
}

pcb_PTR mkEmptyProcQ ()
/* 
This method is used to initialize a variable to be tail pointer to a
process queue.
PARAMETERS: None
RETURN VALUES: a pointer to the tail of an empty process queue (NULL) 
*/
{
    pcb_PTR tp = NULL;
    return tp;
}

int emptyProcQ (pcb_PTR tp)
/* 
Check if the queue whose tail is pointed to by tp is empty
PARAMETERS: tail pointer of the queue in check (tp)
RETURN VALUES: TRUE if queue is empty, FALSE otherwise
*/
{
    if (tp == NULL)
    /*tail pointer is NULL, queue empty*/
        return TRUE;
    return FALSE;
}

void insertProcQ (pcb_PTR *tp, pcb_PTR p)
/*
Insert the pcb pointed to by p into the process queue whose tail-
pointer is pointed to by tp. Note the double indirection through tp
to allow for the possible updating of the tail pointer as well. 
PARAMETERS: pointer to pointer to tail of process queue tp, pointer to 
pcb to be inserted p
RETURN VALUES: void
*/
{
    if (emptyProcQ(*tp))
    /*process queue empty, point *tp to p
    and set p's p_next and p_prev pointers to point to p*/
    {
        *tp = p;
        p->p_next = p;
        p->p_prev = p;
        return;
    }
    
    pcb_PTR hp = (*tp)->p_next; /*save the head pointer*/

    /*link tail and new element*/
    (*tp)->p_next = p;
    p->p_prev = *tp;

    /*link head and new element*/
    p->p_next = hp;
    hp->p_prev = p;

    /*update tail*/
    (*tp) = p;
    return;
}

pcb_PTR removeProcQ (pcb_PTR *tp)
/*
Remove the first (i.e. head) element from the process queue whose
tail-pointer is pointed to by tp. Update the process queue’s tail pointer if necessary.
PARAMETERS: pointer to pointer to tail of process queue tp
RETURN VALUES: NULL if the process queue was initially empty;
otherwise return the pointer to the removed element.
*/
{
    pcb_PTR removed;
    if (emptyProcQ(*tp))
    /*process queue empty, return NULL*/
        removed = NULL;
    
    else if (headProcQ(*tp) == (*tp)){
    /*edge case: process queue has one element,
    return the tail and null it out*/
        removed = (*tp);
        (*tp) = NULL;
    }
    else{
    /*>1 pcb in process queue, get the head and update pointers*/
        removed = (*tp)->p_next;
        (*tp)->p_next = removed->p_next;
        removed->p_next->p_prev = (*tp);

        /*reset removed pcb's pointers*/
        removed->p_next = NULL;
        removed->p_prev = NULL;
    }
    
    return removed;
}

pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p)
/* 
Remove the pcb pointed to by p from the process queue whose tail-
pointer is pointed to by tp. Update the process queue’s tail pointer if
necessary. Note that p can point to any element of the process queue. 
PARAMETERS: pointer to pointer to tail of process queue tp, pointer to 
pcb to be removed p
RETURN VALUES: NULL if the desired entry is not in the indicated queue (an error
condition); otherwise, return p
*/
{
    if (emptyProcQ(*tp))
    /*process queue empty, return NULL*/ 
        return NULL;

    if (headProcQ(*tp) == (*tp))
    /*edge case: process queue has one element */
    {
        if (p == (*tp))
        /*check if the one element is p. If yes, null out the tail
        and return p, otherwise return NULL*/
        {
            (*tp) = NULL;
            return p;
        }
        else 
            return NULL;
    }

    /*check if p is in process queue*/
    pcb_PTR search = (*tp)->p_next;
    while(search != *tp) 
    {
        if (search == p)
            break;
        search = search->p_next;
    }

    if (search == *tp && search != p)
    /*done searching but couldn't find p, return NULL*/
        return NULL; 
    
    /*p found, update pointers of neighboring pcbs*/
    p->p_prev->p_next = p->p_next;
    p->p_next->p_prev = p->p_prev;

    /*p is tail, update tail*/
    if (p == (*tp)) 
        (*tp) = p->p_prev;
    
    /*reset p's pointers after removal*/
    p->p_next = NULL;
    p->p_prev = NULL;
    return p;
}

pcb_PTR headProcQ (pcb_PTR tp)
/* 
Get head of process queue whose tail is pointed to by tp.
Do not remove this pcb from the process queue.
PARAMETERS: pointer to tail of process queue tp
RETURN VALUES: pointer to the first pcb from the process queue whose tail
is pointed to by tp; NULL if the process queue is empty
*/
{
    if (emptyProcQ(tp)) 
    /*process queue empty, return NULL*/
        return NULL;
    return tp->p_next; /*return head*/
}


int emptyChild (pcb_PTR p)
/* 
Return TRUE if the pcb pointed to by p has no children. Return
FALSE otherwise. 
PARAMETERS:
RETURN VALUES:
*/
{
    if (p->p_child == NULL) 
        return TRUE;
    return FALSE;
}


void insertChild (pcb_PTR prnt, pcb_PTR p)
{
    /*insert at the head*/
    pcb_PTR first_child = prnt->p_child;
    prnt->p_child = p;
    prnt->p_child->p_sib_left = NULL;
    prnt->p_child->p_sib = first_child;
    if (first_child != NULL)
        first_child->p_sib_left = prnt->p_child;
    
    prnt->p_child->p_prnt = prnt;
    return;
}


pcb_PTR removeChild (pcb_PTR p)
{
    if(p->p_child == NULL)
        return NULL;

    pcb_PTR removed = p->p_child;
    p->p_child = p->p_child->p_sib;
    if (p->p_child != NULL)
        p->p_child->p_sib_left = NULL;

    removed->p_sib = NULL;
    removed->p_sib_left = NULL;
    return removed;
}

pcb_PTR outChild (pcb_PTR p)
{
    if (p->p_prnt == NULL)
        return NULL;
    if (p->p_prnt->p_child == p)
        return removeChild(p->p_prnt);

    p->p_sib_left->p_sib = p->p_sib;
    
    if (p->p_sib != NULL)
        p->p_sib->p_sib_left = p->p_sib_left;

    p->p_sib = NULL;
    p->p_sib_left = NULL;
    p->p_prnt = NULL;
    return p;
}
