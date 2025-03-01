/*
pcb.c: Implements a queue manager and tree representation for Process Control Blocks (PCBs)
using linked lists. 

Written by Long Pham & Vuong Nguyen

- Maintains a free list (`pcbFree`) for efficient PCB allocation and deallocation.
- Implements process queues as circular doubly linked lists.
- Represents parent-child relationships using a singly linked list.
- Provides functions for queue operations (insert, remove, search) and tree management.
*/

#include "../h/pcb.h"

/*private global variable that points to the head of the pcbFree list*/
HIDDEN pcb_PTR pcbFree_h = NULL; 

void freePcb (pcb_PTR p)
/* 
Insert the element pointed to by p onto the pcbFree list. 
PARAMETERS: pointer to pcb p
RETURN VALUES: void
*/
{
    /*update p->p_next to head, set head to p*/
    p->p_next = pcbFree_h;
    pcbFree_h = p;
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
    p->p_supportStruct = NULL;

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
        freePcb(&static_array[i-1]);
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
    return NULL;
}

int emptyProcQ (pcb_PTR tp)
/* 
Check if the queue whose tail is pointed to by tp is empty
PARAMETERS: tail pointer of the queue in check (tp)
RETURN VALUES: TRUE if queue is empty, FALSE otherwise
*/
{
    /*tail pointer is NULL, queue empty*/
    return (tp == NULL);
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
    if(p == NULL)
        return;
        
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
        return NULL;
    
    if (headProcQ(*tp) == (*tp)){
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
    }
    
    /*reset removed pcb's pointers*/
    removed->p_next = NULL;
    removed->p_prev = NULL;
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
        return removeProcQ(tp);
    }

    /*check if p is in process queue*/
    pcb_PTR search = (*tp)->p_next;
    while(search != *tp && search != p) 
        search = search->p_next;

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
Check whether pcb pointed by p has any child 
PARAMETERS: pointer to pcb p
RETURN VALUES: TRUE if the pcb pointed to by p has no children. FALSE otherwise. 
*/
{
    /*p_child NULL, p has no children*/ 
    return p->p_child == NULL;
}

void insertChild (pcb_PTR prnt, pcb_PTR p)
/*
Make the pcb pointed to by p a child of the pcb pointed to by prnt.
PARAMETERS: pointer to parent pcb prnt, pointer to child pcb p
RETURN VALUES: void
*/
{
    /*update p's pointers*/
    p->p_sib = prnt->p_child;
    p->p_sib_left = NULL;
    p->p_prnt = prnt;

    /*update first child to point to p*/
    if (prnt->p_child != NULL)
        prnt->p_child->p_sib_left = p;

    /*update first child to be p*/
    prnt->p_child = p;
    return;
}

pcb_PTR removeChild (pcb_PTR p)
/* 
Make the first child of the pcb pointed to by p no longer a child of p.
PARAMETERS: pointer to pcb p
RETURN VALUES: NULL if initially there were no children of p. Otherwise,
return a pointer to this removed first child pcb. 
*/
{
    pcb_PTR removed;
    if(emptyChild(p))
    /*p has no children, return NULL*/
        removed = NULL;
    else{
    /*p has children, extract the first one*/
        removed = p->p_child;
        /*update pointer of removed child's sibling*/
        if (removed->p_sib != NULL)
            removed->p_sib->p_sib_left = NULL;
        /*update first child to be removed child's sibling*/
        p->p_child = removed->p_sib;

        /*reset removed child's pointers*/
        removed->p_sib = NULL;
        removed->p_sib_left = NULL;
    }
    return removed;
}

pcb_PTR outChild (pcb_PTR p)
/*
* Make the pcb pointed to by p no longer the child of its parent. 
* Note that the element pointed to by p need not be the first child of its parent. 
* PARAMETERS: pointer to child pcb p
* RETURN VALUES: NULL if the pcb pointed to by p has no parent; otherwise return p
*/
{
    if (p->p_prnt == NULL)
    /*pcb pointed to by p has no parent, return NULL*/
        return NULL;

    if (p->p_prnt->p_child == p)
    /*p is its parent's first child, call removeChild on p's parent*/
        return removeChild(p->p_prnt);

    /*update pointers of p's siblings*/
    p->p_sib_left->p_sib = p->p_sib;
    if (p->p_sib != NULL)
        p->p_sib->p_sib_left = p->p_sib_left;

    /*reset p's pointers*/
    p->p_sib = NULL;
    p->p_sib_left = NULL;
    p->p_prnt = NULL;
    return p;
}
