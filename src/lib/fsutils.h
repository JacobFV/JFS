#ifndef FSUTILS_HEADER
#define FSUTILS_HEADER 1

#include "def.h"

USER user_from_uname(char* uname, VCB vcb);

char** separate_path_list(char* path_list);
// separates /usr/bin//second/path//third/path into
// /usr/bin, /second/path, and /third/path

# endif /* FSUTILS_HEADER */