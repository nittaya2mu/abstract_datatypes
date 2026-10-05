

#if defined(__APPLE__) || defined(__MACH__)
#include <ncurses.h>
#elif defined(__has_include)
#if __has_include(<ncursesw/ncurses.h>)
#include <ncursesw/ncurses.h>
#elif __has_include(<ncurses.h>)
#include <ncurses.h>
#else
#include <curses.h>
#endif
#else
#include <ncurses.h>
#endif
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COURSES 300
#define MAX_STR_LEN 100
#define MAX_CREDITS 22
#define TIME_SLOTS 48
#define TRIE_SIZE 256

#define CP_TITLE 1
#define CP_HEADER 2
#define CP_NORMAL 3
#define CP_SELECT 4
#define CP_SUCCESS 5
#define CP_ERROR 6
#define CP_DIM 7
#define CP_WARN 8
#define CP_DONE 9
#define CP_ENROLL 10
#define CP_FOOTER 11
#define CP_MENU 12

#define IS_ENTER(c) ((c) == '\n' || (c) == '\r' || (c) == KEY_ENTER)
#define IS_QUIT(c) ((c) == 'q' || (c) == 'Q' || (c) == 27)

static int SCROWS, SCCOLS;
static char current_user[MAX_STR_LEN] = "";

struct Course {
  char id[MAX_STR_LEN];
  char name[MAX_STR_LEN];
  int credits;
  int day;
  float start_time;
  float end_time;
  char prereq[MAX_STR_LEN];
  char instructor[MAX_STR_LEN];
  char major[MAX_STR_LEN];
};

struct CourseNode {
  char course_id[MAX_STR_LEN];
  struct CourseNode *next;
};

struct AVLNode {
  struct Course *course;
  struct AVLNode *left, *right;
  int height;
};

struct Queue {
  int data[MAX_COURSES];
  int front, rear, size;
};

struct GraphNode {
  int dest;
  struct GraphNode *next;
};

struct Graph {
  struct GraphNode *adj[MAX_COURSES];
  int in_degree[MAX_COURSES];
  int num_nodes;
};

struct TrieNode {
  struct TrieNode *children[TRIE_SIZE];
  int course_index;
  bool is_end;
};

struct Student {
  struct CourseNode *completed_head;
  int num_completed;
  struct CourseNode *enrolled_head;
  int num_enrolled;
  int total_enrolled_credits;
  bool completed_set[MAX_COURSES];
  bool enrolled_set[MAX_COURSES];
  bool time_grid[8][TIME_SLOTS];
};

bool auth_user_exists(const char *u) {
  FILE *f = fopen("users.txt", "r");
  if (!f)
    return false;
  char fu[100], fp[100];
  while (fscanf(f, "%99s %99s", fu, fp) == 2) {
    if (!strcmp(fu, u)) {
      fclose(f);
      return true;
    }
  }
  fclose(f);
  return false;
}

bool auth_login(const char *u, const char *p) {
  FILE *f = fopen("users.txt", "r");
  if (!f)
    return false;
  char fu[100], fp[100];
  while (fscanf(f, "%99s %99s", fu, fp) == 2) {
    if (!strcmp(fu, u) && !strcmp(fp, p)) {
      fclose(f);
      return true;
    }
  }
  fclose(f);
  return false;
}

bool auth_register(const char *u, const char *p) {
  if (auth_user_exists(u))
    return false;
  FILE *f = fopen("users.txt", "a");
  if (!f)
    return false;
  fprintf(f, "%s %s\n", u, p);
  fclose(f);
  return true;
}

const char *getDayName(int d) {
  switch (d) {
  case 1:
    return "Mon";
  case 2:
    return "Tue";
  case 3:
    return "Wed";
  case 4:
    return "Thu";
  case 5:
    return "Fri";
  case 6:
    return "Sat";
  case 7:
    return "Sun";
  default:
    return "???";
  }
}
void formatString(char *s) {
  for (int i = 0; s[i]; i++)
    if (s[i] == '_')
      s[i] = ' ';
}

int get_utf8_char_len(char c) {
  if ((c & 0x80) == 0)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 1;
}
int get_safe_slice_len(char *s, int m) {
  int l = 0;
  while (s[l] && l < m) {
    int cl = get_utf8_char_len(s[l]);
    if (l + cl > m)
      break;
    l += cl;
  }
  return l;
}

void addCourseNode(struct CourseNode **h, const char *id) {
  struct CourseNode *n = malloc(sizeof *n);
  strcpy(n->course_id, id);
  n->next = *h;
  *h = n;
}
bool removeCourseNode(struct CourseNode **h, const char *id) {
  struct CourseNode *c = *h, *p = NULL;
  while (c) {
    if (!strcmp(c->course_id, id)) {
      if (!p)
        *h = c->next;
      else
        p->next = c->next;
      free(c);
      return true;
    }
    p = c;
    c = c->next;
  }
  return false;
}
void freeCourseNodes(struct CourseNode *h) {
  while (h) {
    struct CourseNode *t = h;
    h = h->next;
    free(t);
  }
}

int compareCourses(const void *a, const void *b) {
  return strcmp(((struct Course *)a)->id, ((struct Course *)b)->id);
}
struct Course *getCourseById(struct Course *a, int n, const char *id) {
  int lo = 0, hi = n - 1;
  while (lo <= hi) {
    int m = lo + (hi - lo) / 2, c = strcmp(a[m].id, id);
    if (!c)
      return &a[m];
    if (c < 0)
      lo = m + 1;
    else
      hi = m - 1;
  }
  return NULL;
}

struct TrieNode *newTrieNode(void) {
  struct TrieNode *n = calloc(1, sizeof *n);
  n->course_index = -1;
  return n;
}
void trieInsert(struct TrieNode *r, const char *id, int idx) {
  struct TrieNode *c = r;
  for (int i = 0; id[i]; i++) {
    unsigned char ch = (unsigned char)id[i];
    if (!c->children[ch])
      c->children[ch] = newTrieNode();
    c = c->children[ch];
  }
  c->is_end = true;
  c->course_index = idx;
}
int trieSearch(struct TrieNode *r, const char *id) {
  struct TrieNode *c = r;
  for (int i = 0; id[i]; i++) {
    unsigned char ch = (unsigned char)id[i];
    if (!c->children[ch])
      return -1;
    c = c->children[ch];
  }
  return c->is_end ? c->course_index : -1;
}
void trieFree(struct TrieNode *r) {
  if (!r)
    return;
  for (int i = 0; i < TRIE_SIZE; i++)
    trieFree(r->children[i]);
  free(r);
}

static int timeToSlot(float t) { return (int)(t * 2.0f); }
bool hasTimeConflictFast(bool tg[][TIME_SLOTS], int day, float s, float e) {
  if (day < 1 || day > 7)
    return false;
  int ss = timeToSlot(s), es = timeToSlot(e);
  if (ss < 0)
    ss = 0;
  if (es < 0)
    es = 0;
  if (ss > TIME_SLOTS)
    ss = TIME_SLOTS;
  if (es > TIME_SLOTS)
    es = TIME_SLOTS;
  for (int i = ss; i < es; i++) {
    if (tg[day][i])
      return true;
  }
  return false;
}
void markTimeSlots(bool tg[][TIME_SLOTS], int day, float s, float e) {
  if (day < 1 || day > 7)
    return;
  int ss = timeToSlot(s), es = timeToSlot(e);
  if (ss < 0)
    ss = 0;
  if (es < 0)
    es = 0;
  if (ss > TIME_SLOTS)
    ss = TIME_SLOTS;
  if (es > TIME_SLOTS)
    es = TIME_SLOTS;
  for (int i = ss; i < es; i++)
    tg[day][i] = true;
}
void unmarkTimeSlots(bool tg[][TIME_SLOTS], int day, float s, float e) {
  if (day < 1 || day > 7)
    return;
  int ss = timeToSlot(s), es = timeToSlot(e);
  if (ss < 0)
    ss = 0;
  if (es < 0)
    es = 0;
  if (ss > TIME_SLOTS)
    ss = TIME_SLOTS;
  if (es > TIME_SLOTS)
    es = TIME_SLOTS;
  for (int i = ss; i < es; i++)
    tg[day][i] = false;
}

