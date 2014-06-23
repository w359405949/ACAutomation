#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
    extern "C" {
#endif

/*
 * create an ACAutoMation content,
 * which record the trie pointer, current state(pointer) and so on.
 * return NULL when failed.
 */
struct ACAutoMation *ac_create();

/*
 * add pattern in mation content, always success.
 */
void ac_add(struct ACAutoMation *mation, const char *pattern, size_t length);

/*
 * destroy conent, always success.
 */
void ac_destroy(struct ACAutoMation *mation);

/*
 * match str, return how many patterns have been found.
 */
int32_t ac_match(struct ACAutoMation *mation, const char *str, size_t length);

#ifdef __cplusplus
    }
#endif
