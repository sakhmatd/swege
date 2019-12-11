#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "stack.h"

static void
memory_err(void)
{
        fprintf(stderr, "Could not allocate memory.");
        exit(EXIT_FAILURE);
}

static void
null_err(void)
{
        fprintf(stderr, "NULL stack encountered, exiting.");
        exit(EXIT_FAILURE);
}

Stack*
new_stack(void)
{
        Stack *stack = malloc(sizeof(Stack));
        if (!stack)
                memory_err();

        stack->head = NULL;

        return stack;
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
        if (!stack)
                null_err();

        Node *newnode = malloc(sizeof(Node));
        if (!newnode)
                memory_err();

        newnode->data = malloc(PATH_MAX * sizeof(char));
        if (!newnode->data)
                memory_err();

        strncpy(newnode->data, data, PATH_MAX);
        newnode->next = stack->head;
        stack->head = newnode;
}

char*
pop(Stack *stack)
{
        if (!stack)
                null_err();

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

int
is_empty(Stack *stack)
{
        return (stack->head == NULL);
}