static int avlH(struct AVLNode *n) { return n ? n->height : 0; }
static void avlUpdH(struct AVLNode *n) {
  if (!n)
    return;
  int l = avlH(n->left), r = avlH(n->right);
  n->height = 1 + (l > r ? l : r);
}
static struct AVLNode *avlRotR(struct AVLNode *y) {
  struct AVLNode *x = y->left, *T = x->right;
  x->right = y;
  y->left = T;
  avlUpdH(y);
  avlUpdH(x);
  return x;
}
static struct AVLNode *avlRotL(struct AVLNode *x) {
  struct AVLNode *y = x->right, *T = y->left;
  y->left = x;
  x->right = T;
  avlUpdH(x);
  avlUpdH(y);
  return y;
}
static struct AVLNode *avlBal(struct AVLNode *n) {
  avlUpdH(n);
  int bf = avlH(n->left) - avlH(n->right);
  if (bf > 1) {
    if (avlH(n->left->right) > avlH(n->left->left))
      n->left = avlRotL(n->left);
    return avlRotR(n);
  }
  if (bf < -1) {
    if (avlH(n->right->left) > avlH(n->right->right))
      n->right = avlRotR(n->right);
    return avlRotL(n);
  }
  return n;
}
struct AVLNode *avlInsert(struct AVLNode *r, struct Course *c) {
  if (!r) {
    struct AVLNode *n = malloc(sizeof *n);
    n->course = c;
    n->left = n->right = NULL;
    n->height = 1;
    return n;
  }
  int cmp = strcmp(c->id, r->course->id);
  if (cmp < 0)
    r->left = avlInsert(r->left, c);
  else if (cmp > 0)
    r->right = avlInsert(r->right, c);
  return avlBal(r);
}
struct Course *avlSearch(struct AVLNode *r, const char *id) {
  if (!r)
    return NULL;
  int cmp = strcmp(id, r->course->id);
  if (!cmp)
    return r->course;
  return cmp < 0 ? avlSearch(r->left, id) : avlSearch(r->right, id);
}
void avlFree(struct AVLNode *r) {
  if (!r)
    return;
  avlFree(r->left);
  avlFree(r->right);
  free(r);
}

static int avl_collect_idx;
static struct Course **avl_collect_buf;
void avlCollect(struct AVLNode *r) {
  if (!r)
    return;
  avlCollect(r->left);
  avl_collect_buf[avl_collect_idx++] = r->course;
  avlCollect(r->right);
}

void qInit(struct Queue *q) { q->front = q->rear = q->size = 0; }
bool qEmpty(struct Queue *q) { return q->size == 0; }
void qPush(struct Queue *q, int v) {
  q->data[q->rear] = v;
  q->rear = (q->rear + 1) % MAX_COURSES;
  q->size++;
}
int qPop(struct Queue *q) {
  int v = q->data[q->front];
  q->front = (q->front + 1) % MAX_COURSES;
  q->size--;
  return v;
}

void graphInit(struct Graph *g, int n) {
  g->num_nodes = n;
  for (int i = 0; i < n; i++) {
    g->adj[i] = NULL;
    g->in_degree[i] = 0;
  }
}
void graphAddEdge(struct Graph *g, int fr, int to) {
  struct GraphNode *nd = malloc(sizeof *nd);
  nd->dest = to;
  nd->next = g->adj[fr];
  g->adj[fr] = nd;
  g->in_degree[to]++;
}
void graphFree(struct Graph *g) {
  for (int i = 0; i < g->num_nodes; i++) {
    struct GraphNode *c = g->adj[i];
    while (c) {
      struct GraphNode *t = c;
      c = c->next;
      free(t);
    }
    g->adj[i] = NULL;
  }
}
struct Graph buildPrereqGraph(struct Course *a, int n, struct TrieNode *t) {
  struct Graph g;
  graphInit(&g, n);
  for (int i = 0; i < n; i++) {
    if (strcmp(a[i].prereq, "NONE") != 0) {
      int pi = trieSearch(t, a[i].prereq);
      if (pi >= 0)
        graphAddEdge(&g, pi, i);
    }
  }
  return g;
}
int topoSort(struct Graph *g, int *order) {
  int deg[MAX_COURSES];
  for (int i = 0; i < g->num_nodes; i++)
    deg[i] = g->in_degree[i];
  struct Queue q;
  qInit(&q);
  for (int i = 0; i < g->num_nodes; i++) {
    if (!deg[i])
      qPush(&q, i);
  }
  int cnt = 0;
  while (!qEmpty(&q)) {
    int cur = qPop(&q);
    order[cnt++] = cur;
    for (struct GraphNode *nb = g->adj[cur]; nb; nb = nb->next) {
      if (!--deg[nb->dest])
        qPush(&q, nb->dest);
    }
  }
  return cnt;
}

static void mergeByTime(struct Course **a, int l, int m, int r) {
  int n1 = m - l + 1, n2 = r - m;
  struct Course *L[24], *R[24];
  for (int i = 0; i < n1; i++)
    L[i] = a[l + i];
  for (int i = 0; i < n2; i++)
    R[i] = a[m + 1 + i];
  int i = 0, j = 0, k = l;
  while (i < n1 && j < n2) {
    a[k++] = (L[i]->start_time <= R[j]->start_time) ? L[i++] : R[j++];
  }
  while (i < n1)
    a[k++] = L[i++];
  while (j < n2)
    a[k++] = R[j++];
}
void mergeSortTime(struct Course **a, int l, int r) {
  if (l >= r)
    return;
  int m = (l + r) / 2;
  mergeSortTime(a, l, m);
  mergeSortTime(a, m + 1, r);
  mergeByTime(a, l, m, r);
}

struct AVLNode *buildAvailableAVL(struct Course *a, int n, struct Student *st,
                                  struct TrieNode *trie) {
  struct AVLNode *root = NULL;
  for (int i = 0; i < n; i++) {
    struct Course *c = &a[i];
    if (st->total_enrolled_credits + c->credits > MAX_CREDITS)
      continue;
    if (strcmp(c->prereq, "NONE") != 0) {
      int pi = trieSearch(trie, c->prereq);
      if (pi < 0 || !st->completed_set[pi])
        continue;
    }
    if (st->completed_set[i] || st->enrolled_set[i])
      continue;
    if (hasTimeConflictFast(st->time_grid, c->day, c->start_time, c->end_time))
      continue;
    root = avlInsert(root, c);
  }
  return root;
}

void saveStudentData(struct Student *st) {
  char filename[128];
  snprintf(filename, sizeof(filename), "student_%s.txt", current_user);
  FILE *f = fopen(filename, "w");
  if (!f)
    return;

  fprintf(f, "COMPLETED\n");
  for (struct CourseNode *c = st->completed_head; c; c = c->next) {
    fprintf(f, "%s\n", c->course_id);
  }
  fprintf(f, "END_COMPLETED\n");

  fprintf(f, "ENROLLED\n");
  for (struct CourseNode *c = st->enrolled_head; c; c = c->next) {
    fprintf(f, "%s\n", c->course_id);
  }
  fprintf(f, "END_ENROLLED\n");

  fclose(f);
}

