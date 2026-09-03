#include "module.h"

ModuleLoader* module_loader_create(void) {
    ModuleLoader *loader = (ModuleLoader *)calloc(1, sizeof(ModuleLoader));
    if (!loader) return NULL;
    
    loader->modules = NULL;
    loader->count = 0;
    loader->capacity = 0;
    loader->search_paths = NULL;
    loader->path_count = 0;
    
    // Agregar ruta por defecto
    module_add_search_path(loader, ".");
    module_add_search_path(loader, "./modules");
    module_add_search_path(loader, "/usr/local/lib/mano");
    
    return loader;
}

void module_loader_destroy(ModuleLoader *loader) {
    if (!loader) return;
    
    for (size_t i = 0; i < loader->count; i++) {
        if (loader->modules[i]) {
            if (loader->modules[i]->name) free(loader->modules[i]->name);
            if (loader->modules[i]->path) free(loader->modules[i]->path);
            if (loader->modules[i]->ast) ast_destroy(loader->modules[i]->ast);
            free(loader->modules[i]);
        }
    }
    
    free(loader->modules);
    
    for (size_t i = 0; i < loader->path_count; i++) {
        free(loader->search_paths[i]);
    }
    free(loader->search_paths);
    
    free(loader);
}

Module* module_load(ModuleLoader *loader, const char *name) {
    if (!loader || !name) return NULL;
    
    // Verificar si ya está cargado
    for (size_t i = 0; i < loader->count; i++) {
        if (loader->modules[i] && strcmp(loader->modules[i]->name, name) == 0) {
            return loader->modules[i];
        }
    }
    
    // Buscar el archivo en los paths
    char filename[512];
    for (size_t i = 0; i < loader->path_count; i++) {
        snprintf(filename, sizeof(filename), "%s/%s.mano", 
                 loader->search_paths[i], name);
        
        FILE *f = fopen(filename, "r");
        if (f) {
            fclose(f);
            
            // Crear módulo
            Module *module = (Module *)calloc(1, sizeof(Module));
            if (!module) return NULL;
            
            module->name = strdup(name);
            module->path = strdup(filename);
            module->ast = NULL;
            module->imports = NULL;
            module->import_count = 0;
            
            // Agregar a la lista
            if (loader->count >= loader->capacity) {
                size_t new_capacity = loader->capacity == 0 ? 8 : loader->capacity * 2;
                Module **new_modules = (Module **)realloc(loader->modules, 
                                                          new_capacity * sizeof(Module *));
                if (!new_modules) {
                    free(module->name);
                    free(module->path);
                    free(module);
                    return NULL;
                }
                loader->modules = new_modules;
                loader->capacity = new_capacity;
            }
            
            loader->modules[loader->count++] = module;
            return module;
        }
    }
    
    return NULL;
}

bool module_add_search_path(ModuleLoader *loader, const char *path) {
    if (!loader || !path) return false;
    
    if (loader->path_count >= 32) return false; // Límite de seguridad
    
    char *new_path = strdup(path);
    if (!new_path) return false;
    
    loader->search_paths = (char **)realloc(loader->search_paths, 
                                            (loader->path_count + 1) * sizeof(char *));
    if (!loader->search_paths) {
        free(new_path);
        return false;
    }
    
    loader->search_paths[loader->path_count++] = new_path;
    return true;
}

void module_loader_print(ModuleLoader *loader) {
    if (!loader) return;
    
    printf("Module Loader:\n");
    printf("  Módulos cargados: %zu\n", loader->count);
    for (size_t i = 0; i < loader->count; i++) {
        if (loader->modules[i]) {
            printf("    - %s (%s)\n", 
                   loader->modules[i]->name,
                   loader->modules[i]->path);
        }
    }
    printf("  Paths de búsqueda: %zu\n", loader->path_count);
    for (size_t i = 0; i < loader->path_count; i++) {
        printf("    - %s\n", loader->search_paths[i]);
    }
}
