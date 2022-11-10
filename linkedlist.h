#if !defined(_LinkedList_H)
#define _LinkedList_H

typedef struct _listnode {
    void *data;
    unsigned long sz;

    struct _listnode *next;
} Node;

typedef struct _list {
    Node *head;
    Node *tail;
} LinkedList;

Node *AddNode(LinkedList *, void *, unsigned long);
Node *GetHead(LinkedList *);
void DeleteNode(LinkedList *, Node *);
Node *FindNodeByRef(LinkedList *, void *);
void FreeNodes(LinkedList *, int);
void FreeList(LinkedList *);

#endif
