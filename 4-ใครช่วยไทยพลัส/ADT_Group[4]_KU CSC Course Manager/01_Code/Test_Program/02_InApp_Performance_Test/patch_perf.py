import re

with open('main_ncurses.c', 'r', encoding='utf-8') as f:
    content = f.read()

time_include = """
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

double get_time_ms() {
#ifdef _WIN32
    LARGE_INTEGER freq, val;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&val);
    return (double)val.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
#endif
}
"""
content = content.replace('#include <string.h>', '#include <string.h>\n' + time_include)

# 1. Available courses
content = content.replace(
    'void show_available(struct Course *all, int n, struct Student *st,\n                    struct TrieNode *trie) {',
    'void show_available(struct Course *all, int n, struct Student *st,\n                    struct TrieNode *trie) {\n  char perf_msg[128] = "";\n'
)
content = content.replace(
    'struct AVLNode *tree = buildAvailableAVL(all, n, st, trie);',
    'double t0 = get_time_ms();\n    struct AVLNode *tree = buildAvailableAVL(all, n, st, trie);'
)
content = content.replace(
    'avail_n = avl_collect_idx;\n    avlFree(tree);',
    'avail_n = avl_collect_idx;\n    avlFree(tree);\n    double t1 = get_time_ms();\n    snprintf(perf_msg, sizeof(perf_msg), "AVL Time: %.3f ms", t1 - t0);'
)
content = content.replace(
    'draw_titlebar("AVAILABLE COURSES", st);',
    'char tb[256]; snprintf(tb, sizeof(tb), "AVAILABLE COURSES [%s]", perf_msg);\n    draw_titlebar(tb, st);'
)

# 2. Timetable
content = content.replace(
    'void show_timetable(struct Course *all, int n, struct Student *st) {',
    'void show_timetable(struct Course *all, int n, struct Student *st) {\n  double t0 = get_time_ms();'
)
content = content.replace(
    'const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};',
    'double t1 = get_time_ms();\n  char perf_msg[128]; snprintf(perf_msg, sizeof(perf_msg), "MergeSort Time: %.3f ms", t1 - t0);\n  const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};'
)
content = content.replace(
    'draw_titlebar("MY TIMETABLE", st);',
    'char tb[256]; snprintf(tb, sizeof(tb), "MY TIMETABLE [%s]", perf_msg);\n    draw_titlebar(tb, st);'
)

# 3. Add/drop enroll time
# we only replace the FIRST occurrence in show_add_drop. Oh wait, the replace above would hit show_available too!
# No, `msg_pair == CP_SUCCESS` is only in show_add_drop? Let's check `if (msg_pair == CP_SUCCESS)` - no, `msg_pair = CP_SUCCESS;` is in show_add_drop and show_available.
# To be safe, let's use a targeted replace for Add Course.
content = content.replace(
    'snprintf(msg, sizeof msg, " Enrolled in %s! (+%d cr) ", input,\n                   vc->credits);\n          msg_pair = CP_SUCCESS;',
    'snprintf(msg, sizeof msg, " Enrolled in %s! (+%d cr) ", input,\n                   vc->credits);\n          msg_pair = CP_SUCCESS;\n          double t1 = get_time_ms();\n          char tmp[256]; snprintf(tmp, sizeof(tmp), "%s [%.3f ms]", msg, t1-t0); strcpy(msg, tmp);'
)
# t0 needs to be declared at the top of show_add_drop or right before buildAvailableAVL
content = content.replace(
    'struct AVLNode *avl = buildAvailableAVL(all, n, st, trie);\n        struct Course *vc = avlSearch(avl, input);',
    'double t0 = get_time_ms();\n        struct AVLNode *avl = buildAvailableAVL(all, n, st, trie);\n        struct Course *vc = avlSearch(avl, input);'
)

# 4. Recommend
content = content.replace(
    'void show_recommend(struct Course *all, int n, struct Student *st,\n                    struct TrieNode *trie) {',
    'void show_recommend(struct Course *all, int n, struct Student *st,\n                    struct TrieNode *trie) {\n  char perf_msg[128] = "";\n'
)
content = content.replace(
    'find_best_bundle(avail, avail_n, 0, temp_tg, 0, R, current_sel, best_sel,',
    'double t0 = get_time_ms();\n  find_best_bundle(avail, avail_n, 0, temp_tg, 0, R, current_sel, best_sel,'
)
content = content.replace(
    '&max_credits, suffix_credits);',
    '&max_credits, suffix_credits);\n  double t1 = get_time_ms();\n  snprintf(perf_msg, sizeof(perf_msg), "Backtrack Time: %.3f ms (Iters: %d)", t1 - t0, backtrack_iters);'
)
content = content.replace(
    'draw_titlebar("RECOMMEND COURSES  [Backtracking]", st);',
    'char tb[256]; snprintf(tb, sizeof(tb), "RECOMMEND COURSES [%s]", perf_msg);\n    draw_titlebar(tb, st);'
)

# 5. Prereq Map
content = content.replace(
    'void show_prereq_map(struct Course *all, int n, struct Student *st,\n                     struct TrieNode *trie) {',
    'void show_prereq_map(struct Course *all, int n, struct Student *st,\n                     struct TrieNode *trie) {\n  char perf_msg[128] = "";\n'
)
content = content.replace(
    'struct Graph g = buildPrereqGraph(all, n, trie);',
    'double t0 = get_time_ms();\n  struct Graph g = buildPrereqGraph(all, n, trie);'
)
content = content.replace(
    'int scroll = 0, list_h;',
    'double t1 = get_time_ms();\n  snprintf(perf_msg, sizeof(perf_msg), "Graph+TopoSort: %.3f ms", t1 - t0);\n  int scroll = 0, list_h;'
)
content = content.replace(
    'draw_titlebar("PREREQUISITE MAP  [Graph + Topological Sort]", st);',
    'char tb[256]; snprintf(tb, sizeof(tb), "PREREQ MAP [%s]", perf_msg);\n    draw_titlebar(tb, st);'
)

with open('main_perf_test.c', 'w', encoding='utf-8') as f:
    f.write(content)

print("Patch applied successfully.")
