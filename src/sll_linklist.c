#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sll_linklist.h"
#include <assert.h>

int sll_link_list_init(SLL_LINK_LIST_BASE *list)
{
    list = malloc(sizeof(SLL_LINK_LIST_BASE));
    if(NULL == list)
    {
        printf("malloc failed!\n");
        assert(0);
        return RET_ERROR;
    }
    list->head = NULL;
    list->tail = NULL;
    return RET_SUCCESS;
}


int sll_link_list_insert_at_head(SLL_LINK_LIST_BASE *list,void *p_node)
{
    if(list == NULL || p_node == NULL)
    {
        printf("param error!\n");
        assert(0);
        return RET_ERROR;
    }
    SLL_LINK_LIST_NODE *node = (SLL_LINK_LIST_NODE *)p_node;
    node->next = list->head;
    list->head = node;
    if(list->tail == NULL)
    {
        list->tail = node;
    }
    return RET_SUCCESS;

}


int sll_link_list_insert_at_index(SLL_LINK_LIST_BASE *list,void *p_node,int index)
{
    long i = 1;
    SLL_LINK_LIST_NODE *node = (SLL_LINK_LIST_NODE *)p_node;
    if(list->head == NULL)
    {
        printf("empty list!\n");
        assert(0);
        return RET_ERROR;
    }
    SLL_LINK_LIST_NODE *tmp_head = NULL;

    tmp_head = list->head;
    while(tmp_head && i < index)
    {
        tmp_head = tmp_head->next;
        i++;
    }

    if(tmp_head)
    {
        node->next = tmp_head->next;
        tmp_head->next = node;
    }
    return RET_SUCCESS;
}


int sll_link_list_insert_at_tail(SLL_LINK_LIST_BASE *list,void *p_node)
{
    SLL_LINK_LIST_NODE *node = (SLL_LINK_LIST_NODE *)p_node;
    SLL_LINK_LIST_NODE *tmp_node = list->head;

    if(list->head == NULL)
    {
        list->head = node;
        list->tail = node;
        node->next = NULL;
        
        return RET_SUCCESS;
    }
    
    while(tmp_node->next != NULL)
    {
        tmp_node = tmp_node->next;
    }
    tmp_node->next = node;
    node->next = NULL;
    return RET_SUCCESS;
}


BOOL sll_link_list_empty(SLL_LINK_LIST_BASE *list)
{
    if(list->head == NULL)
    {
        return TRUE;
    }
    return FALSE;
}


void *sll_link_list_get_head(SLL_LINK_LIST_BASE *list)
{
    if(list == NULL)
    {
        printf("invailed list\n");
        assert(0);
        return NULL;
    }
    if(sll_link_list_empty(list))
    {
        return NULL;
    }
    return list->head;
}


void *sll_link_list_get_tail(SLL_LINK_LIST_BASE *list)
{
    if(list == NULL)
    {
        printf("invailed list\n");
        assert(0);
        return NULL;
    }
    if(sll_link_list_empty(list))
    {
        return NULL;
    }
    SLL_LINK_LIST_NODE *tmp_node = NULL;
    tmp_node = list->head;
    while(tmp_node->next)
    {
        tmp_node = tmp_node->next;
    }
    return tmp_node;
}


void *sll_link_list_get_index(SLL_LINK_LIST_BASE *list,int index)
{

    int i = 1;
    SLL_LINK_LIST_NODE *node = list->head;
    if(index == 1)
    {
        return list->head;
    }
    node = node->next;
    for(i = 1; i <= index; i++)
    {
        if(node->next != NULL)
            node = node->next;
        else
            break;
    }
    return node;
}


int sll_link_list_head_remove(SLL_LINK_LIST_BASE * list)
{
    if(sll_link_list_empty(list))
    {
        printf("empty list\n");
        assert(0);
        return RET_ERROR;
    }
    SLL_LINK_LIST_NODE *node = list->head;
    list->head = list->head->next;
    free(node);
    node = NULL;
    return RET_SUCCESS;
}


int sll_link_list_tail_remove(SLL_LINK_LIST_BASE *list)
{
    if(sll_link_list_empty(list))
    {
        printf("empty list\n");
        assert(0);
        return RET_ERROR;
    }
    SLL_LINK_LIST_NODE *node = list->head;
    SLL_LINK_LIST_NODE *pre_node = NULL;
    while(node->next)
    {
        pre_node = node;
        node = node->next;
    }
    pre_node->next = NULL;
    free(node);
    node = NULL;
    return RET_SUCCESS;
}


int sll_link_list_node_remove(SLL_LINK_LIST_BASE *list,void *node)
{
    if(sll_link_list_empty(list))
    {
        printf("list is empty\n");
        assert(0);
        return RET_ERROR;
    }
    SLL_LINK_LIST_NODE *sllnode = (SLL_LINK_LIST_NODE *)node;
    SLL_LINK_LIST_NODE *p_node = list->head;

    if(node == list->head)
    {
        sll_link_list_head_remove(list);
        return RET_SUCCESS;
    }
    if(node == sll_link_list_get_tail(list))
    {
        sll_link_list_tail_remove(list);
        return RET_SUCCESS;
    }
    while(p_node)
    {
        if(p_node->next == sllnode)
        {
            p_node->next = sllnode->next;
            free(sllnode);
            node = NULL;
            return RET_SUCCESS;
        }
        p_node = p_node->next;
    }
    printf("no such node : %p\n",sllnode);
    return RET_ERROR;
}


void sll_link_list_destory(SLL_LINK_LIST_BASE *list)
{
    if(list->head == NULL)
    {
        free(list);
        return;
    }
    SLL_LINK_LIST_NODE *node = list->head;
    while(node)
    {
        node = node->next;
        free(node);
        node = NULL;
    }
    free(list);
    return;
}




