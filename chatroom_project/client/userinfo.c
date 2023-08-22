#include "client.h"

// 初始化头节点
user_friend_p user_friend_init()
{
    // 申请链表头
    user_friend_p head = malloc(sizeof(user_friend_t));
    if (head == NULL)
    {
        perror("fial user_friend_init");
        return NULL;
    }
    memset(head->ID, 0, sizeof(head->ID));
    head->flag = 0;
    head->next = head;
    head->prev = head;

    return head;
}

// 尾插节点
void insertNode(user_friend_p head, char id[], int flag)
{
    user_friend_p newNode = (user_friend_p)malloc(sizeof(user_friend_t));
    strcpy(newNode->ID, id);
    newNode->flag = flag;
    newNode->next = NULL;
    newNode->prev = NULL;

    if (head->next == head)
    {
        // 空链表
        newNode->next = head;
        newNode->prev = head;
        head->next = newNode;
        head->prev = newNode;
    }
    else
    {
        // 不是空链表，则添加到链表尾部
        newNode->next = head;
        newNode->prev = head->prev;
        head->prev->next = newNode;
        head->prev = newNode;
    }
}

// 根据用户名删除节点
void deleteNode(user_friend_p head, char id[])
{
    user_friend_p curr = head->next;
    while (curr != NULL)
    {
        if (strcmp(curr->ID, id) == 0)
        {
            curr->prev->next = curr->next;
            if (curr->next != NULL)
            {
                curr->next->prev = curr->prev;
            }
            free(curr);
            return;
        }
        curr = curr->next;
    }
}

// 遍历链表
void traverseList(user_friend_p head)
{
    user_friend_p curr = head->next;
    int count = 0;
    while (curr != head)
    {
        printf("[%d] Account: %s, online_status: %d\n", ++count, curr->ID, curr->flag);
        curr = curr->next;
    }
    printf("\n");
}

// 销毁链表
void destroyList(user_friend_p head)
{
    user_friend_p curr = head->next;
    while (curr != head)
    {
        user_friend_p temp = curr;
        curr = curr->next;
        free(temp);
    }
    free(head);
    head = NULL;
}