void init_colors(void) {
  start_color();
  use_default_colors();
  init_pair(CP_TITLE, COLOR_WHITE, COLOR_BLUE);
  init_pair(CP_HEADER, COLOR_CYAN, -1);
  init_pair(CP_NORMAL, COLOR_WHITE, -1);
  init_pair(CP_SELECT, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_SUCCESS, COLOR_GREEN, -1);
  init_pair(CP_ERROR, COLOR_RED, -1);
  init_pair(CP_DIM, COLOR_WHITE, -1);
  init_pair(CP_WARN, COLOR_YELLOW, -1);
  init_pair(CP_DONE, COLOR_GREEN, -1);
  init_pair(CP_ENROLL, COLOR_CYAN, -1);
  init_pair(CP_FOOTER, COLOR_BLACK, COLOR_WHITE);
  init_pair(CP_MENU, COLOR_YELLOW, -1);
}

void draw_titlebar(const char *title, struct Student *st) {
  attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
  mvhline(0, 0, ' ', SCCOLS);
  mvprintw(0, 2, " %s", title);
  if (st) {
    char info[64];
    snprintf(info, sizeof info, "Credits: %d/%d  Enrolled: %d ",
             st->total_enrolled_credits, MAX_CREDITS, st->num_enrolled);
    mvprintw(0, SCCOLS - (int)strlen(info) - 1, "%s", info);
  }
  attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);
}

void draw_footer(const char *hint) {
  attron(COLOR_PAIR(CP_FOOTER) | A_BOLD);
  mvhline(SCROWS - 1, 0, ' ', SCCOLS);
  mvprintw(SCROWS - 1, 2, " %s", hint);
  attroff(COLOR_PAIR(CP_FOOTER) | A_BOLD);
}

WINDOW *make_box(int h, int w, int y, int x, const char *title) {
  if (y < 0)
    y = 0;
  if (x < 0)
    x = 0;
  WINDOW *win = newwin(h, w, y, x);
  box(win, 0, 0);
  if (title) {
    wattron(win, COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (w - (int)strlen(title) - 2) / 2, " %s ", title);
    wattroff(win, COLOR_PAIR(CP_HEADER) | A_BOLD);
  }
  return win;
}

bool show_login_dialog(bool is_register) {
  char u[32] = "", p[32] = "";
  int focus = 0;
  char msg[128] = "";

  while (1) {
    erase();
    draw_titlebar(is_register ? "REGISTER" : "LOGIN", NULL);
    draw_footer(" Up/Down Switch Field   Enter Confirm   ESC Cancel");

    WINDOW *dlg =
        make_box(10, 50, (SCROWS - 10) / 2, (SCCOLS - 50) / 2,
                 is_register ? " REGISTER NEW ACCOUNT " : " LOGIN TO ACCOUNT ");

    wattron(dlg, COLOR_PAIR(CP_NORMAL));
    mvwprintw(dlg, 2, 4, "Username: ");
    mvwprintw(dlg, 4, 4, "Password: ");
    wattroff(dlg, COLOR_PAIR(CP_NORMAL));

    if (focus == 0)
      wattron(dlg, COLOR_PAIR(CP_SELECT) | A_BOLD);
    else
      wattron(dlg, COLOR_PAIR(CP_DIM));
    mvwprintw(dlg, 2, 14, " %-20s ", u);
    if (focus == 0)
      wattroff(dlg, COLOR_PAIR(CP_SELECT) | A_BOLD);
    else
      wattroff(dlg, COLOR_PAIR(CP_DIM));

    char p_mask[32] = "";
    for (int i = 0; p[i]; i++)
      p_mask[i] = '*';
    if (focus == 1)
      wattron(dlg, COLOR_PAIR(CP_SELECT) | A_BOLD);
    else
      wattron(dlg, COLOR_PAIR(CP_DIM));
    mvwprintw(dlg, 4, 14, " %-20s ", p_mask);
    if (focus == 1)
      wattroff(dlg, COLOR_PAIR(CP_SELECT) | A_BOLD);
    else
      wattroff(dlg, COLOR_PAIR(CP_DIM));

    if (msg[0]) {
      wattron(dlg, COLOR_PAIR(CP_ERROR) | A_BOLD);
      mvwprintw(dlg, 7, (50 - (int)strlen(msg)) / 2, "%s", msg);
      wattroff(dlg, COLOR_PAIR(CP_ERROR) | A_BOLD);
    }

    refresh();
    wrefresh(dlg);

    int ch = wgetch(dlg);
    if (IS_QUIT(ch)) {
      delwin(dlg);
      return false;
    } else if (ch == KEY_UP || ch == KEY_DOWN || ch == '\t') {
      focus = 1 - focus;
      msg[0] = '\0';
    } else if (IS_ENTER(ch)) {
      if (strlen(u) == 0) {
        strcpy(msg, "Username cannot be empty");
        focus = 0;
      } else if (strlen(p) == 0) {
        strcpy(msg, "Password cannot be empty");
        focus = 1;
      } else {
        if (is_register) {
          if (auth_register(u, p)) {
            strcpy(current_user, u);
            delwin(dlg);
            return true;
          } else {
            strcpy(msg, "Username already exists!");
            focus = 0;
          }
        } else {
          if (auth_login(u, p)) {
            strcpy(current_user, u);
            delwin(dlg);
            return true;
          } else {
            strcpy(msg, "Invalid username or password");
          }
        }
      }
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == 8) {
      char *target = focus == 0 ? u : p;
      int len = strlen(target);
      if (len > 0)
        target[len - 1] = '\0';
      msg[0] = '\0';
    } else if (ch >= 32 && ch < 127) {
      char *target = focus == 0 ? u : p;
      int len = strlen(target);
      if (len < 20 && ch != ' ') {
        if (focus == 0 && !((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) {
          msg[0] = '\0';
          continue; // Block special chars in username
        }
        target[len] = ch;
        target[len + 1] = '\0';
      }
      msg[0] = '\0';
    }
  }
}

bool show_auth_menu(void) {
  int sel = 0;
  const char *opts[] = {"Login to existing account", "Register new account",
                        "Exit Program"};
  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    erase();
    draw_titlebar("KU CSC Course Manager", NULL);
    draw_footer(" Up/Down Navigate   Enter Select");

    WINDOW *win =
        make_box(8, 44, (SCROWS - 8) / 2, (SCCOLS - 44) / 2, " WELCOME ");

    for (int i = 0; i < 3; i++) {
      if (i == sel) {
        wattron(win, COLOR_PAIR(CP_SELECT) | A_BOLD);
        mvwprintw(win, i + 2, 4, " %d. %-32s", i + 1, opts[i]);
        wattroff(win, COLOR_PAIR(CP_SELECT) | A_BOLD);
      } else {
        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, i + 2, 4, " %d. %-32s", i + 1, opts[i]);
        wattroff(win, COLOR_PAIR(CP_NORMAL));
      }
    }
    refresh();
    wrefresh(win);

    int ch = getch();
    if (ch == KEY_UP)
      sel = (sel - 1 + 3) % 3;
    else if (ch == KEY_DOWN)
      sel = (sel + 1) % 3;
    else if (IS_ENTER(ch)) {
      delwin(win);
      if (sel == 0) {
        if (show_login_dialog(false))
          return true;
      } else if (sel == 1) {
        if (show_login_dialog(true))
          return true;
      } else if (sel == 2)
        return false;
    } else if (IS_QUIT(ch)) {
      delwin(win);
      return false;
    }
    delwin(win);
  }
}

static const char *MENU_ITEMS[] = {
    "  View Available Courses    ",
    "  View My Timetable         ",
    "  Add Course                ",
    "  Drop Course               ",
    "  Recommend Courses         ",
    "  Prerequisite Map          ",
    "  Logout & Exit",
};
#define MENU_COUNT 7

