#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#define CHUNK_SIZE 1000
int progress = 0;

pthread_mutex_t TASKS_LOCK;

typedef struct t {
    int start;
    int end;
    int taken;
    int taskNo;
} Task;

typedef struct n {
    int value;
    Task* task;
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
    List* tasks;
    int size;
} Data;

Node* createNode(int);
List* createList();
void removeList(List*);
void addLast(List*, int);
void removeFirst(List*);
void printList(List*, int);

int isPrime(int);
List* findPrimes(int, int);
Data* prepareTasks(int, int);
void addTask(List*, Task*);
Task* removeTask(List*);
void printTasks(List*, int);

void* threadFunction(void* d) {
    Data* data = d;
    Task* task = NULL;
    while(data->tasks->elements > 0) {
        task = removeTask(data->tasks);
        if(task == NULL) return NULL;
        // printf("Thread {%ld} starts processing chunk {%d/%d}\n", pthread_self(), task->taskNo, data->size);
        for(int i = task->start; i < task->end; i++) {
            if(isPrime(i)) {
                addLast(data->list, i);
            }
        }
        progress++;
        // printf("Thread {%ld} finished processing chunk {%d/%d} %.2f%% ready\n", pthread_self(), task->taskNo + 1, data->size, (double) progress*100/data->size);
    }
    // printf("Thread {%ld} - no more tasks\n", pthread_self());
    return NULL;
}

int main(void) {
    if(pthread_mutex_init(&TASKS_LOCK, NULL) != 0) return 1;
    double time = 0.0;
    clock_t begin = clock();

    List* list = findPrimes(0, 100000);
    if(list == NULL) return 1;
    // printList(list, 1);
    printf("\n# of elements = %d\n", list->elements);
    removeList(list);
    free(list);
    pthread_mutex_destroy(&TASKS_LOCK);

    clock_t end = clock();
    time += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Elepsed time: %fs\n", time);

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
        free(tmp->task);
        free(tmp);
        l->head = l->head->next;
    }
    free(l->head);
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
            printf("%7d. %d\t", cnt, head->value);
            if(cnt % columns == 0) printf("\n");
            cnt++;
        }
        head = head->next;
    }
    printf("\n");
}

int isPrime(int n) {
    if(n < 2) return 0;
    for(int i = 2; i < n; i++) {
        if(n % i == 0) return 0;
    }
    return 1;
}

List* findPrimes(int s, int e) {
    List* list = createList();
    if(list == NULL) return NULL;
    int threadsNumber = get_nprocs() - 1 > 2 ? get_nprocs() - 1 : 2;
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threadsNumber);
    Data* data = prepareTasks(s, e);
    data->list = list;

    for(int i = 0; i < threadsNumber; i++) {
        pthread_create(&threads[i], NULL, threadFunction, data);
    }

    for(int i = 0; i < threadsNumber; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    removeList(data->tasks);
    free(data->tasks);
    free(data);
    return list;
}

Data* prepareTasks(int s, int e) {
    Data* data = (Data*)malloc(sizeof(Data));
    List* tasks = createList();
    int i = 0, start = s, end = e; 
    while(start < end) {
        Task* t = (Task*)malloc(sizeof(Task));
        t->start = start;
        t->taken = 0;
        t->taskNo = i;
        if(start + CHUNK_SIZE <= end) t->end = start + CHUNK_SIZE;
        else t->end = end;
        addTask(tasks, t);
        i++;
        start += CHUNK_SIZE;
    }
    data->tasks = tasks;
    data->size = i;
    // printTasks(tasks, 1);
    return data;
}

void addTask(List* l, Task* t) {
    pthread_mutex_lock(&l->lock);
    Node* n = createNode(-1);
    n->task = t;
    n->prev = l->tail->prev;
    n->next = l->tail;
    l->tail->prev->next = n;
    l->tail->prev = n;
    l->elements++;
    pthread_mutex_unlock(&l->lock);
}

Task* removeTask(List* l) {
    pthread_mutex_lock(&l->lock);
    if(l->elements == 0) return NULL;
    Node* tmp = l->head->next;
    l->head->next->next->prev = l->head;
    l->head->next = l->head->next->next;
    l->elements--;
    Task* task = tmp->task;
    free(tmp);
    pthread_mutex_unlock(&l->lock);
    return task;
}

void printTasks(List* l, int columns) {
    int cnt = 1;
    Node* head = l->head;
    while(head != NULL) {
        if(head->value != INT_MIN && head->value != INT_MAX) {
            printf("%7d. s: %d e: %d\t", cnt, head->task->start, head->task->end);
            if(cnt % columns == 0) printf("\n");
            cnt++;
        }
        head = head->next;
    }
    printf("\n");
}