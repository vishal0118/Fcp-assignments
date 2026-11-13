/*
 * 5_updated.c - Rectangle Inventory and Geometry Analysis
 *
 * Task: interactive program that stores rectangles (unique ID, width, height,
 * area, perimeter) in a dynamic array. Supports add, display, search, update,
 * delete, statistics (largest / smallest / total / average area), sorting by
 * area and filtering by an area threshold. Area and perimeter are recomputed
 * whenever the dimensions change.
 *
 * Rules: 1 to 1000 rectangles; dimensions must be finite and greater than 0;
 * a rectangle is a "square" when its sides differ by at most 0.0001 (we never
 * compare floats with ==); sorting works on a copy.
 *
 * Sample run: capacity 3 -> add 201 (5 x 4), 202 (6.5 x 3.5), 203 (4 x 4) ->
 *   sorted by area: 203 (16.00), 201 (20.00), 202 (22.75) -> 203 is a square ->
 *   update 201 to 6 x 6 -> total area 74.75, average area 24.92.
 *
 * Edge cases tested: zero / negative / NaN / infinite dimensions, duplicate ID,
 *   single rectangle, equal areas, square with non-integer sides (2.5 x 2.5),
 *   empty list after deleting everything, array growth.
 *
 * Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 5_updated.c -o rectangles
 */

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RECTS 1000
#define EPSILON 0.0001f
#define MAX_ID 1000000000

typedef struct {
    int id;
    float w;
    float h;
    float area;
    float perim;
} Rect;

typedef struct {
    Rect *data;
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

/* A square has sides that differ by at most EPSILON */
static int is_square(const Rect *r)
{
    return fabsf(r->w - r->h) <= EPSILON;
}

/* Prints one rectangle with its area, perimeter and shape */
static void show(const Rect *r)
{
    printf("ID %d | %.2f x %.2f | area %.2f | perimeter %.2f | %s\n", r->id, (double)r->w,
           (double)r->h, (double)r->area, (double)r->perim, is_square(r) ? "square" : "non-square");
}

/* Reads width and height, then recomputes area and perimeter */
static int read_dimensions(Rect *r)
{
    double w;
    double h;

    if (read_double("Width: ", &w) != 0 || w <= 0 || w > FLT_MAX) {
        return -1;
    }

    if (read_double("Height: ", &h) != 0 || h <= 0 || h > FLT_MAX) {
        return -1;
    }

    r->w = (float)w;
    r->h = (float)h;
    r->area = r->w * r->h;
    r->perim = 2 * (r->w + r->h);

    return (isfinite(r->area) && isfinite(r->perim)) ? 0 : -1;
}

/* Returns the index of an ID, or -1 if it is not stored */
static int find(const List *list, int id)
{
    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].id == id) {
            return (int)i;
        }
    }

    return -1;
}

/* Asks for an ID and returns its index, or -1 */
static int ask_index(const List *list)
{
    long id;

    if (read_int("Rectangle ID: ", &id) != 0) {
        printf("Invalid ID.\n");
        return -1;
    }

    int i = (id > 0 && id <= MAX_ID) ? find(list, (int)id) : -1;

    if (i < 0) {
        printf("ID not found.\n");
    }

    return i;
}

/* Adds a rectangle and grows the array with realloc when it is full */
static void add_rect(List *list)
{
    Rect r;
    long id;

    if (read_int("Rectangle ID: ", &id) != 0 || id <= 0 || id > MAX_ID) {
        printf("Invalid ID.\n");
        return;
    }

    if (find(list, (int)id) >= 0) {
        printf("Duplicate ID.\n");
        return;
    }

    r.id = (int)id;

    if (read_dimensions(&r) != 0) {
        printf("Dimensions must be finite and greater than 0.\n");
        return;
    }

    if (list->count == list->capacity) {
        size_t cap = list->capacity * 2;
        Rect *bigger = cap > MAX_RECTS ? NULL : realloc(list->data, cap * sizeof *bigger);

        if (bigger == NULL) {
            printf("Cannot grow the list.\n");
            return;
        }

        list->data = bigger;
        list->capacity = cap;
    }

    list->data[list->count++] = r;
    printf("Rectangle %d added.\n", r.id);
}