int show_menu(struct Student *st) {
  int sel = 0;
  int box_h = MENU_COUNT + 4;
  int box_w = 54;
  int box_y, box_x;

  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    box_y = (SCROWS - box_h) / 2;
    box_x = (SCCOLS - box_w) / 2;
    erase();
    draw_titlebar("KU CSC Course Manager", st);
    draw_footer(" Up/Down Navigate   Enter Select   Number Shortcut   Q Quit");

    int pct = (st->total_enrolled_credits * (SCCOLS - 20)) / MAX_CREDITS;
    attron(COLOR_PAIR(CP_DIM));
    mvprintw(SCROWS - 3, 2, "Credits ");
    attroff(COLOR_PAIR(CP_DIM));

    attron(COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    for (int i = 0; i < pct; i++)
      mvaddch(SCROWS - 3, 10 + i, ACS_BLOCK);
    attroff(COLOR_PAIR(CP_SUCCESS) | A_BOLD);

    attron(COLOR_PAIR(CP_DIM));
    for (int i = pct; i < SCCOLS - 20; i++)
      mvaddch(SCROWS - 3, 10 + i, ACS_HLINE);
    mvprintw(SCROWS - 3, SCCOLS - 9, "%2d / %2d", st->total_enrolled_credits,
             MAX_CREDITS);
    attroff(COLOR_PAIR(CP_DIM));

    mvprintw(SCROWS - 5, 2, "Logged in as: ");
    attron(COLOR_PAIR(CP_SUCCESS) | A_BOLD);
    printw("%s", current_user);
    attroff(COLOR_PAIR(CP_SUCCESS) | A_BOLD);

    WINDOW *menu_win = make_box(box_h, box_w, box_y, box_x, "MAIN MENU");

    for (int i = 0; i < MENU_COUNT; i++) {
      if (i == sel) {
        wattron(menu_win, COLOR_PAIR(CP_SELECT) | A_BOLD);
        mvwprintw(menu_win, i + 2, 1, "%-*s", box_w - 2, "");
        mvwprintw(menu_win, i + 2, 2, " %d.%s", i + 1, MENU_ITEMS[i]);
        wattroff(menu_win, COLOR_PAIR(CP_SELECT) | A_BOLD);
      } else {
        wattron(menu_win, COLOR_PAIR(CP_MENU) | A_BOLD);
        mvwprintw(menu_win, i + 2, 2, " %d.", i + 1);
        wattroff(menu_win, COLOR_PAIR(CP_MENU) | A_BOLD);
        wattron(menu_win, COLOR_PAIR(CP_NORMAL));
        wprintw(menu_win, "%s", MENU_ITEMS[i]);
        wattroff(menu_win, COLOR_PAIR(CP_NORMAL));
      }
    }
    refresh();
    wrefresh(menu_win);

    int ch = getch();
    if (ch == KEY_UP) {
      sel = (sel - 1 + MENU_COUNT) % MENU_COUNT;
    } else if (ch == KEY_DOWN) {
      sel = (sel + 1) % MENU_COUNT;
    } else if (IS_ENTER(ch)) {
      delwin(menu_win);
      return sel + 1;
    } else if (IS_QUIT(ch)) {
      delwin(menu_win);
      return 7;
    } else if (ch >= '1' && ch <= '7') {
      delwin(menu_win);
      return ch - '0';
    }

    delwin(menu_win);
  }
}

void show_available(struct Course *all, int n, struct Student *st,
                    struct TrieNode *trie) {
  struct Course *avail[MAX_COURSES];
  int avail_n = 0;
  {
    struct AVLNode *tree = buildAvailableAVL(all, n, st, trie);
    avl_collect_buf = avail;
    avl_collect_idx = 0;
    avlCollect(tree);
    avail_n = avl_collect_idx;
    avlFree(tree);
  }

  int sel = 0, offset = 0;
  int list_h;
  char msg[128] = "";
  int msg_pair = CP_NORMAL;

  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    list_h = SCROWS - 6;
    erase();
    draw_titlebar("AVAILABLE COURSES", st);
    draw_footer(" Up/Down Scroll   Enter Enroll   Q Back");

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD | A_UNDERLINE);
    mvprintw(1, 0, " %-11s  %-36s  Cr  %-3s  %-13s  %-11s", "ID", "Name", "Day",
             "Time", "Prereq");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD | A_UNDERLINE);

    if (avail_n == 0) {
      attron(COLOR_PAIR(CP_ERROR) | A_BOLD);
      mvprintw(SCROWS / 2, (SCCOLS - 30) / 2,
               "  No courses available to enroll  ");
      attroff(COLOR_PAIR(CP_ERROR) | A_BOLD);
    }

    for (int i = 0; i < list_h && (i + offset) < avail_n; i++) {
      struct Course *c = avail[i + offset];
      int row = i + 2;
      bool is_sel = (i + offset == sel);
      char name_buf[37];
      strncpy(name_buf, c->name, 36);
      name_buf[36] = '\0';
      char time_buf[16];
      snprintf(time_buf, sizeof time_buf, "%05.2f-%05.2f", c->start_time,
               c->end_time);

      if (is_sel) {
        attron(COLOR_PAIR(CP_SELECT) | A_BOLD);
        mvhline(row, 0, ' ', SCCOLS);
        mvprintw(row, 0, " %-11s  %-36s  %2d  %-3s  %-13s  %-11s", c->id,
                 name_buf, c->credits, getDayName(c->day), time_buf, c->prereq);
        attroff(COLOR_PAIR(CP_SELECT) | A_BOLD);
      } else {
        attron(COLOR_PAIR(CP_WARN));
        mvprintw(row, 1, "%-11s", c->id);
        attroff(COLOR_PAIR(CP_WARN));
        attron(COLOR_PAIR(CP_NORMAL));
        mvprintw(row, 14, "%-36s  %2d  %-3s  %-13s  %-11s", name_buf,
                 c->credits, getDayName(c->day), time_buf, c->prereq);
        attroff(COLOR_PAIR(CP_NORMAL));
      }
    }

    attron(COLOR_PAIR(CP_DIM));
    mvprintw(SCROWS - 3, 2, " Total: %d courses  |  Showing %d-%d", avail_n,
             avail_n ? offset + 1 : 0,
             (offset + list_h < avail_n) ? offset + list_h : avail_n);
    if (offset > 0)
      mvprintw(SCROWS - 3, SCCOLS - 8, "  [MORE^]");
    if (offset + list_h < avail_n)
      mvprintw(SCROWS - 3, SCCOLS - 8, "  [MOREv]");
    attroff(COLOR_PAIR(CP_DIM));

    if (msg[0]) {
      attron(COLOR_PAIR(msg_pair) | A_BOLD);
      mvprintw(SCROWS - 2, (SCCOLS - (int)strlen(msg)) / 2, "%s", msg);
      attroff(COLOR_PAIR(msg_pair) | A_BOLD);
    }

    refresh();
    int ch = getch();

    if (IS_QUIT(ch))
      break;
    else if (ch == KEY_UP) {
      if (sel > 0)
        sel--;
      if (sel < offset)
        offset = sel;
      msg[0] = '\0';
    } else if (ch == KEY_DOWN) {
      if (sel < avail_n - 1)
        sel++;
      if (sel >= offset + list_h)
        offset = sel - list_h + 1;
      msg[0] = '\0';
    } else if (IS_ENTER(ch) && avail_n > 0) {
      struct Course *c = avail[sel];
      int idx = trieSearch(trie, c->id);
      addCourseNode(&st->enrolled_head, c->id);
      st->num_enrolled++;
      st->total_enrolled_credits += c->credits;
      if (idx >= 0) {
        st->enrolled_set[idx] = true;
        markTimeSlots(st->time_grid, c->day, c->start_time, c->end_time);
      }
      saveStudentData(st);
      snprintf(msg, sizeof msg, " Enrolled in %s! ", c->id);
      msg_pair = CP_SUCCESS;

      struct AVLNode *tree = buildAvailableAVL(all, n, st, trie);
      avl_collect_buf = avail;
      avl_collect_idx = 0;
      avlCollect(tree);
      avail_n = avl_collect_idx;
      avlFree(tree);

      if (sel >= avail_n)
        sel = avail_n > 0 ? avail_n - 1 : 0;
      if (offset >= avail_n)
        offset = avail_n > 0 ? avail_n - 1 : 0;
    }
  }
}

