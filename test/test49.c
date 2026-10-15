#include <stddef.h>

struct foo {
    int val;
    struct foo *next;
};

union holder {
    struct foo *link;
    int number;
};

struct foo head;
struct foo mid;
struct foo tail;
struct foo *list[2];
union holder box;

int main() {
    struct foo *ptr;

    tail.val= 20;
    tail.next= NULL;
    mid.val= 15;
    mid.next= &tail;
    head.val= 10;
    head.next= &mid;

    /* One selection after another follows the chain of links, whichever
     * way the base of the chain is written. */
    ptr= &head;
    print head.val;
    print ptr->val;
    print ptr->next->val;
    print ptr->next->next->val;
    print (*ptr).next->val;
    print (*ptr).next->next->val;

    /* An element of an array may be the base, and so may a member. */
    list[0]= &head;
    list[1]= &mid;
    print list[0]->val;
    print list[1]->next->val;

    box.link= &tail;
    print box.link->val;
    return(0);
}
