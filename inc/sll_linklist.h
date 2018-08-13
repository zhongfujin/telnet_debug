#ifndef __SLL_LINKLIST__H
#define __SLL_LINKLIST__H

#define RET_SUCCESS 1
#define RET_ERROR   0

#define TRUE    1
#define FALSE   0

typedef int BOOL;

/*单链表节点结构体*/
typedef struct node
{
    struct node *next;
}SLL_LINK_LIST_NODE;

/*单链表结构体定义*/
typedef struct list
{
    SLL_LINK_LIST_NODE *head;
    SLL_LINK_LIST_NODE *tail;
}SLL_LINK_LIST_BASE;

/*测试节点*/
typedef struct test_node
{
    SLL_LINK_LIST_NODE node;    /*节点的next*/
    char buf[32];               /*节点内容*/
}TEST_NODE;

int sll_link_list_init(SLL_LINK_LIST_BASE *list);
int sll_link_list_insert_at_head(SLL_LINK_LIST_BASE *list,void *node);
int sll_link_list_insert_at_index(SLL_LINK_LIST_BASE *list,void *node,int index);
int sll_link_list_insert_at_tail(SLL_LINK_LIST_BASE *list,void *node);
BOOL sll_link_list_empty(SLL_LINK_LIST_BASE *list);

void *sll_link_list_get_head(SLL_LINK_LIST_BASE *list);
void *sll_link_list_get_tail(SLL_LINK_LIST_BASE *list);
void *sll_link_list_get_index(SLL_LINK_LIST_BASE *list,int index);
int sll_link_list_head_remove(SLL_LINK_LIST_BASE * list);
int sll_link_list_tail_remove(SLL_LINK_LIST_BASE *list);
int sll_link_list_node_remove(SLL_LINK_LIST_BASE *list,void *node);

void sll_link_list_destory(SLL_LINK_LIST_BASE *list);


#endif
