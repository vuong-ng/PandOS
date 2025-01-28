#include "../h/pcb.h"


HIDDEN pcb_PTR pcbFree_h = NULL;

void freePcb (pcb_PTR p)
{
    if (pcbFree_h == NULL){
        pcbFree_h = p;
        pcbFree_h->p_next = NULL;
    }

    else{
        pcb_PTR head = pcbFree_h;
        pcbFree_h = p;
        pcbFree_h->p_next = head;
    }
    return;
}

pcb_PTR allocPcb ()
{
    if(pcbFree_h == NULL)
        return NULL;
    pcb_PTR p = pcbFree_h;
    pcbFree_h = pcbFree_h->p_next;

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

HIDDEN pcb_t static_array [MAXPROC];
void initPcbs ()
{
    pcbFree_h = &(static_array[MAXPROC - 1]);
    pcbFree_h->p_next = NULL;
    int i = MAXPROC - 1;
    while (i > 0){
        /* pcbFree_h->p_prev = &static_array[i-1]; */
        pcbFree_h = &static_array[i-1];
        pcbFree_h->p_next = &static_array[i];
        i -= 1;
    }
    return;
}

pcb_PTR mkEmptyProcQ ()
{
    struct pcb_t empty_proc_q;
    pcb_PTR tp = &empty_proc_q;
    return tp;
}

int emptyProcQ (pcb_PTR tp)
{
    if (tp == NULL) return 1;
    return 0;
}

void insertProcQ (pcb_PTR *tp, pcb_PTR p)
{
    if (emptyProcQ(*tp))
    {
        *tp = p;
        p->p_next = p;
        p->p_prev = p;
        return;
    }
    
    pcb_PTR hp = (*tp)->p_next;
    (*tp)->p_next = p;
    p->p_prev = *tp;
    (*tp) = p;
    (*tp)->p_next = hp;
    hp->p_prev = (*tp);

    return;
}

pcb_PTR removeProcQ (pcb_PTR *tp)
{
    /* empty*/ 
    if (emptyProcQ(*tp))
        return NULL;

    /*one pcb, head==tail*/
    pcb_PTR removed;
    if (headProcQ(*tp) == (*tp)) 
    {
        removed = (*tp);
        (*tp) = NULL;
        return removed;
    }
        
    removed = (*tp)->p_next;
    (*tp)->p_next = (*tp)->p_next->p_next;
    (*tp)->p_next->p_prev = (*tp);

    removed->p_next = NULL;
    removed->p_prev = NULL;
    return removed;
}

pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p)
{
    /*p guaranteed to be in tp?*/

    if (emptyProcQ(*tp)) 
        return NULL;

    /*one pcb*/
    if (headProcQ(*tp) == (*tp))
    {
        if (p == (*tp))
        {
            (*tp) = NULL;
            return p;
        }
        else return NULL;
    }

    pcb_PTR search = (*tp)->p_next;
    while(search != *tp)
    {
        if (search == p)
            break;
        search = search->p_next;
    }
    if (search == *tp && search != p) return NULL; /*not found*/
    
    
    p->p_prev->p_next = p->p_next;
    p->p_next->p_prev = p->p_prev;

    if (p == (*tp)) 
        (*tp) = p->p_prev;
    
    p->p_next = NULL;
    p->p_prev = NULL;
    return p;
}

pcb_PTR headProcQ (pcb_PTR tp)
{
    if (emptyProcQ(tp)) 
        return NULL;
    return tp->p_next;
}


int emptyChild (pcb_PTR p)
{
    if (p->p_child == NULL) 
        return 1;
    return 0;
}


void insertChild (pcb_PTR prnt, pcb_PTR p)
{
    /*insert head or tail?*/
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
