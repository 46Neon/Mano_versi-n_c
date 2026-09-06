#ifndef MILENA_MODULE_H
#define MILENA_MODULE_H

#include "common.h"
#include "ast.h"

typedef struct Module {
    char *name;
    char *path;
    ASTNode *ast;
    struct Module *imports;
    size_t import_count;
} Module;

typedef struct ModuleLoader {
    Module **modules;
    size_t count;
    size_t capacity;
    char **search_paths;
    size_t path_count;
} ModuleLoader;

ModuleLoader* module_loader_create(void);
void module_loader_destroy(ModuleLoader *loader);
Module* module_load(ModuleLoader *loader, const char *name);
bool module_add_search_path(ModuleLoader *loader, const char *path);
void module_loader_print(ModuleLoader *loader);

#endif
