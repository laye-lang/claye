#include "config.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ODIR "out"

#define OUT_LIBCLAYEC_PATH "libclayec"
#define OUT_CLAYEC_PATH    "clayec"

#define LIBCLAYE_STATIC_LIBRARY_FILE "libclaye"
#define CLAYE_EXECUTABLE_FILE        "claye"
#define CLAYEC_EXECUTABLE_FILE       "clayec"

#if defined(NOBCONFIG_MISSING)
#    error No nob configuration has been specified. Please copy the relevant config file from the config directory for your platform and toolchain into the appropriate 'nob_config.<PLATFORM>.h' file.
#endif

#define NOB_IMPLEMENTATION
#include "nob.h"

static struct {
    const char* source_file;
    const char* object_file;
} libclaye_files[] = {
    {"lib/eval.c", ODIR "/eval.o"},
    {0},
};

static struct {
    const char* source_file;
    const char* object_file;
} clayec_files[] = {
    {"src/clayec.c", ODIR "/clayec.o"},
    {0},
};

static struct {
    const char* source_file;
    const char* object_file;
} claye_files[] = {
    {"src/claye.c", ODIR "/claye.o"},
    {0},
};

static bool compile_object(const char* source_path, const char* object_path, const char* source_root) {
    bool result = true;

    Nob_Cmd cmd = {0};
    if (0 == nob_needs_rebuild1(object_path, source_path)) {
        nob_return_defer(true);
    }

    nob_cmd_append(&cmd, CC);
#if defined(CC_MSVC)
    nob_cmd_append(&cmd, "/nologo");
    nob_cmd_append(&cmd, "/c", source_path);
    nob_cmd_append(&cmd, nob_temp_sprintf("/Fo%s", object_path));
    nob_cmd_append(&cmd, nob_temp_sprintf("/I%s/include", source_root));
#else // !CC_MSVC
    nob_cmd_append(&cmd, "-c", source_path);
    nob_cmd_append(&cmd, "-o", object_path);
    nob_cmd_append(&cmd, nob_temp_sprintf("-I%s/include", source_root));
#endif
    nob_cmd_append(&cmd, "" CFLAGS "");

    if (!nob_cmd_run_sync(cmd)) {
        nob_return_defer(false);
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}

static bool package_library(Nob_File_Paths object_files, const char* library_path) {
    bool result = true;

    Nob_Cmd cmd = {0};
    if (0 == nob_needs_rebuild(library_path, object_files.items, object_files.count)) {
        nob_return_defer(true);
    }

    nob_cmd_append(&cmd, LIB);
#if defined(LIB_MSVC)
    nob_cmd_append(&cmd, "/nologo");
    nob_cmd_append(&cmd, nob_temp_sprintf("/out:%s", library_path));
#else // !LIB_MSVC
    nob_cmd_append(&cmd, "rcs", library_path);
#endif
    nob_da_append_many(&cmd, object_files.items, object_files.count);

    if (!nob_cmd_run_sync(cmd)) {
        nob_return_defer(false);
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}

static bool link_executable(Nob_File_Paths input_paths, const char* executable_path) {
    bool result = true;

    Nob_Cmd cmd = {0};
    if (0 == nob_needs_rebuild(executable_path, input_paths.items, input_paths.count)) {
        nob_return_defer(true);
    }

    nob_cmd_append(&cmd, LD);
#if defined(LD_MSVC)
    nob_cmd_append(&cmd, "/nologo", "/subsystem:console");
    nob_cmd_append(&cmd, nob_temp_sprintf("/out:%s", executable_path));
    nob_cmd_append(&cmd, nob_temp_sprintf("/pdb:%s.pdb", executable_path));
#else // !LD_MSVC
    nob_cmd_append(&cmd, "-o", executable_path);
#endif
    nob_cmd_append(&cmd, "" LDFLAGS "");
    nob_da_append_many(&cmd, input_paths.items, input_paths.count);

    if (!nob_cmd_run_sync(cmd)) {
        nob_return_defer(false);
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}

static void clean(void) {
    if (nob_file_exists("./claye")) remove("./claye");
    if (nob_file_exists("./claye.exe")) remove("./claye.exe");

    if (nob_file_exists("./clayec")) remove("./clayec");
    if (nob_file_exists("./clayec.exe")) remove("./clayec.exe");

    Nob_File_Paths outs = {0};
    nob_read_entire_dir(ODIR, &outs);
    for (size_t i = 2; i < outs.count; i++) {
        remove(nob_temp_sprintf(ODIR "/%s", outs.items[i]));
    }

    remove(ODIR);
}

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    bool result = 0;

    const char* program_name = nob_shift_args(&argc, &argv);

    if (argc > 0) {
        const char* arg = nob_shift_args(&argc, &argv);
        if (0 == strcmp(arg, "clean")) {
            clean();
            nob_return_defer(0);
        }
    }

    if (!nob_mkdir_if_not_exists(ODIR)) {
        nob_return_defer(1);
    }

    const char* source_root = ".";
    while (!nob_file_exists(nob_temp_sprintf("%s/include", source_root))) {
        source_root = nob_temp_sprintf("%s/..", source_root);
    }

    Nob_File_Paths libclaye_object_paths = {0};
    for (int64_t i = 0; libclaye_files[i].source_file != 0; i++) {
        const char* source_file = nob_temp_sprintf("%s/%s", source_root, libclaye_files[i].source_file);
        if (!compile_object(source_file, libclaye_files[i].object_file, source_root)) {
            nob_return_defer(1);
        }

        nob_da_append(&libclaye_object_paths, libclaye_files[i].object_file);
    }

    const char* libfile = ODIR "/" LIBCLAYE_STATIC_LIBRARY_FILE LIB_EXT;
    if (!package_library(libclaye_object_paths, libfile)) {
        nob_return_defer(1);
    }

    Nob_File_Paths clayec_input_paths = {0};
    for (int64_t i = 0; clayec_files[i].source_file != 0; i++) {
        const char* source_file = nob_temp_sprintf("%s/%s", source_root, clayec_files[i].source_file);
        if (!compile_object(source_file, clayec_files[i].object_file, source_root)) {
            nob_return_defer(1);
        }

        nob_da_append(&clayec_input_paths, clayec_files[i].object_file);
    }

    nob_da_append(&clayec_input_paths, libfile);
    const char* clayecfile = ODIR "/" CLAYEC_EXECUTABLE_FILE EXE_EXT;
    if (!link_executable(clayec_input_paths, clayecfile)) {
        nob_return_defer(1);
    }

    Nob_File_Paths claye_input_paths = {0};
    for (int64_t i = 0; claye_files[i].source_file != 0; i++) {
        const char* source_file = nob_temp_sprintf("%s/%s", source_root, claye_files[i].source_file);
        if (!compile_object(source_file, claye_files[i].object_file, source_root)) {
            nob_return_defer(1);
        }

        nob_da_append(&claye_input_paths, claye_files[i].object_file);
    }

    nob_da_append(&claye_input_paths, libfile);
    const char* clayefile = ODIR "/" CLAYE_EXECUTABLE_FILE EXE_EXT;
    if (!link_executable(claye_input_paths, clayefile)) {
        nob_return_defer(1);
    }

    if (!nob_copy_file(clayecfile, CLAYEC_EXECUTABLE_FILE)) {
        nob_return_defer(1);
    }

    if (!nob_copy_file(clayefile, CLAYE_EXECUTABLE_FILE)) {
        nob_return_defer(1);
    }

defer:;
    return result;
}
