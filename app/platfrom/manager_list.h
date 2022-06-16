#ifndef _MANAGER_LIST_H_
#define _MANAGER_LIST_H_

typedef struct manager_list
{
    void *data;
    struct manager_list *next;
    struct manager_list *pre;
} t_manager_list;

/*
* Specific method of searching interface is realized by the user.
* meaning of return value:
*  0: query suceess
*  1: query fail
*/
typedef int (*query_element_callback)(t_manager_list *node, void *target);
/*
* meaning of return value:
*  0: query suceess
*  1: query fail
*/
typedef int (*del_element_callback)(t_manager_list *node, void *parament);

/*
* meaning of return value:
*  0: query suceess
*  1: query fail
*/
typedef int (*modify_element_callback)(t_manager_list *node, void *parament);

typedef int (*test_element_callback)(t_manager_list *node, void *parament);



void init_manager_list(t_manager_list *freq_list);
int count_manager_list(t_manager_list *freq_list);

void insert_head_manager_list(t_manager_list *freq_list, void *data);
void insert_tail_manager_list(t_manager_list *freq_list, void *data);
t_manager_list *get_head_list(t_manager_list *freq_list);

t_manager_list *query_head_list(t_manager_list *freq_list, void *target, query_element_callback cb);
int del_manager_node(t_manager_list *freq_list, void *parament, del_element_callback cb);
int remove_head_manager_node(t_manager_list *freq_list, void *parament);

int modify_manager_node(t_manager_list *freq_list, void *parament, modify_element_callback cb);
int remove_manager_node(t_manager_list *freq_list, void *parament, del_element_callback cb);
void clean_manager_list(t_manager_list *freq_list);
void test_head_list(t_manager_list *freq_list, void *target, test_element_callback cb);


#endif
