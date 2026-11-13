/*
 * 4_updated.c - Student Records and Academic Analytics System
 *
 * Task: menu-driven program that stores students (roll number, name, age, GPA)
 * in a dynamic array that grows with realloc. Supports add, display, search,
 * update, delete, GPA summary (highest / lowest / average) and a sorted list.
 * The sorted list is built on a copy, so the stored order never changes.
 *
 * Sample run: capacity 2 -> add 101 Aladdin 20 3.5 -> add 102 Batman 21 3.7 ->
 *   add 103 Catwoman 22 3.8 (array grows) -> search 102 -> update 102 to GPA 3.9
 *   -> delete 101 -> sorted: 102 (3.90), 103 (3.80) -> average GPA 3.85.
 *
 * Edge cases tested: empty list, duplicate roll number, invalid age / GPA,
 *   49-character name (accepted), 50-character name (rejected), name with
 *   spaces, missing roll number, reallocation, zero / negative / huge capacity.
 *
 * Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 4_updated.c -o students
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_SIZE 50
#define MAX_CAPACITY 100000
#define MAX_ROLL 1000000000

typedef struct {
    int roll;
    char name[NAME_SIZE];
    int age;
    float gpa;
} Student;

typedef struct {
    Student *data;
    size_t count;
    size_t capacity;
} List;

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

/* Reads a whole number */
static int read_int(const char *prompt, long *value)
{
    char buf[64];
    char *end;

    printf("%s", prompt);

    if (read_line(buf, sizeof buf) != 0) {
        return -1;
    }

    *value = strtol(buf, &end, 10);

    return (end == buf || *end != '\0') ? -1 : 0;
}

/* Reads a finite decimal number */
static int read_double(const char *prompt, double *value)
{
    char buf[64];
    char *end;

    printf("%s", prompt);

    if (read_line(buf, sizeof buf) != 0) {
        return -1;
    }

    *value = strtod(buf, &end);

    return (end == buf || *end != '\0' || !isfinite(*value)) ? -1 : 0;
}

/* Returns the index of a roll number, or -1 if it is not stored */
static int find(const List *list, int roll)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].roll == roll) {
            return (int)i;
        }
    }

    return -1;
}

/* Prints one student */
static void show(const Student *s)
{
    printf("Roll %d | %s | Age %d | GPA %.2f\n", s->roll, s->name, s->age, (double)s->gpa);
}

/* Reads and validates name, age and GPA */
static int read_student(Student *s)
{
    char name[NAME_SIZE + 1];
    long age;
    double gpa;

    printf("Name: ");

    if (read_line(name, sizeof name) != 0 || name[0] == '\0' || strlen(name) >= NAME_SIZE) {
        return -1;
    }

    if (read_int("Age (16-100): ", &age) != 0 || age < 16 || age > 100) {
        return -1;
    }

    if (read_double("GPA (0-4): ", &gpa) != 0 || gpa < 0 || gpa > 4) {
        return -1;
    }

    strcpy(s->name, name);
    s->age = (int)age;
    s->gpa = (float)gpa;

    return 0;
}

/* Asks for a roll number and returns its index, or -1 */
static int ask_index(const List *list)
{
    long roll;

    if (read_int("Roll number: ", &roll) != 0) {
        printf("Invalid roll number.\n");
        return -1;
    }

    int i = (roll > 0 && roll <= MAX_ROLL) ? find(list, (int)roll) : -1;

    if (i < 0) {
        printf("Roll number not found.\n");
    }

    return i;
}

/* Adds a student and grows the array with realloc when it is full */
static void add_student(List *list)
{
    Student s;
    long roll;

    if (read_int("Roll number: ", &roll) != 0 || roll <= 0 || roll > MAX_ROLL) {
        printf("Invalid roll number.\n");
        return;
    }

    if (find(list, (int)roll) >= 0) {
        printf("Duplicate roll number.\n");
        return;
    }

    s.roll = (int)roll;

    if (read_student(&s) != 0) {
        printf("Invalid input.\n");
        return;
    }

    if (list->count == list->capacity) {
        size_t cap = list->capacity * 2;
        Student *bigger = cap > MAX_CAPACITY ? NULL : realloc(list->data, cap * sizeof *bigger);

        if (bigger == NULL) {
            printf("Cannot grow the list.\n");
            return;
        }

        list->data = bigger;
        list->capacity = cap;
    }

    list->data[list->count++] = s;
    printf("Student added.\n");
}

