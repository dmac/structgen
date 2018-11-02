#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clang-c/Index.h"
#include "example/example.h"

typedef struct StructData {
    CXCursor cursor;
    int      num_fields;
    CXCursor fields[];
} StructData;

const char *getTypeFormatter(enum CXTypeKind kind) {
    switch (kind) {
    case CXType_Bool:
    case CXType_Char_U:
    case CXType_UChar:
    case CXType_Char16:
    case CXType_Char32:
    case CXType_UShort:
    case CXType_UInt:
    case CXType_ULong:
    case CXType_ULongLong:
    case CXType_UInt128:
    case CXType_Char_S:
    case CXType_SChar:
    case CXType_WChar:
        fprintf(stderr, "Unhandled CXTypeKind: %d\n", kind);
        exit(1);
    case CXType_Short:
        return "%d";
    case CXType_Int:
        return "%d";
    case CXType_Long:
        return "%ld";
    case CXType_LongLong:
        return "%lld";
    case CXType_Int128:
        return "%lld";
    case CXType_Float:
        return "%g";
    case CXType_Double:
        return "%g";
    case CXType_LongDouble:
    case CXType_NullPtr:
    case CXType_Overload:
    case CXType_Dependent:
    case CXType_ObjCId:
    case CXType_ObjCClass:
    case CXType_ObjCSel:
    case CXType_Float128:
    case CXType_Half:
    case CXType_Float16:
    case CXType_Complex:
        fprintf(stderr, "Unhandled CXTypeKind: %d\n", kind);
        exit(1);
    case CXType_Pointer:
        return "0x%lx";
    case CXType_BlockPointer:
    case CXType_LValueReference:
    case CXType_RValueReference:
    case CXType_Record:
    case CXType_Enum:
    case CXType_Typedef:
    case CXType_ObjCInterface:
    case CXType_ObjCObjectPointer:
    case CXType_FunctionNoProto:
    case CXType_FunctionProto:
    case CXType_ConstantArray:
    case CXType_Vector:
    case CXType_IncompleteArray:
    case CXType_VariableArray:
    case CXType_DependentSizedArray:
    case CXType_MemberPointer:
    case CXType_Auto:
        fprintf(stderr, "Unhandled CXTypeKind: %d\n", kind);
        exit(1);
    case CXType_Elaborated:
        return "%s";
    default:
        fprintf(stderr, "Unhandled CXTypeKind: %d\n", kind);
        exit(1);
    }
    return "";
}

const char *getShortname(CXString s) {
    const char *shortname = clang_getCString(s);
    for (const char *rest = shortname; *rest != '\0'; rest++) {
        if (*rest == ' ') {
            shortname = rest + 1;
        }
    }
    return shortname;
}

void generateStructString(StructData *sd, FILE *f) {
    CXType   type         = clang_getCursorType(sd->cursor);
    CXString name         = clang_getTypeSpelling(type);
    const char *typename  = clang_getCString(name);
    const char *shortname = getShortname(name);

    // function signature
    fprintf(f, "char *%s_string(%s *this) {\n", shortname, typename);

    // format string
    fprintf(f, "    char *format = \"%s{", shortname);
    for (int i = 0; i < sd->num_fields; i++) {
        CXType   type = clang_getCursorType(sd->fields[i]);
        CXString name = clang_getCursorSpelling(sd->fields[i]);
        fprintf(f, "%s=%s", clang_getCString(name), getTypeFormatter(type.kind));
        if (i < sd->num_fields - 1) {
            fprintf(f, " ");
        }
        clang_disposeString(name);
    }
    fprintf(f, "}\";\n");

    // create elaborated type strings
    for (int i = 0; i < sd->num_fields; i++) {
        CXType type = clang_getCursorType(sd->fields[i]);
        if (type.kind != CXType_Elaborated) {
            continue;
        }
        CXString fieldname        = clang_getCursorSpelling(sd->fields[i]);
        CXString typename         = clang_getTypeSpelling(type);
        const char *shorttypename = getShortname(typename);
        fprintf(f, "    char *%s_string = %s_string(&this->%s);\n",
                clang_getCString(fieldname), shorttypename, clang_getCString(fieldname));
        clang_disposeString(fieldname);
        clang_disposeString(typename);
    }

    // string size
    fprintf(f, "    int size = snprintf(NULL, 0, format, ");
    for (int i = 0; i < sd->num_fields; i++) {
        CXType   type = clang_getCursorType(sd->fields[i]);
        CXString name = clang_getCursorSpelling(sd->fields[i]);
        if (type.kind == CXType_Elaborated) {
            fprintf(f, "%s_string", clang_getCString(name));
        } else {
            fprintf(f, "this->%s", clang_getCString(name));
        }
        if (i < sd->num_fields - 1) {
            fprintf(f, ", ");
        }
        clang_disposeString(name);
    }
    fprintf(f, ");\n");

    // allocate string
    fprintf(f, "    char *s = malloc(size + 1);\n");

    // populate string
    fprintf(f, "    snprintf(s, size + 1, format, ");
    for (int i = 0; i < sd->num_fields; i++) {
        CXType   type = clang_getCursorType(sd->fields[i]);
        CXString name = clang_getCursorSpelling(sd->fields[i]);
        if (type.kind == CXType_Elaborated) {
            fprintf(f, "%s_string", clang_getCString(name));
        } else {
            fprintf(f, "this->%s", clang_getCString(name));
        }
        if (i < sd->num_fields - 1) {
            fprintf(f, ", ");
        }
        clang_disposeString(name);
    }
    fprintf(f, ");\n");

    // free elaborated type strings
    for (int i = 0; i < sd->num_fields; i++) {
        CXType type = clang_getCursorType(sd->fields[i]);
        if (type.kind != CXType_Elaborated) {
            continue;
        }
        CXString fieldname = clang_getCursorSpelling(sd->fields[i]);
        fprintf(f, "    free(%s_string);\n", clang_getCString(fieldname));
        clang_disposeString(fieldname);
    }

    fprintf(f, "    return s;\n");
    fprintf(f, "}\n\n");

    clang_disposeString(name);
}

void generateStructFunctions(StructData *sd, FILE *f) {
    generateStructString(sd, f);
}

enum CXVisitorResult countStructFields(CXCursor cursor, CXClientData num_fields) {
    (*(int *)num_fields)++;
    return CXVisit_Continue;
}

enum CXVisitorResult populateStructFields(CXCursor cursor, CXClientData struct_data) {
    StructData *sd               = (StructData *)struct_data;
    sd->fields[sd->num_fields++] = cursor;
    return CXVisit_Continue;
}

enum CXChildVisitResult visitor(CXCursor cursor, CXCursor parent, CXClientData output_file) {
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
    generateStructFunctions(sd, (FILE *)output_file);
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
    clang_visitChildren(cursor, visitor, stdout);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    struct Entity e = {
        .x      = 1.2,
        .y      = 3.4,
        .health = 5,
    };
    char *entity_string = Entity_string(&e);
    printf("%s\n", entity_string);

    Player p = {
        .entity = e,
        .name   = "alice",
    };
    char *player_string = Player_string(&p);
    printf("%s\n", player_string);

    free(entity_string);
    free(player_string);

    return 0;
}
