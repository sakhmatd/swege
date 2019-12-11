#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "stack.h"

void
new_stack(Stack *stack)
{
        stack = malloc(sizeof(Stack));
        stack->head = NULL;
}

void
free_stack(Stack *stack)
{
        while (stack->head != NULL) {
                pop(stack);
        }

        free(stack);
}

void
push(Stack *stack, char *data)
{
        Node *newnode = malloc(sizeof(Node));
        newnode->data = malloc(PATH_MAX * sizeof(char));
        strncpy(newnode->data, data, PATH_MAX);
        newnode->next = stack->head;
        stack->head = newnode;
}

char*
pop(Stack *stack)
{
        if (stack->head == NULL) {
                return NULL;
        }

        Node *tmp;
        char *ret;

        tmp = stack->head;
        ret = tmp->data;
        stack->head = stack->head->next;

        free(tmp);

        return ret;
}

char*
top(Stack *stack)
{
        return stack->head->data;
}
