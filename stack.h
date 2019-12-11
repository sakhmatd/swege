typedef struct Node {
        char *data;
        struct Node *next;
} Node;

typedef struct Stack {
        Node *head;
} Stack;

void new_stack(Stack *stack);
char* pop(Stack *stack);
char* top(Stack *stack);
void push(Stack *stack, char *data);