static void draw_center_str(int y, int x1, int x2, const char *str, int attr) {
  int box_w = x2 - x1 - 2; // Leave 1 space padding on left and right
  if (box_w <= 0 || y < 0 || y >= SCROWS)
    return;
  
  int len = (int)strlen(str);
  int print_len = len > box_w ? box_w : len;
  int safe_len = get_safe_slice_len((char *)str, print_len);
  
  int start_x = x1 + 1 + (box_w - safe_len) / 2;
  
  if (attr)
    attron(attr);
  mvwaddnstr(stdscr, y, start_x, str, safe_len);
  if (attr)
    attroff(attr);
}

void show_timetable(struct Course *all, int n, struct Student *st) {
  struct Course *by_day[8][24];
  int cnt[8] = {0};
  for (struct CourseNode *cur = st->enrolled_head; cur; cur = cur->next) {
    struct Course *c = getCourseById(all, n, cur->course_id);
    if (c && c->day >= 1 && c->day <= 7 && cnt[c->day] < 24)
      by_day[c->day][cnt[c->day]++] = c;
  }
  for (int d = 1; d <= 7; d++)
    if (cnt[d] > 1)
      mergeSortTime(by_day[d], 0, cnt[d] - 1);

  const char *days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    erase();
    draw_titlebar("MY TIMETABLE", st);
    draw_footer(" Q Back   ESC Back");

    int margin_x = 1;
    int time_w = 15;
    int avail_w = SCCOLS - 2 * margin_x - time_w - 9;
    if (avail_w < 70)
      avail_w = 70;
    int base_day_w = avail_w / 7;
    int extra = avail_w % 7;

    int col_x[9];
    col_x[0] = margin_x;
    col_x[1] = col_x[0] + time_w + 1;
    for (int d = 1; d <= 7; d++) {
      int cur_day_w = base_day_w + (d <= extra ? 1 : 0);
      col_x[d + 1] = col_x[d] + cur_day_w + 1;
    }

    // Determine row height for morning/afternoon to fill full vertical screen height
    int start_y = 1;
    int bottom_margin = 2;
    int avail_table_h = SCROWS - bottom_margin - start_y;
    int period_h = (avail_table_h - 5) / 2;
    if (period_h < 4)
      period_h = 4;

    // 1. Top border
    attron(COLOR_PAIR(CP_DIM));
    mvaddch(start_y, col_x[0], ACS_ULCORNER);
    for (int k = 0; k < 8; k++) {
      for (int x = col_x[k] + 1; x < col_x[k + 1]; x++)
        mvaddch(start_y, x, ACS_HLINE);
      mvaddch(start_y, col_x[k + 1], k == 7 ? ACS_URCORNER : ACS_TTEE);
    }
    attroff(COLOR_PAIR(CP_DIM));

    // 2. Header row
    int hy = start_y + 1;
    attron(COLOR_PAIR(CP_DIM));
    for (int k = 0; k <= 8; k++)
      mvaddch(hy, col_x[k], ACS_VLINE);
    attroff(COLOR_PAIR(CP_DIM));
    draw_center_str(hy, col_x[0] + 1, col_x[1], "Period / Time",
                    COLOR_PAIR(CP_HEADER) | A_BOLD);
    for (int d = 1; d <= 7; d++) {
      draw_center_str(hy, col_x[d] + 1, col_x[d + 1], days[d - 1],
                      COLOR_PAIR(CP_HEADER) | A_BOLD);
    }

    // 3. Header separator
    int sep_y = start_y + 2;
    attron(COLOR_PAIR(CP_DIM));
    mvaddch(sep_y, col_x[0], ACS_LTEE);
    for (int k = 0; k < 8; k++) {
      for (int x = col_x[k] + 1; x < col_x[k + 1]; x++)
        mvaddch(sep_y, x, ACS_HLINE);
      mvaddch(sep_y, col_x[k + 1], k == 7 ? ACS_RTEE : ACS_PLUS);
    }
    attroff(COLOR_PAIR(CP_DIM));

    struct {
      const char *label;
      const char *time_range;
      float start_limit;
      float end_limit;
    } periods[2] = {
        {"Morning", "09.00 - 12.00", 0.0f, 12.0f},
        {"Afternoon", "13.00 - 16.00", 12.0f, 24.0f}};

    int cur_y = sep_y + 1;

    for (int p = 0; p < 2; p++) {
      // Draw grid lines for this period
      for (int r = 0; r < period_h; r++) {
        attron(COLOR_PAIR(CP_DIM));
        for (int k = 0; k <= 8; k++)
          mvaddch(cur_y + r, col_x[k], ACS_VLINE);
        attroff(COLOR_PAIR(CP_DIM));
      }

      // Draw Time column (vertically centered)
      int t_pad = (period_h - 3) / 2;
      if (t_pad < 0)
        t_pad = 0;
      draw_center_str(cur_y + t_pad, col_x[0] + 1, col_x[1],
                      periods[p].time_range, COLOR_PAIR(CP_WARN) | A_BOLD);
      draw_center_str(cur_y + t_pad + 1, col_x[0] + 1, col_x[1],
                      periods[p].label, COLOR_PAIR(CP_HEADER));
      if (t_pad + 2 < period_h) {
        draw_center_str(cur_y + t_pad + 2, col_x[0] + 1, col_x[1],
                        "[ 3 Hours ]", COLOR_PAIR(CP_DIM));
      }

      // Draw Day columns
      for (int d = 1; d <= 7; d++) {
        struct Course *found = NULL;
        for (int ci = 0; ci < cnt[d]; ci++) {
          struct Course *c = by_day[d][ci];
          if ((c->start_time >= periods[p].start_limit && c->start_time < periods[p].end_limit) ||
              (c->end_time > periods[p].start_limit && c->end_time <= periods[p].end_limit) ||
              (c->start_time <= periods[p].start_limit && c->end_time >= periods[p].end_limit)) {
            found = c;
            break;
          }
        }

        if (found) {
          int c_pad = (period_h - 3) / 2;
          if (c_pad < 0)
            c_pad = 0;

          // 1. Course ID
          draw_center_str(cur_y + c_pad, col_x[d] + 1, col_x[d + 1],
                          found->id, COLOR_PAIR(CP_ENROLL) | A_BOLD);

          // 2. Course Name
          char name_buf[40];
          strncpy(name_buf, found->name, sizeof(name_buf) - 1);
          name_buf[sizeof(name_buf) - 1] = '\0';
          draw_center_str(cur_y + c_pad + 1, col_x[d] + 1, col_x[d + 1],
                          name_buf, COLOR_PAIR(CP_NORMAL) | A_BOLD);

          // 3. Instructor
          if (c_pad + 2 < period_h) {
            draw_center_str(cur_y + c_pad + 2, col_x[d] + 1, col_x[d + 1],
                            found->instructor, COLOR_PAIR(CP_WARN));
          }
        } else {
          // Empty cell: faint dash in center
          draw_center_str(cur_y + period_h / 2, col_x[d] + 1, col_x[d + 1],
                          "-", COLOR_PAIR(CP_DIM));
        }
      }

      cur_y += period_h;

      // Draw Lunch separator after Morning
      if (p == 0) {
        attron(COLOR_PAIR(CP_DIM));
        mvaddch(cur_y, col_x[0], ACS_LTEE);
        for (int x = col_x[0] + 1; x < col_x[8]; x++)
          mvaddch(cur_y, x, ACS_HLINE);
        mvaddch(cur_y, col_x[8], ACS_RTEE);
        attroff(COLOR_PAIR(CP_DIM));

        const char *lunch_str = " 12.00 - 13.00  Lunch Break ";
        draw_center_str(cur_y, col_x[0] + 1, col_x[8], lunch_str,
                        COLOR_PAIR(CP_WARN) | A_BOLD);
        cur_y++;
      }
    }

    // 4. Bottom border
    attron(COLOR_PAIR(CP_DIM));
    mvaddch(cur_y, col_x[0], ACS_LLCORNER);
    for (int k = 0; k < 8; k++) {
      for (int x = col_x[k] + 1; x < col_x[k + 1]; x++)
        mvaddch(cur_y, x, ACS_HLINE);
      mvaddch(cur_y, col_x[k + 1], k == 7 ? ACS_LRCORNER : ACS_BTEE);
    }
    attroff(COLOR_PAIR(CP_DIM));

    if (st->num_enrolled == 0) {
      attron(COLOR_PAIR(CP_ERROR) | A_BOLD);
      mvprintw(SCROWS / 2, (SCCOLS - 26) / 2, "  No courses enrolled yet  ");
      attroff(COLOR_PAIR(CP_ERROR) | A_BOLD);
    }

    refresh();
    int ch = getch();
    if (IS_QUIT(ch))
      break;
  }
}

