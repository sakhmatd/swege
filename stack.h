typedef struct Node {
        char *data;
        struct Node *next;
} Node;

typedef struct Stack {
        Node *head;
} Stack;

Stack* new_stack(void);
void free_stack(Stack *stack);
char* pop(Stack *stack);
char* top(Stack *stack);
void push(Stack *stack, char *data);
