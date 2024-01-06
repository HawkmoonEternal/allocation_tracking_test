#include "pmix.h"
#include "pmix_server.h"
#include <pmix_tool.h>

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_JOIN_COMPAT(a, b) \
        pmix_argv_join(a, b)
#else
#define PMIX_ARGV_JOIN_COMPAT(a, b) \
        PMIx_Argv_join(a, b)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_SPLIT_COMPAT(a, b) \
        pmix_argv_split(a, b)
#else
#define PMIX_ARGV_SPLIT_COMPAT(a, b) \
        PMIx_Argv_split(a, b)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_SPLIT_WITH_EMPTY_COMPAT(a, b) \
        pmix_argv_split_with_empty(a, b)
#else
#define PMIX_ARGV_SPLIT_WITH_EMPTY_COMPAT(a, b) \
        PMIx_Argv_split_with_empty(a, b)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_COUNT_COMPAT(a) \
        pmix_argv_count(a)
#else
#define PMIX_ARGV_COUNT_COMPAT(a) \
        PMIx_Argv_count(a)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_FREE_COMPAT(a) \
        pmix_argv_free(a)
#else
#define PMIX_ARGV_FREE_COMPAT(a) \
        PMIx_Argv_free(a)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_APPEND_UNIQUE_COMPAT(a, b) \
        pmix_argv_append_unique_nosize(a, b)
#else
#define PMIX_ARGV_APPEND_UNIQUE_COMPAT(a, b) \
        PMIx_Argv_append_unique_nosize(a, b)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_APPEND_NOSIZE_COMPAT(a, b) \
        pmix_argv_append_nosize(a, b)
#else
#define PMIX_ARGV_APPEND_NOSIZE_COMPAT(a, b) \
        PMIx_Argv_append_nosize(a, b)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_ARGV_COPY_COMPAT(a) \
        pmix_argv_copy(a)
#else
#define PMIX_ARGV_COPY_COMPAT(a) \
        PMIx_Argv_copy(a)
#endif

#if PMIX_NUMERIC_VERSION < 0x00040203
#define PMIX_SETENV_COMPAT(a, b, c, d) \
        pmix_setenv(a, b, c, d)
#else
#define PMIX_SETENV_COMPAT(a, b, c, d) \
        PMIx_Setenv(a, b, c, d)
#endif