void show_add_drop(struct Course *all, int n, struct Student *st,
                   struct TrieNode *trie, bool is_add) {
  int box_h = 12, box_w = 58;
  int box_y, box_x;

  char input[MAX_STR_LEN] = "";
  char msg[128] = "";
  int msg_pair = CP_NORMAL;
  bool done = false;

  while (!done) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    box_y = (SCROWS - box_h) / 2;
    box_x = (SCCOLS - box_w) / 2;
    erase();
    draw_titlebar(is_add ? "ADD COURSE" : "DROP COURSE", st);
    draw_footer(" Type Course ID   Enter Confirm   ESC Cancel");

    WINDOW *dlg = make_box(box_h, box_w, box_y, box_x,
                           is_add ? "ADD COURSE" : "DROP COURSE");

    wattron(dlg, COLOR_PAIR(CP_NORMAL));
    mvwprintw(dlg, 2, 2, "Enter Course ID:");
    wattroff(dlg, COLOR_PAIR(CP_NORMAL));

    wattron(dlg, COLOR_PAIR(CP_SELECT) | A_BOLD);
    mvwprintw(dlg, 3, 2, " %-20s ", input);
    wattroff(dlg, COLOR_PAIR(CP_SELECT) | A_BOLD);

    wattron(dlg, COLOR_PAIR(CP_DIM));
    mvwprintw(dlg, 5, 2, "%-54s", "");
    if (is_add)
      mvwprintw(dlg, 5, 2, "Tip: View Available (option 1) to see valid IDs");
    else
      mvwprintw(dlg, 5, 2,
                "Tip: Drop removes course & frees credits/time slot");
    wattroff(dlg, COLOR_PAIR(CP_DIM));

    if (msg[0]) {
      wattron(dlg, COLOR_PAIR(msg_pair) | A_BOLD);
      mvwprintw(dlg, 7, (box_w - (int)strlen(msg)) / 2, "%s", msg);
      wattroff(dlg, COLOR_PAIR(msg_pair) | A_BOLD);
    }

    wattron(dlg, COLOR_PAIR(CP_DIM));
    mvwprintw(dlg, 9, 2, "Enrolled: ");
    wattroff(dlg, COLOR_PAIR(CP_DIM));
    int cx = 12;
    for (struct CourseNode *cur = st->enrolled_head; cur && cx < box_w - 4;
         cur = cur->next) {
      wattron(dlg, COLOR_PAIR(CP_ENROLL));
      mvwprintw(dlg, 9, cx, "%s ", cur->course_id);
      wattroff(dlg, COLOR_PAIR(CP_ENROLL));
      cx += (int)strlen(cur->course_id) + 1;
    }

    refresh();
    wrefresh(dlg);

    int ch = wgetch(dlg);

    if (IS_QUIT(ch)) {
      done = true;
    } else if (IS_ENTER(ch) && input[0]) {
      if (is_add) {
        struct AVLNode *avl = buildAvailableAVL(all, n, st, trie);
        struct Course *vc = avlSearch(avl, input);
        if (vc) {
          int idx = trieSearch(trie, input);
          addCourseNode(&st->enrolled_head, input);
          st->num_enrolled++;
          st->total_enrolled_credits += vc->credits;
          if (idx >= 0) {
            st->enrolled_set[idx] = true;
            markTimeSlots(st->time_grid, vc->day, vc->start_time, vc->end_time);
          }
          saveStudentData(st);
          snprintf(msg, sizeof msg, " Enrolled in %s! (+%d cr) ", input,
                   vc->credits);
          msg_pair = CP_SUCCESS;
        } else {
          int idx = trieSearch(trie, input);
          if (idx < 0)
            snprintf(msg, sizeof msg, " Course %s not found ", input);
          else {
            struct Course *c = &all[idx];
            if (st->completed_set[idx])
              snprintf(msg, sizeof msg, " Already completed %s ", input);
            else if (st->enrolled_set[idx])
              snprintf(msg, sizeof msg, " Already enrolled in %s ", input);
            else if (st->total_enrolled_credits + c->credits > MAX_CREDITS)
              snprintf(msg, sizeof msg, " Exceeds %d credit limit ",
                       MAX_CREDITS);
            else if (hasTimeConflictFast(st->time_grid, c->day, c->start_time,
                                         c->end_time))
              snprintf(msg, sizeof msg, " Time conflict! ");
            else
              snprintf(msg, sizeof msg, " Prerequisite not met: %s ",
                       c->prereq);
          }
          msg_pair = CP_ERROR;
        }
        avlFree(avl);
      } else {
        if (removeCourseNode(&st->enrolled_head, input)) {
          int idx = trieSearch(trie, input);
          if (idx >= 0) {
            st->total_enrolled_credits -= all[idx].credits;
            st->enrolled_set[idx] = false;
            unmarkTimeSlots(st->time_grid, all[idx].day, all[idx].start_time,
                            all[idx].end_time);
          }
          st->num_enrolled--;
          saveStudentData(st);
          snprintf(msg, sizeof msg, " Dropped %s! ", input);
          msg_pair = CP_SUCCESS;
        } else {
          snprintf(msg, sizeof msg, " Not enrolled in %s ", input);
          msg_pair = CP_ERROR;
        }
      }
      input[0] = '\0';
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == 8) {
      int len = strlen(input);
      if (len > 0)
        input[len - 1] = '\0';
      msg[0] = '\0';
    } else if (ch >= 32 && ch < 127 && strlen(input) < MAX_STR_LEN - 1) {
      char add = (ch >= 'a' && ch <= 'z') ? ch - 32 : ch;
      int len = strlen(input);
      input[len] = add;
      input[len + 1] = '\0';
      msg[0] = '\0';
    }
    delwin(dlg);
  }
}

int backtrack_iters = 0;

