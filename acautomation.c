#include <stdlib.h>
#include <string.h>

#include "acautomation.h"

struct ACAutoMation
{
    struct TrieNode *root; //
    int32_t dirty_node; // new add node count which didn't build the fail pointer.
    int32_t (*get_word_length)(const char *str, size_t length); // the right length of a (chinese) word in (utf-8).

    int32_t word_count;
};

struct TrieNode
{
    struct TrieNode *fail;
    struct TrieNode **children;
    struct TrieNode *next; // queue helper
    char *pattern;

    size_t children_len;

    size_t pattern_len;

    int32_t end;

};

int32_t get_utf_8_word_length(const char *str, size_t length)
{
    if (length < 1) {
        return 0;
    }
    char tmp = str[0];
    if (tmp > 0) {
        return 1;
    }

    int i = 0;
    do {
        tmp <<= 1;
        i++;
    } while (tmp < 0);

    return i > length? length : i;
}

struct ACAutoMation *ac_create()
{
    struct ACAutoMation *mation = (struct ACAutoMation *)malloc(sizeof(struct ACAutoMation));
    if (NULL == mation) {
        return NULL;
    }
    mation->root = (struct TrieNode*)malloc(sizeof(struct TrieNode));
    if (NULL == mation->root) {
        free(mation);
        return NULL;
    }

    mation->root->children = NULL;
    mation->get_word_length = get_utf_8_word_length;
    mation->dirty_node = 0;
    mation->word_count = 0;
    return mation;
}

void destroy_trie_node(struct TrieNode *node)
{
    if (NULL == node) {
        return;
    }
    int32_t i = 0;
    for(; i < node->children_len; i++) {
        destroy_trie_node(node->children[i]);
    }
    free(node->children);
    free(node->pattern);
    free(node);
}

void ac_destroy(struct ACAutoMation *mation)
{
    if (NULL == mation) {
        return;
    }
    destroy_trie_node(mation->root);
    free(mation);
}

void add_trie_node(struct ACAutoMation *mation, struct TrieNode *parent, const char *str, size_t length)
{
    struct TrieNode *node = NULL;

    if (NULL != parent->children) {
        int32_t i = 0;
        for (; i < parent->children_len; i++) {
            struct TrieNode *children = parent->children[i];
            if (length < children->pattern_len) {
                continue;
            }
            if (strncmp(children->pattern, str, children->pattern_len) == 0) {
                node = children;
            }
        }
    }

    if (NULL == node) {

        size_t pattern_len = (*(mation->get_word_length))(str, length);

        node = (struct TrieNode*)malloc(sizeof(struct TrieNode));
        if (NULL == node) {
            return;
        }
        node->children = NULL;
        node->pattern = (char*)malloc(pattern_len);
        if (NULL == node->pattern) {
            free(node);
            return;
        }

        // expand for new node
        int32_t children_len = parent->children_len + 1;
        struct TrieNode **children = realloc(parent->children, sizeof(struct TrieNode *) * children_len);
        if (NULL == children) {
            free(node->pattern);
            free(node);
            return;
        }
        parent->children_len = children_len;
        parent->children = children;

        node->pattern_len = pattern_len;
        memcpy(node->pattern, str, node->pattern_len);

        parent->children[parent->children_len - 1] = node;
        node->fail = parent;
        mation->dirty_node += 1;
    }

    if (node->pattern_len == length) { // end
        node->end += 1;
        mation->word_count += 1;
    } else {
        add_trie_node(mation, node, str + node->pattern_len, length - node->pattern_len);
    }
}

void ac_add(struct ACAutoMation *mation, const char *str, size_t length)
{
    add_trie_node(mation, mation->root, str, length);
}

void build_failed_pointer(struct TrieNode *root, struct TrieNode *bfs_head, struct TrieNode *bfs_tail)
{
    if (NULL == bfs_head || bfs_head->fail == root) {
        return;
    }

    int32_t i = 0;
    for (; i < bfs_head->children_len; i++) {
        bfs_head->children[i]->fail = bfs_head;

        bfs_tail->next = bfs_head->children[i];
        bfs_tail = bfs_tail->next;
    }

    struct TrieNode *fail = bfs_head->fail;
    for (; fail != NULL; fail = fail->fail) {
        for (; i < fail->children_len; i++) {
            struct TrieNode *children = fail->children[i];
            if (children != bfs_head && strncmp(children->pattern, bfs_head->pattern, children->pattern_len) == 0) {
                bfs_head->fail = children;

                build_failed_pointer(root, bfs_head->next, bfs_tail);
                bfs_head->next = NULL;
                return;
            }
        }
    }

    bfs_head->fail = root;
    build_failed_pointer(root, bfs_head->next, bfs_tail);
    bfs_head->next = NULL;
}

int32_t match_trie_node(struct ACAutoMation *mation, struct TrieNode* current, const char *str, size_t length)
{
    int32_t i = 0;
    for (; i < current->children_len; i++) {
        struct TrieNode *children = current->children[i];
        if (children->pattern_len > length) {
            continue;
        }
        if (strncmp(children->pattern, str, children->pattern_len) != 0) {
            continue;
        }
        if (children->end > 0) {
            return children->end;
        }

        // matched
        return match_trie_node(mation, children, str + children->pattern_len, length - children->pattern_len);
    }

    if (current != mation->root) {
        return match_trie_node(mation, current->fail, str, length);
    }

    // not matched
    int32_t len = (*mation->get_word_length)(str, length);
    if (len <= length && len > 0) {
        return match_trie_node(mation, current, str + len, length - len);
    }
    return -1;
}

int32_t ac_match(struct ACAutoMation *mation, const char *str, size_t length)
{
    if (mation->dirty_node > 0) {
        build_failed_pointer(mation->root, mation->root, mation->root);
        mation->dirty_node = 0;
    }

    return match_trie_node(mation, mation->root, str, length);
}