/* Replaces the name, age and GPA of an existing student */
static void update_student(List *list)
{
    int i = ask_index(list);

    if (i < 0) {
        return;
    }

    Student s = list->data[i];

    if (read_student(&s) != 0) {
        printf("Invalid input.\n");
        return;
    }

    list->data[i] = s;
    printf("Student updated.\n");
}

/* Deletes a student and shifts the later records left */
static void delete_student(List *list)
{
    int i = ask_index(list);

    if (i < 0) {
        return;
    }

    memmove(&list->data[i], &list->data[i + 1], (list->count - (size_t)i - 1) * sizeof(Student));
    list->count--;
    printf("Student deleted.\n");
}

/* Prints the highest, lowest and average GPA */
static void summary(const List *list)
{
    if (list->count == 0) {
        printf("No records.\n");
        return;
    }

    size_t high = 0;
    size_t low = 0;
    double total = 0;

    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].gpa > list->data[high].gpa) {
            high = i;
        }

        if (list->data[i].gpa < list->data[low].gpa) {
            low = i;
        }

        total += list->data[i].gpa;
    }

    printf("Highest: ");
    show(&list->data[high]);
    printf("Lowest:  ");
    show(&list->data[low]);
    printf("Average GPA: %.2f\n", total / (double)list->count);
}

/* Sort order: higher GPA first, then smaller roll number */
static int by_gpa_desc(const void *x, const void *y)
{
    const Student *a = x;
    const Student *b = y;

    if (a->gpa != b->gpa) {
        return a->gpa < b->gpa ? 1 : -1;
    }

    return (a->roll > b->roll) - (a->roll < b->roll);
}

/* Sorting works on a copy, so the stored order never changes. */
static void show_sorted(const List *list)
{
    if (list->count == 0) {
        printf("No records.\n");
        return;
    }

    Student *copy = malloc(list->count * sizeof(Student));

    if (copy == NULL) {
        printf("Not enough memory.\n");
        return;
    }

    memcpy(copy, list->data, list->count * sizeof(Student));
    qsort(copy, list->count, sizeof(Student), by_gpa_desc);

    for (size_t i = 0; i < list->count; i++) {
        show(&copy[i]);
    }

    free(copy);
}

/* Reads the capacity, then runs the menu until Exit */
int main(void)
{
    List list = {NULL, 0, 0};
    long cap = 0;

    while (list.data == NULL) {
        if (read_int("Initial capacity: ", &cap) != 0) {
            if (feof(stdin)) {
                return 1;
            }

            printf("Invalid input.\n");
        } else if (cap <= 0 || cap > MAX_CAPACITY) {
            printf("Capacity must be 1 to %d.\n", MAX_CAPACITY);
        } else if ((list.data = malloc((size_t)cap * sizeof(Student))) == NULL) {
            printf("Not enough memory.\n");
        }
    }

    list.capacity = (size_t)cap;

    for (;;) {
        long choice;

        printf("\n1 Add  2 Show  3 Search  4 Update  5 Delete  6 GPA summary  7 Sorted  0 Exit\n");

        if (read_int("Choice: ", &choice) != 0) {
            if (feof(stdin)) {
                break;
            }

            printf("Invalid choice.\n");
            continue;
        }

        if (choice == 0) {
            break;
        }

        switch (choice) {
        case 1:
            add_student(&list);
            break;
        case 2:
            if (list.count == 0) {
                printf("No records.\n");
            }

            for (size_t i = 0; i < list.count; i++) {
                show(&list.data[i]);
            }

            break;
        case 3: {
            int i = ask_index(&list);

            if (i >= 0) {
                show(&list.data[i]);
            }

            break;
        }
        case 4:
            update_student(&list);
            break;
        case 5:
            delete_student(&list);
            break;
        case 6:
            summary(&list);
            break;
        case 7:
            show_sorted(&list);
            break;
        default:
            printf("Invalid choice.\n");
            break;
        }
    }

    free(list.data);

    return 0;
}