static void find_best_bundle(struct Course **avail, int avail_n, int idx,
                             bool temp_tg[8][TIME_SLOTS], int current_credits,
                             int budget, bool *current_sel, bool *best_sel,
                             int *max_credits, int *suffix_credits) {
  if (backtrack_iters++ > 50000)
    return;
  if (current_credits > *max_credits) {
    *max_credits = current_credits;
    for (int i = 0; i < avail_n; i++)
      best_sel[i] = current_sel[i];
  }

  if (idx >= avail_n || current_credits == budget)
    return;

  if (current_credits + suffix_credits[idx] <= *max_credits)
    return;

  struct Course *c = avail[idx];
  if (current_credits + c->credits <= budget &&
      !hasTimeConflictFast(temp_tg, c->day, c->start_time, c->end_time)) {
    markTimeSlots(temp_tg, c->day, c->start_time, c->end_time);
    current_sel[idx] = true;
    find_best_bundle(avail, avail_n, idx + 1, temp_tg,
                     current_credits + c->credits, budget, current_sel,
                     best_sel, max_credits, suffix_credits);
    current_sel[idx] = false;
    unmarkTimeSlots(temp_tg, c->day, c->start_time, c->end_time);
  }
  find_best_bundle(avail, avail_n, idx + 1, temp_tg, current_credits, budget,
                   current_sel, best_sel, max_credits, suffix_credits);
}

void show_recommend(struct Course *all, int n, struct Student *st,
                    struct TrieNode *trie) {
  const char *majors[] = {"CPE (Computer Eng)",
                          "EE (Electrical Eng)",
                          "ME (Mechanical Eng)",
                          "CE (Civil Eng)",
                          "CS (Computer Science)",
                          "CHEM (Applied Chem)",
                          "MGT (Management)",
                          "MKT (Marketing)",
                          "ACC (Accounting)",
                          "FIN (Finance)",
                          "LAW (Law)",
                          "PA (Public Admin)",
                          "ENG (English)",
                          "HIM (Hospitality)",
                          "PUBH (Public Health)",
                          "FOOD (Food Tech)",
                          "FSN (Food Safety)",
                          "AGRI (Agriculture)",
                          "ANIM (Animal Science)",
                          "ALL (All Majors)"};
  const char *major_codes[] = {
      "CPE", "EE", "ME",  "CE",  "CS",   "CHEM", "MGT", "MKT",  "ACC",  "FIN",
      "LAW", "PA", "ENG", "HIM", "PUBH", "FOOD", "FSN", "AGRI", "ANIM", "ALL"};
  int sel_major = 0;
  bool major_chosen = false;

  while (!major_chosen) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    erase();
    draw_titlebar("RECOMMEND COURSES - Select Major", st);
    draw_footer(" Up/Down Navigate   Enter Select   ESC Cancel");
    WINDOW *win = make_box(15, 50, (SCROWS - 15) / 2, (SCCOLS - 50) / 2,
                           " SELECT MAJOR ");
    int list_start = (sel_major / 10) * 10;
    int list_end = list_start + 10;
    if (list_end > 20)
      list_end = 20;
    for (int i = list_start; i < list_end; i++) {
      int display_idx = i - list_start;
      if (i == sel_major) {
        wattron(win, COLOR_PAIR(CP_SELECT) | A_BOLD);
        mvwprintw(win, display_idx + 2, 2, " %d. %-35s", i + 1, majors[i]);
        wattroff(win, COLOR_PAIR(CP_SELECT) | A_BOLD);
      } else {
        wattron(win, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win, display_idx + 2, 2, " %d. %-35s", i + 1, majors[i]);
        wattroff(win, COLOR_PAIR(CP_NORMAL));
      }
    }
    wattron(win, COLOR_PAIR(CP_DIM));
    mvwprintw(win, 13, 2, "Showing %d-%d of 20 (Scroll)", list_start + 1,
              list_end);
    wattroff(win, COLOR_PAIR(CP_DIM));
    refresh();
    wrefresh(win);
    int ch = getch();
    if (IS_QUIT(ch)) {
      delwin(win);
      return;
    } else if (ch == KEY_UP)
      sel_major = (sel_major - 1 + 20) % 20;
    else if (ch == KEY_DOWN)
      sel_major = (sel_major + 1) % 20;
    else if (IS_ENTER(ch)) {
      major_chosen = true;
    }
    delwin(win);
  }
  const char *target_major = major_codes[sel_major];
  int R = MAX_CREDITS - st->total_enrolled_credits;

  struct Course *avail[MAX_COURSES];
  int avail_n = 0;
  for (int i = 0; i < n; i++) {
    struct Course *c = &all[i];
    if (c->credits > R)
      continue;
    if (strcmp(c->prereq, "NONE") != 0) {
      int pi = trieSearch(trie, c->prereq);
      if (pi < 0 || !st->completed_set[pi])
        continue;
    }
    if (st->completed_set[i] || st->enrolled_set[i])
      continue;
    if (hasTimeConflictFast(st->time_grid, c->day, c->start_time, c->end_time))
      continue;
    if (strcmp(target_major, "ALL") != 0 &&
        strcmp(c->major, target_major) != 0 && strcmp(c->major, "GENED") != 0)
      continue;
    avail[avail_n++] = c;
  }

  bool temp_tg[8][TIME_SLOTS];
  memcpy(temp_tg, st->time_grid, sizeof temp_tg);
  bool current_sel[MAX_COURSES] = {false};
  bool best_sel[MAX_COURSES] = {false};
  int max_credits = 0;

  backtrack_iters = 0;
  int suffix_credits[MAX_COURSES] = {0};
  if (avail_n > 0) {
    suffix_credits[avail_n - 1] = avail[avail_n - 1]->credits;
    for (int i = avail_n - 2; i >= 0; i--)
      suffix_credits[i] = suffix_credits[i + 1] + avail[i]->credits;
  }
  find_best_bundle(avail, avail_n, 0, temp_tg, 0, R, current_sel, best_sel,
                   &max_credits, suffix_credits);

  bool sel[MAX_COURSES];
  for (int i = 0; i < MAX_COURSES; i++)
    sel[i] = best_sel[i];

  int scroll = 0;
  int list_h;

  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    list_h = SCROWS - 8;

    erase();
    draw_titlebar("RECOMMEND COURSES  [Backtracking]", st);
    draw_footer(" Up/Down Scroll   Q Back");

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvprintw(1, 2,
             "Optimal bundle: %d/%d credits used  |  Budget: %d/%d remaining",
             max_credits, R, R - max_credits, R);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    attron(COLOR_PAIR(CP_WARN) | A_BOLD | A_UNDERLINE);
    mvprintw(2, 0, " %-11s  %-36s  Cr  %-3s  %-13s  Pick", "ID", "Name", "Day",
             "Time");
    attroff(COLOR_PAIR(CP_WARN) | A_BOLD | A_UNDERLINE);

    for (int i = 0; i < list_h && (i + scroll) < avail_n; i++) {
      int idx = i + scroll;
      struct Course *c = avail[idx];
      int row = i + 3;
      char time_buf[16];
      snprintf(time_buf, sizeof time_buf, "%05.2f-%05.2f", c->start_time,
               c->end_time);
      char name_s[37];
      strncpy(name_s, c->name, 36);
      name_s[36] = '\0';

      if (sel[idx]) {
        attron(COLOR_PAIR(CP_SUCCESS) | A_BOLD);
        mvprintw(row, 0, " %-11s  %-36s  %2d  %-3s  %-13s  [YES]", c->id,
                 name_s, c->credits, getDayName(c->day), time_buf);
        attroff(COLOR_PAIR(CP_SUCCESS) | A_BOLD);
      } else {
        attron(COLOR_PAIR(CP_DIM));
        mvprintw(row, 0, " %-11s  %-36s  %2d  %-3s  %-13s   ---", c->id, name_s,
                 c->credits, getDayName(c->day), time_buf);
        attroff(COLOR_PAIR(CP_DIM));
      }
    }

    attron(COLOR_PAIR(CP_DIM));
    mvprintw(SCROWS - 3, 2,
             "Green = Recommended   Grey = Not selected   Total available: %d",
             avail_n);
    attroff(COLOR_PAIR(CP_DIM));

    refresh();
    int ch = getch();
    if (IS_QUIT(ch))
      break;
    if (ch == KEY_UP && scroll > 0)
      scroll--;
    if (ch == KEY_DOWN && scroll + list_h < avail_n)
      scroll++;
  }
}