/* Changes the dimensions; area and perimeter stay consistent */
static void update_rect(List *list)
{
    int i = ask_index(list);

    if (i < 0) {
        return;
    }

    Rect r = list->data[i];

    if (read_dimensions(&r) != 0) {
        printf("Dimensions must be finite and greater than 0.\n");
        return;
    }

    list->data[i] = r;
    printf("Rectangle %d updated.\n", r.id);
}

/* Deletes a rectangle and shifts the later ones left */
static void delete_rect(List *list)
{
    int i = ask_index(list);

    if (i < 0) {
        return;
    }

    printf("Rectangle %d deleted.\n", list->data[i].id);
    memmove(&list->data[i], &list->data[i + 1], (list->count - (size_t)i - 1) * sizeof(Rect));
    list->count--;
}

/* Prints the largest and smallest area, plus total and average area */
static void stats(const List *list)
{
    if (list->count == 0) {
        printf("No rectangles.\n");
        return;
    }

    size_t big = 0;
    size_t small = 0;
    double total = 0;

    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].area > list->data[big].area) {
            big = i;
        }

        if (list->data[i].area < list->data[small].area) {
            small = i;
        }

        total += list->data[i].area;
    }

    printf("Largest:  ");
    show(&list->data[big]);
    printf("Smallest: ");
    show(&list->data[small]);
    printf("Total area %.2f | average area %.2f\n", total, total / (double)list->count);
}

/* Sort order: smaller area first, then smaller ID */
static int by_area_then_id(const void *x, const void *y)
{
    const Rect *a = x;
    const Rect *b = y;

    if (a->area != b->area) {
        return a->area < b->area ? -1 : 1;
    }

    return (a->id > b->id) - (a->id < b->id);
}

static void show_sorted(const List *list)
{
    if (list->count == 0) {
        printf("No rectangles.\n");
        return;
    }

    Rect *copy = malloc(list->count * sizeof(Rect));

    if (copy == NULL) {
        printf("Not enough memory.\n");
        return;
    }

    memcpy(copy, list->data, list->count * sizeof(Rect));
    qsort(copy, list->count, sizeof(Rect), by_area_then_id);

    for (size_t i = 0; i < list->count; i++) {
        show(&copy[i]);
    }

    free(copy);
}

/* Shows only rectangles whose area is above a threshold */
static void filter_by_area(const List *list)
{
    double limit;
    int found = 0;

    if (read_double("Show area greater than: ", &limit) != 0) {
        printf("Invalid number.\n");
        return;
    }

    for (size_t i = 0; i < list->count; i++) {
        if (list->data[i].area > limit) {
            show(&list->data[i]);
            found = 1;
        }
    }

    if (!found) {
        printf("No rectangle matches.\n");
    }
}

/* Reads the capacity, then runs the menu until Exit */
int main(void)
{
    List list = {NULL, 0, 0};
    long n = 0;

    while (list.data == NULL) {
        if (read_int("How many rectangles (1-1000)? ", &n) != 0) {
            if (feof(stdin)) {
                return 1;
            }

            printf("Invalid input.\n");
        } else if (n < 1 || n > MAX_RECTS) {
            printf("Choose 1 to %d.\n", MAX_RECTS);
        } else if ((list.data = malloc((size_t)n * sizeof(Rect))) == NULL) {
            printf("Not enough memory.\n");
        }
    }

    list.capacity = (size_t)n;

    for (;;) {
        long choice;

        printf("\n1 Add  2 Show  3 Search  4 Update  5 Delete  6 Stats  7 Sorted  8 Filter  0 Exit\n");

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
            add_rect(&list);
            break;
        case 2:
            if (list.count == 0) {
                printf("No rectangles.\n");
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
            update_rect(&list);
            break;
        case 5:
            delete_rect(&list);
            break;
        case 6:
            stats(&list);
            break;
        case 7:
            show_sorted(&list);
            break;
        case 8:
            filter_by_area(&list);
            break;
        default:
            printf("Invalid choice.\n");
            break;
        }
    }

    free(list.data);

    return 0;
}
