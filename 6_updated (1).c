/*
 * 6_updated.c - Memory-Efficient Typed Records and Safe Union Access
 *
 * Task: compare a structure and a union that declare the same members (an int,
 * a float and a 20-byte char array), then build a tagged record where an enum
 * says which union member is currently valid, so the wrong member is never read.
 *
 * Shows: size and alignment of each type, member addresses, structure offsets
 * (offsetof), memory for 100 records of each type, five mixed tagged records
 * and a short explanation of the results.
 *
 * Sample output (64-bit Linux, other platforms may differ): StructData is 28
 * bytes, UnionData is 20; offsets 0, 4, 8; 100 structs = 2800 bytes, 100
 * unions = 2000 bytes; records print as INT 10, FLOAT 220.50, TEXT "...".
 *
 * Edge cases tested: 19-character string (accepted), 20-character string
 * (rejected, no room for the null terminator), one record switching
 * int -> text -> float, invalid tag, sizes that depend on the platform.
 *
 * Compile: gcc -std=c11 -Wall -Wextra -Wpedantic 6_updated.c -o memory_demo
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int i;
    float f;
    char s[20];
} StructData;

typedef union {
    int i;
    float f;
    char s[20];
} UnionData;

typedef enum { VALUE_INT, VALUE_FLOAT, VALUE_TEXT } ValueType;

typedef struct {
    ValueType type;
    UnionData data;
} Tagged;

/* Stores an int and sets the tag */
static int set_int(Tagged *t, int v)
{
    if (t == NULL) {
        return -1;
    }

    t->type = VALUE_INT;
    t->data.i = v;

    return 0;
}

/* Stores a float and sets the tag */
static int set_float(Tagged *t, float v)
{
    if (t == NULL) {
        return -1;
    }

    t->type = VALUE_FLOAT;
    t->data.f = v;

    return 0;
}

/* Stores a string only if it fits in 20 bytes including the null terminator */
static int set_text(Tagged *t, const char *text)
{
    if (t == NULL || text == NULL || strlen(text) >= sizeof t->data.s) {
        return -1;
    }

    memset(t->data.s, 0, sizeof t->data.s);
    strcpy(t->data.s, text);
    t->type = VALUE_TEXT;

    return 0;
}

/* Prints only the member that the tag says is active */
static void show(const Tagged *t)
{
    switch (t->type) {
    case VALUE_INT:
        printf("INT %d\n", t->data.i);
        break;
    case VALUE_FLOAT:
        printf("FLOAT %.2f\n", (double)t->data.f);
        break;
    case VALUE_TEXT:
        printf("TEXT \"%s\"\n", t->data.s);
        break;
    default:
        printf("Invalid tag, nothing read\n");
        break;
    }
}

/* Prints the layout analysis, then demonstrates the tagged records */
int main(void)
{
    StructData st;
    UnionData un;
    Tagged rec[5];
    Tagged t;

    printf("StructData: size %zu, align %zu\n", sizeof(StructData), (size_t)_Alignof(StructData));
    printf("UnionData:  size %zu, align %zu\n", sizeof(UnionData), (size_t)_Alignof(UnionData));
    printf("Struct offsets: i=%zu f=%zu s=%zu\n", offsetof(StructData, i),
           offsetof(StructData, f), offsetof(StructData, s));
    printf("Struct addresses: %p %p %p\n", (void *)&st.i, (void *)&st.f, (void *)st.s);
    printf("Union addresses:  %p %p %p\n", (void *)&un.i, (void *)&un.f, (void *)un.s);
    printf("100 structs = %zu bytes, 100 unions = %zu bytes\n", 100 * sizeof(StructData),
           100 * sizeof(UnionData));

    un.i = 10;
    printf("union int: %d\n", un.i);
    un.f = 220.5f;
    printf("union float: %.1f\n", (double)un.f);
    strcpy(un.s, "Union");
    printf("union text: %s\n", un.s);

    set_int(&rec[0], 10);
    set_float(&rec[1], 220.5f);
    set_text(&rec[2], "Union Programming");
    set_int(&rec[3], 42);
    set_text(&rec[4], "Short text");

    for (int i = 0; i < 5; i++) {
        printf("Record %d: ", i + 1);
        show(&rec[i]);
    }

    printf("Tagged size %zu\n", sizeof(Tagged));

    printf("19 chars: %s\n", set_text(&t, "1234567890123456789") == 0 ? "accepted" : "rejected");
    printf("20 chars: %s\n", set_text(&t, "12345678901234567890") == 0 ? "accepted" : "rejected");

    set_int(&t, 7);
    show(&t);
    set_text(&t, "now text");
    show(&t);
    set_float(&t, 1.5f);
    show(&t);

    memset(&t, 0, sizeof t);
    t.type = (ValueType)99;
    show(&t);

    printf("A union shares one block of memory, so it saves space when a record holds\n");
    printf("one of several types at a time, but its values cannot coexist.\n");
    printf("The tag tells us which member is valid, so we never read stale bytes.\n");

    return 0;
}
