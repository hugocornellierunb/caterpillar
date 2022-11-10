#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

Node *AddNode(LinkedList *list, void *data, unsigned long sz) {
    Node *node;
    if (!(list)) {
        return NULL;
    }
    if (!(list->head)) {
        list->head = malloc(sizeof(Node));
        if (!(list->head)) {
            return NULL;
        }
        list->tail = list->head;
        node = list->tail;
    } else {
        list->tail->next = malloc(sizeof(Node));
        if (!(list->tail->next)) {
            return NULL;
        }
        list->tail = list->tail->next;
        node = list->tail;
    }
    memset(node, 0x00, sizeof(Node));
    node->data = malloc(sz);
    memset(node->data, 0x00, sz);
    node->sz = sz;
    node->data = data;
    return node;
}

Node *GetHead(LinkedList *list) {
    Node *node;
    node = list->head;
    return node;
}

void DeleteNode(LinkedList *list, Node *node) {
    Node *del;
    if (!(list)) {
        return;
    }
    if (node == list->head) {
        list->head = node->next;
        free(node->data);
        free(node);
        return;
    }
    del = list->head;
    while (del != NULL && del->next != node) {
        del = del->next;
    }
    if (!(del)) {
        return;
    }
    if (del->next == list->tail) {
        list->tail = del;
    }
    del->next = del->next->next;
    free(node->data);
    free(node);
}

Node *FindNodeByRef(LinkedList *list, void *data) {
    Node *node;
    if (!(list)) {
        return NULL;
    }
    node = list->head;
    while ((node)) {
        if (node->data == data) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

void FreeNodes(LinkedList *list, int FreeParameterAsWell) {
    Node *node, *previous;
    if (!(list)) {
        return;
    }
    node = list->head;
    while (node) {
        free(node->data);
        previous = node;
        node = node->next;
        free(previous);
    }
    if (FreeParameterAsWell)
        free(list);
    return;
}

void FreeList(LinkedList *list) {
    FreeNodes(list, 0);
    return;
}
