/*
 * 10_updated.c - Safe Heterogeneous Data Container Library
 *
 * Task: a container that holds exactly ONE value at a time (int, float, char
 * or string). A union stores the value and an enum tag remembers its type.
 * Strings are copied onto the heap, so the container owns them and frees them.
 *
 * Safety rules: a new string is copied before the old one is freed (a failed
 * malloc keeps the old value); getters return success / failure separately
 * from the value, so 0 is never an error marker; clearing twice and copying a
 * container onto itself are safe; no non-standard strdup is used.
 *
 * Sample output: [0] INT 10, [1] FLOAT 3.14, [2] CHAR 'A', [3] STRING "Hello",
 *   [4] INT 0, [5] STRING "Data"; the copy of [5] keeps "Data" after the
 *   original is changed to "Changed".
 *
 * Edge cases tested: integer zero, wrong-type getter, empty container, string
 *   replaced by its own pointer, STRING -> INT -> STRING -> FLOAT transitions,
 *   repeated clear, deep copy, self-copy.
 *
 * Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 10_updated.c -o container
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { TYPE_EMPTY, TYPE_INT, TYPE_FLOAT, TYPE_CHAR, TYPE_STRING } DataType;

typedef union {
    int i;
    float f;
    char c;
    char *s;
} Data;

typedef struct {
    DataType type;
    Data data;
} Container;

/* Puts the container into a valid empty state */
void container_init(Container *c)
{
    c->type = TYPE_EMPTY;
    memset(&c->data, 0, sizeof c->data);
}

/* Frees an owned string and resets to empty (safe to repeat) */
void container_clear(Container *c)
{
    if (c->type == TYPE_STRING) {
        free(c->data.s);
    }

    container_init(c);
}

/* Setters replace the current value and the type tag */
int container_set_int(Container *c, int v)
{
    if (c == NULL) {
        return -1;
    }

    container_clear(c);
    c->type = TYPE_INT;
    c->data.i = v;

    return 0;
}

int container_set_float(Container *c, float v)
{
    if (c == NULL) {
        return -1;
    }

    container_clear(c);
    c->type = TYPE_FLOAT;
    c->data.f = v;

    return 0;
}

int container_set_char(Container *c, char v)
{
    if (c == NULL) {
        return -1;
    }

    container_clear(c);
    c->type = TYPE_CHAR;
    c->data.c = v;

    return 0;
}

/* The copy is made first, so a failed malloc keeps the old value. */
int container_set_string(Container *c, const char *text)
{
    if (c == NULL || text == NULL) {
        return -1;
    }

    size_t size = strlen(text) + 1;
    char *copy = malloc(size);

    if (copy == NULL) {
        return -1;
    }

    memcpy(copy, text, size);
    container_clear(c);
    c->type = TYPE_STRING;
    c->data.s = copy;

    return 0;
}

/* Getters return 0 on success, -1 on wrong type; the value comes back through out */
int container_get_int(const Container *c, int *out)
{
    if (c == NULL || out == NULL || c->type != TYPE_INT) {
        return -1;
    }

    *out = c->data.i;

    return 0;
}

int container_get_float(const Container *c, float *out)
{
    if (c == NULL || out == NULL || c->type != TYPE_FLOAT) {
        return -1;
    }

    *out = c->data.f;

    return 0;
}

int container_get_char(const Container *c, char *out)
{
    if (c == NULL || out == NULL || c->type != TYPE_CHAR) {
        return -1;
    }

    *out = c->data.c;

    return 0;
}

int container_get_string(const Container *c, const char **out)
{
    if (c == NULL || out == NULL || c->type != TYPE_STRING) {
        return -1;
    }

    *out = c->data.s;

    return 0;
}

/* Deep copy: strings get their own buffer; copying onto itself does nothing */
int container_copy(Container *dst, const Container *src)
{
    if (dst == NULL || src == NULL) {
        return -1;
    }

    if (dst == src) {
        return 0;
    }

    if (src->type == TYPE_STRING) {
        return container_set_string(dst, src->data.s);
    }

    container_clear(dst);
    dst->type = src->type;
    dst->data = src->data;

    return 0;
}

/* Returns a readable label for a type */
const char *container_type_name(DataType type)
{
    static const char *names[] = {"EMPTY", "INT", "FLOAT", "CHAR", "STRING"};

    return (type >= TYPE_EMPTY && type <= TYPE_STRING) ? names[type] : "UNKNOWN";
}

/* Prints the value according to its active type */
void container_print(const Container *c)
{
    switch (c->type) {
    case TYPE_INT:
        printf("%d\n", c->data.i);
        break;
    case TYPE_FLOAT:
        printf("%.2f\n", (double)c->data.f);
        break;
    case TYPE_CHAR:
        printf("'%c'\n", c->data.c);
        break;
    case TYPE_STRING:
        printf("\"%s\"\n", c->data.s);
        break;
    default:
        printf("(empty)\n");
        break;
    }
}

/* Demonstrates an array of containers, a deep copy and type transitions */
int main(void)
{
    Container *items = malloc(6 * sizeof(Container));
    Container copy;
    Container box;
    int number = -1;
    float decimal = -1;
    char letter = '?';
    const char *text = NULL;

    if (items == NULL) {
        return 1;
    }

    for (int i = 0; i < 6; i++) {
        container_init(&items[i]);
    }

    container_set_int(&items[0], 10);
    container_set_float(&items[1], 3.14f);
    container_set_char(&items[2], 'A');
    container_set_string(&items[3], "Hello");
    container_set_int(&items[4], 0);
    container_set_string(&items[5], "Data");

    for (int i = 0; i < 6; i++) {
        printf("[%d] %-6s ", i, container_type_name(items[i].type));
        container_print(&items[i]);
    }

    container_init(&copy);
    container_copy(&copy, &items[5]);
    container_copy(&copy, &copy);
    container_set_string(&items[5], "Changed");
    printf("original: ");
    container_print(&items[5]);
    printf("copy:     ");
    container_print(&copy);

    container_init(&box);
    container_set_string(&box, "first");
    container_set_int(&box, 99);
    container_set_string(&box, "second");
    container_get_string(&box, &text);
    container_set_string(&box, text);
    container_set_float(&box, 2.5f);
    printf("%s ", container_type_name(box.type));
    container_print(&box);

    printf("int getter on float: %s\n", container_get_int(&box, &number) == 0 ? "ok" : "refused");
    container_get_float(&box, &decimal);
    container_get_char(&items[2], &letter);
    container_get_int(&items[4], &number);
    printf("float %.2f, char %c, int zero %d\n", (double)decimal, letter, number);

    container_clear(&box);
    container_clear(&box);
    container_clear(&copy);

    for (int i = 0; i < 6; i++) {
        container_clear(&items[i]);
    }

    free(items);

    return 0;
}
