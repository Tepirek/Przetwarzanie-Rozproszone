#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#define ITERATIONS 1000000

typedef struct n {
    int value;
    struct n* prev;
    struct n* next;
} Node;

typedef struct l {
    int elements;
    Node* head;
    Node* tail;
    pthread_mutex_t lock;
} List;

typedef struct d {
    List* list;
    int pid;
} data;

Node* createNode(int);
List* createList();
void removeList(List*);
void addFirst(List*, int);
void addLast(List*, int);
void removeFirst(List*);
void removeLast(List*);
void printList(List*, int);
int isPrime(int);

void* insertElements(void* d) {
    data* data = d;
    List* l = data->list; 
    for(int i = 0; i < ITERATIONS; i++) {
        addLast(l, data->pid);
    }
    return NULL;
}

int main(void) {
    double time = 0.0;
    clock_t begin = clock();
    List* list = createList();
    if(list == NULL) return 1;
    int threadsNumber = get_nprocs() - 1;
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threadsNumber);
    data* params = (data*)malloc(sizeof(data) * threadsNumber);

    for(int i = 0; i < threadsNumber; i++) {
        data s = { list, i };
        params[i] = s;
        pthread_create(&threads[i], NULL, insertElements, &params[i]);
    }

    for(int i = 0; i < threadsNumber; i++) {
        pthread_join(threads[i], NULL);
    }

    // printList(list, 10);
    printf("# of elements = %d\n", list->elements);
    clock_t end = clock();
    time += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Elepsed time: %fs\n", time);
    removeList(list);
    free(threads);
    free(params);
    free(list);

    return 0;
}

Node* createNode(int v) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->value = v;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

List* createList() {
    List* l = (List*)malloc(sizeof(List));
    if(pthread_mutex_init(&l->lock, NULL) != 0) return NULL;
    l->elements = 0;
    l->head = createNode(INT_MAX);
    l->tail = createNode(INT_MIN);
    l->head->next = l->tail;
    l->tail->prev = l->head;
    return l;
}

void removeList(List* l) {
    while(l->head->next != NULL) {
        Node* tmp = l->head;
        free(tmp);
        l->head = l->head->next;
    }
    free(l->head);
    free(l->tail);
    pthread_mutex_destroy(&l->lock);
}

void addFirst(List* l, int v) {
    pthread_mutex_lock(&l->lock);
    Node* n = createNode(v);
    n->prev = l->head;
    n->next = l->head->next;
    l->head->next->prev = n;
    l->head->next = n;
    l->elements++;
    pthread_mutex_unlock(&l->lock);
}

void addLast(List* l, int v) {
    pthread_mutex_lock(&l->lock);
    Node* n = createNode(v);
    n->prev = l->tail->prev;
    n->next = l->tail;
    l->tail->prev->next = n;
    l->tail->prev = n;
    l->elements++;
    pthread_mutex_unlock(&l->lock);
}

void removeFirst(List* l) {
    pthread_mutex_lock(&l->lock);
    if(l->elements == 0) return;
    Node* tmp = l->head->next;
    l->head->next->next->prev = l->head;
    l->head->next = l->head->next->next;
    free(tmp);
    pthread_mutex_unlock(&l->lock);
}

void removeLast(List* l) {
    pthread_mutex_lock(&l->lock);
    if(l->elements == 0) return;
    Node* tmp = l->tail->prev;
    l->tail->prev->prev->next = l->tail;
    l->tail->prev = l->tail->prev->prev;
    free(tmp);
    pthread_mutex_unlock(&l->lock);
}

void printList(List* l, int columns) {
    int cnt = 1;
    Node* head = l->head;
    while(head != NULL) {
        if(head->value != INT_MIN && head->value != INT_MAX) {
            printf("%7d, %d\t", cnt, head->value);
            if(cnt % columns == 0) printf("\n");
            cnt++;
        }
        head = head->next;
    }
    printf("\n");
}

int isPrime(int n) {

    return 0;
}