void show_prereq_map(struct Course *all, int n, struct Student *st,
                     struct TrieNode *trie) {
  struct Graph g = buildPrereqGraph(all, n, trie);
  int order[MAX_COURSES];
  int processed = topoSort(&g, order);
  bool has_cycle = (processed < n);
  if (has_cycle) {
    bool in_order[MAX_COURSES] = {false};
    for (int i = 0; i < processed; i++) in_order[order[i]] = true;
    for (int i = 0; i < n; i++) {
      if (!in_order[i]) order[processed++] = i;
    }
  }

  int scroll = 0, list_h;

  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    list_h = SCROWS - 5;
    erase();
    draw_titlebar("PREREQUISITE MAP  [Graph + Topological Sort]", st);
    draw_footer(" Up/Down Scroll   Q Back");

    if (has_cycle) {
      attron(COLOR_PAIR(CP_ERROR) | A_BOLD);
      mvprintw(1, 2, "[!] Circular prerequisite detected!");
      attroff(COLOR_PAIR(CP_ERROR) | A_BOLD);
    }

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD | A_UNDERLINE);
    mvprintw(2, 0, " %-4s  %-11s  %-36s  %-11s  %-12s  Unlocks", "Step", "ID",
             "Name", "Prereq", "Status");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD | A_UNDERLINE);

    for (int i = 0; i < list_h && (i + scroll) < processed; i++) {
      int idx = order[i + scroll];
      struct Course *c = &all[idx];
      int row = i + 3;

      const char *status_str;
      int status_pair;
      if (st->completed_set[idx]) {
        status_str = "[Done]    ";
        status_pair = CP_DONE;
      } else if (st->enrolled_set[idx]) {
        status_str = "[Enrolled]";
        status_pair = CP_ENROLL;
      } else {
        status_str = "[Not taken]";
        status_pair = CP_DIM;
      }

      attron(COLOR_PAIR(CP_MENU));
      mvprintw(row, 1, "%-4d", i + scroll + 1);
      attroff(COLOR_PAIR(CP_MENU));

      attron(COLOR_PAIR(CP_WARN));
      mvprintw(row, 6, "%-11s", c->id);
      attroff(COLOR_PAIR(CP_WARN));

      attron(COLOR_PAIR(CP_NORMAL));
      char name_s[37];
      strncpy(name_s, c->name, 36);
      name_s[36] = '\0';
      mvprintw(row, 18, "%-36s  %-11s  ", name_s, c->prereq);
      attroff(COLOR_PAIR(CP_NORMAL));

      attron(COLOR_PAIR(status_pair) | A_BOLD);
      mvprintw(row, 69, "%-12s", status_str);
      attroff(COLOR_PAIR(status_pair) | A_BOLD);

      int ucx = 82;
      attron(COLOR_PAIR(CP_SUCCESS));
      for (struct GraphNode *nb = g.adj[idx]; nb && ucx < SCCOLS - 2;
           nb = nb->next) {
        mvprintw(row, ucx, "%s ", all[nb->dest].id);
        ucx += (int)strlen(all[nb->dest].id) + 1;
      }
      attroff(COLOR_PAIR(CP_SUCCESS));
    }

    attron(COLOR_PAIR(CP_DIM));
    mvprintw(SCROWS - 3, 2,
             "Total: %d courses  |  Showing %d-%d  |  Green = [Done]  Cyan = "
             "[Enrolled]",
             processed, scroll + 1,
             scroll + list_h < processed ? scroll + list_h : processed);
    attroff(COLOR_PAIR(CP_DIM));

    refresh();
    int ch = getch();
    if (IS_QUIT(ch))
      break;
    if (ch == KEY_UP && scroll > 0)
      scroll--;
    if (ch == KEY_DOWN && scroll + list_h < processed)
      scroll++;
  }
  graphFree(&g);
}

int main(void) {
  struct Course all[MAX_COURSES];
  int n = 0;

  FILE *f = fopen("courses.txt", "r");
  if (!f) {
    fprintf(stderr, "Error: Cannot open courses.txt\n");
    return 1;
  }
  char line[1024];
  while (fgets(line, sizeof line, f) && n < MAX_COURSES) {
    strcpy(all[n].prereq, "NONE");
    strcpy(all[n].instructor, "-");
    strcpy(all[n].major, "GENED");
    int sc = sscanf(line, "%99s %99s %d %d %f %f %99s %99s %99s", all[n].id,
                    all[n].name, &all[n].credits, &all[n].day,
                    &all[n].start_time, &all[n].end_time, all[n].prereq,
                    all[n].instructor, all[n].major);
    if (sc >= 6) {
      formatString(all[n].name);
      formatString(all[n].instructor);
      n++;
    }
  }
  fclose(f);
  qsort(all, n, sizeof *all, compareCourses);

  struct TrieNode *trie = newTrieNode();
  for (int i = 0; i < n; i++)
    trieInsert(trie, all[i].id, i);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  set_escdelay(50);
  getmaxyx(stdscr, SCROWS, SCCOLS);

  if (SCROWS < 20 || SCCOLS < 95) {
    endwin();
    fprintf(stderr, "Terminal too small! Need at least 95x20\n");
    return 1;
  }

  init_colors();

  if (!show_auth_menu()) {
    endwin();
    trieFree(trie);
    return 0;
  }

  struct Student st;
  memset(&st, 0, sizeof st);

  char filename[128];
  snprintf(filename, sizeof(filename), "student_%s.txt", current_user);
  f = fopen(filename, "r");
  if (f) {
    char tok[MAX_STR_LEN];
    int mode = 0;
    while (fscanf(f, "%99s", tok) == 1) {
      if (!strcmp(tok, "COMPLETED"))
        mode = 1;
      else if (!strcmp(tok, "END_COMPLETED"))
        mode = 0;
      else if (!strcmp(tok, "ENROLLED"))
        mode = 2;
      else if (!strcmp(tok, "END_ENROLLED"))
        mode = 0;
      else {
        int idx = trieSearch(trie, tok);
        if (mode == 1) {
          addCourseNode(&st.completed_head, tok);
          st.num_completed++;
          if (idx >= 0)
            st.completed_set[idx] = true;
        } else if (mode == 2) {
          addCourseNode(&st.enrolled_head, tok);
          st.num_enrolled++;
          if (idx >= 0) {
            st.enrolled_set[idx] = true;
            st.total_enrolled_credits += all[idx].credits;
            markTimeSlots(st.time_grid, all[idx].day, all[idx].start_time,
                          all[idx].end_time);
          }
        }
      }
    }
    fclose(f);
  }

  while (1) {
    getmaxyx(stdscr, SCROWS, SCCOLS);
    int choice = show_menu(&st);

    if (choice == 1)
      show_available(all, n, &st, trie);
    else if (choice == 2)
      show_timetable(all, n, &st);
    else if (choice == 3)
      show_add_drop(all, n, &st, trie, true);
    else if (choice == 4)
      show_add_drop(all, n, &st, trie, false);
    else if (choice == 5)
      show_recommend(all, n, &st, trie);
    else if (choice == 6)
      show_prereq_map(all, n, &st, trie);
    else
      break;
  }

  endwin();
  freeCourseNodes(st.completed_head);
  freeCourseNodes(st.enrolled_head);
  trieFree(trie);

  printf("\033[1;32mGoodbye %s! Data saved to %s\033[0m\n", current_user,
         filename);
  return 0;
}
