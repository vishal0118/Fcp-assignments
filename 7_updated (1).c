/*
 * 7_updated.c - Interactive Command-Driven Task Manager
 *
 * Task: console task-list manager. The typed command word is converted to an
 * enum (CommandType) and a switch runs the matching operation. Commands are not
 * case-sensitive: ADD, DELETE, VIEW, UPDATE, COMPLETE, FIND, HELP, EXIT.
 * Tasks (ID, description, PENDING / DONE) live in a dynamic array that doubles
 * with realloc when full. Input uses fgets, so over-long lines are detected.
 * Nothing is saved to a file.
 *
 * Sample run: add 101 "Buy milk" -> ADD 102 "Write report" -> view ->
 *   complete 101 -> find 101 -> delete 102 -> help -> exit.
 *
 * Edge cases tested: mixed-case commands, unknown command, command that is too
 *   long, duplicate ID, missing task, malformed ID, spaces in descriptions,
 *   empty description, empty list, end of input (EOF), array growth.
 *
 * Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 7_updated.c -o tasks
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DESC_SIZE 100
#define MAX_ID 1000000000

typedef enum {
    CMD_ADD,
    CMD_DELETE,
    CMD_VIEW,
    CMD_UPDATE,
    CMD_COMPLETE,
    CMD_FIND,
    CMD_HELP,
    CMD_EXIT,
    CMD_UNKNOWN
} Command;

typedef enum { PENDING, DONE } Status;

typedef struct {
    int id;
    char desc[DESC_SIZE];
    Status status;
} Task;

typedef struct {
    Task *data;
    size_t count;
    size_t capacity;
} List;

static const char *names[] = {"ADD", "DELETE", "VIEW", "UPDATE", "COMPLETE", "FIND", "HELP", "EXIT"};

/* Converts a command word to the enum, ignoring case */
static Command parse(const char *text)
{
    char up[16];
    size_t i;

    for (i = 0; text[i] != '\0' && i < sizeof up - 1; i++) {
        up[i] = (char)toupper((unsigned char)text[i]);
    }

    up[i] = '\0';

    if (text[i] != '\0') {
        return CMD_UNKNOWN;
    }

    for (int c = 0; c < CMD_UNKNOWN; c++) {
        if (strcmp(up, names[c]) == 0) {
            return (Command)c;
        }
    }

    return CMD_UNKNOWN;
}

/* Reads one line safely: returns 0 if OK, 1 if the line was too long, -1 at end of input */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        return -1;
    }

    size_t n = strlen(buf);

    if (n > 0 && buf[n - 1] == '\n') {
        buf[n - 1] = '\0';
        return 0;
    }

    int c = getchar();

    if (c == EOF || c == '\n') {
        return 0;
    }

    while (c != '\n' && c != EOF) {
        c = getchar();
    }

    return 1;
}

/* Reads a positive task ID */
static int read_id(long *id)
{
    char buf[64];
    char *end;

    printf("Task ID: ");

    if (read_line(buf, sizeof buf) != 0) {
        return -1;
    }

    *id = strtol(buf, &end, 10);

    return (end == buf || *end != '\0' || *id <= 0 || *id > MAX_ID) ? -1 : 0;
}

/* Returns the index of a task ID, or -1 if it is not stored */
static int find(const List *list, int id)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].id == id) {
            return (int)i;
        }
    }

    return -1;
}

/* Asks for a task ID and returns its index, or -1 */
static int ask_index(const List *list)
{
    long id;

    if (read_id(&id) != 0) {
        printf("Invalid task ID.\n");
        return -1;
    }

    int i = find(list, (int)id);

    if (i < 0) {
        printf("Task %ld does not exist.\n", id);
    }

    return i;
}

/* Reads a non-empty description (spaces allowed) */
static int read_desc(char *desc)
{
    printf("Description: ");

    return (read_line(desc, DESC_SIZE) != 0 || desc[0] == '\0') ? -1 : 0;
}

/* Prints one task */
static void show(const Task *t)
{
    printf("Task %d [%s] %s\n", t->id, t->status == DONE ? "DONE" : "PENDING", t->desc);
}

/* Adds a task and grows the array with realloc when it is full */
static void add_task(List *list)
{
    Task t;
    long id;

    if (read_id(&id) != 0) {
        printf("Invalid task ID.\n");
        return;
    }

    if (find(list, (int)id) >= 0) {
        printf("Task %ld already exists.\n", id);
        return;
    }

    t.id = (int)id;
    t.status = PENDING;

    if (read_desc(t.desc) != 0) {
        printf("Invalid description.\n");
        return;
    }

    if (list->count == list->capacity) {
        Task *bigger = realloc(list->data, list->capacity * 2 * sizeof(Task));

        if (bigger == NULL) {
            printf("Cannot grow the list.\n");
            return;
        }

        list->data = bigger;
        list->capacity *= 2;
    }

    list->data[list->count++] = t;
    printf("Task %d added.\n", t.id);
}

/* Deletes a task and shifts the later ones left */
static void delete_task(List *list)
{
    int i = ask_index(list);

    if (i < 0) {
        return;
    }

    printf("Task %d deleted.\n", list->data[i].id);
    memmove(&list->data[i], &list->data[i + 1], (list->count - (size_t)i - 1) * sizeof(Task));
    list->count--;
}

/* Changes the description of an existing task */
static void update_task(List *list)
{
    char desc[DESC_SIZE];
    int i = ask_index(list);

    if (i < 0) {
        return;
    }

    if (read_desc(desc) != 0) {
        printf("Invalid description.\n");
        return;
    }

    strcpy(list->data[i].desc, desc);
    printf("Task %d updated.\n", list->data[i].id);
}

/* Marks a task as DONE */
static void complete_task(List *list)
{
    int i = ask_index(list);

    if (i >= 0) {
        list->data[i].status = DONE;
        printf("Task %d marked DONE.\n", list->data[i].id);
    }
}

/* Shows one task by ID */
static void find_task(const List *list)
{
    int i = ask_index(list);

    if (i >= 0) {
        show(&list->data[i]);
    }
}

/* Lists every task */
static void view_tasks(const List *list)
{
    if (list->count == 0) {
        printf("The task list is empty.\n");
    }

    for (size_t i = 0; i < list->count; i++) {
        show(&list->data[i]);
    }
}

/* Lists the recognised commands */
static void help(void)
{
    printf("ADD, DELETE, VIEW, UPDATE, COMPLETE, FIND, HELP, EXIT (not case-sensitive)\n");
}

/* Reads commands in a loop; the enum decides which operation runs */
int main(void)
{
    List list = {malloc(2 * sizeof(Task)), 0, 2};
    char cmd[32];
    int r;
    int running = 1;

    if (list.data == NULL) {
        return 1;
    }

    help();

    while (running) {
        printf("\n> ");
        r = read_line(cmd, sizeof cmd);

        if (r < 0) {
            break;
        }

        if (r > 0) {
            printf("Command too long.\n");
            continue;
        }

        switch (parse(cmd)) {
        case CMD_ADD:
            add_task(&list);
            break;
        case CMD_DELETE:
            delete_task(&list);
            break;
        case CMD_VIEW:
            view_tasks(&list);
            break;
        case CMD_UPDATE:
            update_task(&list);
            break;
        case CMD_COMPLETE:
            complete_task(&list);
            break;
        case CMD_FIND:
            find_task(&list);
            break;
        case CMD_HELP:
            help();
            break;
        case CMD_EXIT:
            running = 0;
            break;
        default:
            printf("Unknown command.\n");
            break;
        }
    }

    free(list.data);

    return 0;
}
