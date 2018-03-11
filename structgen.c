#include <stdio.h>
#include <stdlib.h>

#include "clang-c/Index.h"

typedef struct StructData {
    CXCursor cursor;
    int      num_fields;
    CXCursor fields[];
} StructData;

void generateStructFields(StructData *sd) {
    CXType   type = clang_getCursorType(sd->cursor);
    CXString name = clang_getTypeSpelling(type);
    printf("%s (%d)\n", clang_getCString(name), sd->num_fields);
    clang_disposeString(name);
    for (int i = 0; i < sd->num_fields; i++) {
        CXCursor cursor     = sd->fields[i];
        CXType   type       = clang_getCursorType(cursor);
        CXString type_name  = clang_getTypeSpelling(type);
        CXString field_name = clang_getCursorSpelling(cursor);
        printf("  %s %s\n", clang_getCString(type_name), clang_getCString(field_name));
        clang_disposeString(type_name);
        clang_disposeString(field_name);
    }
}

enum CXVisitorResult countStructFields(CXCursor cursor, CXClientData p_num_fields) {
    (*(int *)p_num_fields)++;
    return CXVisit_Continue;
}

enum CXVisitorResult populateStructFields(CXCursor cursor, CXClientData p_struct_data) {
    StructData *sd               = (StructData *)p_struct_data;
    sd->fields[sd->num_fields++] = cursor;
    return CXVisit_Continue;
}

enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData unused) {
    if (cursor.kind != CXCursor_StructDecl) {
        return CXChildVisit_Continue;
    }
    CXType type       = clang_getCursorType(cursor);
    int    num_fields = 0;
    clang_Type_visitFields(type, countStructFields, &num_fields);
    StructData *sd = malloc(sizeof(*sd) + num_fields * sizeof(CXCursor));
    sd->cursor     = cursor;
    sd->num_fields = 0;
    clang_Type_visitFields(type, populateStructFields, sd);
    generateStructFields(sd);
    free(sd);
    return CXChildVisit_Recurse;
}

int main(void) {
    CXIndex           index = clang_createIndex(0, 0);
    CXTranslationUnit unit  = clang_parseTranslationUnit(
        index,
        "example/example.h", NULL, 0,
        NULL, 0,
        CXTranslationUnit_None);
    if (unit == NULL) {
        printf("Unable to parse translation unit.\n");
        return 1;
    }
    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, visitor, NULL);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    return 0;
}
