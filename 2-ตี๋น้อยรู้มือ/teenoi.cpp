// Teenoi Suki - Restaurant Reservation & Table Management (Terminal C++17)
// Build:  g++ -std=c++17 -O2 -o teenoi.exe teenoi.cpp
// Run:    ./teenoi.exe          (interactive menu)
//         ./teenoi.exe --test   (self-check of core logic & persistence, asserts, no menu)

#include <bits/stdc++.h>
#include <cassert>
#include <fstream>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace std;

const string DATA_FILENAME = "teenoi_data.txt";

// ---------------- Data models ----------------
struct Customer {
    string phone;   // key
    string name;
    int visitCount = 0;
    int usualPax = 0;
};

// One line of food/drink ordered at a table (name + quantity + unit price).
struct OrderItem {
    string name;
    int qty = 1;
    double price = 0.0;
};

struct Table {
    int id;
    int capacity;
    bool isAvailable = true;
    string customerName;
    int mergedNextId = -1; // linked-list pointer via id, -1 = none
    vector<OrderItem> orders; // items on this table; for a merged group they live on the head table
};

struct Branch {
    int id;
    string name;
    double x, y;
    vector<Table> tables;
    queue<Customer> waitlist;
    double salesToday = 0;
    int customersToday = 0;
    int tableTurnsToday = 0;
};

struct Action {
    string type;    // "SEAT" | "CHECKOUT"
    int branchIdx;
    int tableId;
    string prevCustomerName;
    bool prevAvailable;
    double billAmount = 0.0;
    vector<OrderItem> prevOrders; // restored when a CHECKOUT is undone
};

struct PromotionRule {
    string expression;      // infix condition, e.g. "pax >= 4"
    double percentDiscount = 0;
    double flatDiscount = 0;
};

struct CustNode {
    Customer data;
    CustNode* left = nullptr;
    CustNode* right = nullptr;
};

// plain (unbalanced) BST — O(log n) avg but O(n) worst case on
// sorted/adversarial insert order (also risks stack overflow via recursion
// at that depth). Upgrade path: AVL/red-black tree if insertion order can't
// be assumed random. Iterative here just to drop the recursion overhead.
CustNode* bstInsert(CustNode* root, const Customer& c) {
    if (!root) return new CustNode{c};
    CustNode* cur = root;
    while (true) {
        if (c.phone < cur->data.phone) {
            if (!cur->left) { cur->left = new CustNode{c}; return root; }
            cur = cur->left;
        } else if (c.phone > cur->data.phone) {
            if (!cur->right) { cur->right = new CustNode{c}; return root; }
            cur = cur->right;
        } else { cur->data = c; return root; } // update existing
    }
}

Customer* bstFind(CustNode* root, const string& phone) {
    while (root) {
        if (phone == root->data.phone) return &root->data;
        root = phone < root->data.phone ? root->left : root->right;
    }
    return nullptr;
}

void bstCollect(CustNode* root, vector<Customer>& list) {
    if (!root) return;
    bstCollect(root->left, list);
    list.push_back(root->data);
    bstCollect(root->right, list);
}

void bstClear(CustNode*& root) {
    if (!root) return;
    bstClear(root->left);
    bstClear(root->right);
    delete root;
    root = nullptr;
}

// ---------------- Input Validation & Helpers ----------------
bool validPhone(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

bool validField(const string& s) {
    if (s.empty()) return false;
    bool hasNonSpace = false;
    for (char c : s) {
        if (c == '|' || c == '\r' || c == '\n') return false;
        if (!isspace(static_cast<unsigned char>(c))) hasNonSpace = true;
    }
    return hasNonSpace;
}

int branchCapacity(const Branch& b) {
    int total = 0;
    for (const auto& t : b.tables) total += t.capacity;
    return total;
}

bool inAnyWaitlist(const vector<Branch>& branches, const string& phone) {
    for (const auto& b : branches) {
        queue<Customer> q = b.waitlist;
        while (!q.empty()) {
            if (q.front().phone == phone) return true;
            q.pop();
        }
    }
    return false;
}

// ---------------- Nearest branch (min-heap) ----------------
static double distSq(double x1, double y1, double x2, double y2) {
    return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2); // No square root (much faster)
}

// Direct min-heap extraction. Build the vector first and hand it to the
// priority_queue's range constructor: that heapifies in O(n) (std::make_heap)
// instead of n sequential push()es at O(log n) each — same heap, same top().
int nearestBranchIdx(vector<Branch>& branches, double cx, double cy) {
    vector<pair<double, int>> items;
    items.reserve(branches.size());
    for (int i = 0; i < (int)branches.size(); i++)
        items.emplace_back(distSq(cx, cy, branches[i].x, branches[i].y), i);
    priority_queue<pair<double, int>, vector<pair<double, int>>, greater<>> pq(greater<>(), move(items));
    return pq.empty() ? -1 : pq.top().second;
}

bool dataDirty = false; // skip disk write on exit if nothing changed

// ---------------- Table management & helpers (linked-list pointer chain) ----------------
Table* findTable(Branch& b, int id) {
    for (auto& t : b.tables) if (t.id == id) return &t;
    return nullptr;
}

int totalMergedCapacity(const Branch& b, int startId) {
    int cap = 0, id = startId;
    unordered_set<int> seen;
    while (id != -1 && seen.insert(id).second) {
        const Table* t = nullptr;
        for (const auto& item : b.tables) {
            if (item.id == id) { t = &item; break; }
        }
        if (!t) break;
        cap += t->capacity;
        id = t->mergedNextId;
    }
    return cap;
}

// ---------------- Free-table report & table orders ----------------
// Head of the merged chain that contains `id` (the table that owns the group's orders).
int chainHeadId(const Branch& b, int id) {
    unordered_set<int> seen;
    while (seen.insert(id).second) {
        bool found = false;
        for (const auto& t : b.tables) {
            if (t.mergedNextId == id) { id = t.id; found = true; break; }
        }
        if (!found) break;
    }
    return id;
}

int freeTableCount(const Branch& b) {
    int n = 0;
    for (const auto& t : b.tables) if (t.isAvailable) n++;
    return n;
}

int freeSeatCount(const Branch& b) {
    int n = 0;
    for (const auto& t : b.tables) if (t.isAvailable) n += t.capacity;
    return n;
}

// ':' and ';' are the separators of the saved order list, so item names may not contain them.
bool validItemName(const string& s) {
    return validField(s) && s.find(':') == string::npos && s.find(';') == string::npos;
}

double orderTotal(const vector<OrderItem>& orders) {
    double sum = 0;
    for (const auto& o : orders) sum += o.qty * o.price;
    return sum;
}

// Add an item to the order of the (merged) table group containing tableId.
// Only occupied tables can order; the same name+price just increases the quantity.
bool addOrderItem(Branch& b, int tableId, const OrderItem& item) {
    if (item.qty <= 0 || item.price < 0 || !validItemName(item.name)) return false;
    Table* head = findTable(b, chainHeadId(b, tableId));
    if (!head || head->isAvailable) return false;
    for (auto& o : head->orders) {
        if (o.name == item.name && fabs(o.price - item.price) < 1e-9) {
            o.qty += item.qty;
            dataDirty = true;
            return true;
        }
    }
    head->orders.push_back(item);
    dataDirty = true;
    return true;
}

// ---------------- Best-fit table (sort + binary search on effective capacities) ----------------
Table* findBestFitTable(Branch& b, int pax) {
    // Collect all follower table IDs (tables pointed to by another merged table)
    unordered_set<int> followers;
    for (const auto& t : b.tables) {
        if (t.mergedNextId != -1) {
            followers.insert(t.mergedNextId);
        }
    }

    struct TableCandidate {
        int effCap;
        Table* table;
    };
    vector<TableCandidate> avail;
    avail.reserve(b.tables.size());

    for (auto& t : b.tables) {
        // Skip follower tables (they are physically part of their head table)
        if (followers.count(t.id)) continue;

        // Check if all tables in this chain are currently available
        int curId = t.id;
        bool allAvail = true;
        int effCap = 0;
        unordered_set<int> seen;
        while (curId != -1 && seen.insert(curId).second) {
            Table* cur = findTable(b, curId);
            if (!cur || !cur->isAvailable) { allAvail = false; break; }
            effCap += cur->capacity;
            curId = cur->mergedNextId;
        }

        if (allAvail && effCap > 0) {
            avail.push_back({effCap, &t});
        }
    }

    sort(avail.begin(), avail.end(), [](const TableCandidate& a, const TableCandidate& c) {
        return a.effCap < c.effCap;
    });

    auto it = lower_bound(avail.begin(), avail.end(), pax,
                           [](const TableCandidate& cand, int p) { return cand.effCap < p; });
    return it != avail.end() ? it->table : nullptr;
}

bool mergeTables(Branch& b, int id1, int id2) {
    if (id1 == id2) return false;
    Table* t1 = findTable(b, id1);
    Table* t2 = findTable(b, id2);
    if (!t1 || !t2) return false;

    // Both tables must be available and empty
    if (!t1->isAvailable || !t2->isAvailable || !t1->customerName.empty() || !t2->customerName.empty())
        return false;

    // Find the head of chain containing id1 (predecessors)
    int head1 = id1;
    unordered_set<int> seen1;
    while (seen1.insert(head1).second) {
        bool foundPred = false;
        for (const auto& t : b.tables) {
            if (t.mergedNextId == head1) {
                head1 = t.id;
                foundPred = true;
                break;
            }
        }
        if (!foundPred) break;
    }

    // Find the head of chain containing id2
    int head2 = id2;
    unordered_set<int> seen2;
    while (seen2.insert(head2).second) {
        bool foundPred = false;
        for (const auto& t : b.tables) {
            if (t.mergedNextId == head2) {
                head2 = t.id;
                foundPred = true;
                break;
            }
        }
        if (!foundPred) break;
    }

    // Cannot merge if they are already in the same merged component (prevents cycles)
    if (head1 == head2) return false;

    // id2 must be the head of its chain (cannot merge into middle of an existing chain)
    if (head2 != id2) return false;

    // Verify all tables in chain 1 are available and find tail of chain 1
    Table* cur1 = findTable(b, head1);
    unordered_set<int> chain1;
    while (cur1 && chain1.insert(cur1->id).second) {
        if (!cur1->isAvailable || !cur1->customerName.empty()) return false;
        if (cur1->mergedNextId == -1) break;
        cur1 = findTable(b, cur1->mergedNextId);
    }
    if (!cur1) return false;

    // Verify all tables in chain 2 are available
    Table* cur2 = findTable(b, head2);
    unordered_set<int> chain2;
    while (cur2 && chain2.insert(cur2->id).second) {
        if (!cur2->isAvailable || !cur2->customerName.empty()) return false;
        if (cur2->mergedNextId == -1) break;
        cur2 = findTable(b, cur2->mergedNextId);
    }

    // Link tail of chain 1 to head of chain 2
    cur1->mergedNextId = head2;
    dataDirty = true;
    return true;
}

bool unmergeTables(Branch& b, int tableId) {
    Table* target = findTable(b, tableId);
    if (!target) return false;

    // Build undirected graph of merged links
    unordered_map<int, vector<int>> adj;
    bool hasAnyMerge = false;
    for (const auto& t : b.tables) {
        if (t.mergedNextId != -1) {
            adj[t.id].push_back(t.mergedNextId);
            adj[t.mergedNextId].push_back(t.id);
            hasAnyMerge = true;
        }
    }
    if (!hasAnyMerge || adj.find(tableId) == adj.end() || adj[tableId].empty()) {
        return false; // Table is standalone, not merged
    }

    // BFS to find all tables in this merged component
    unordered_set<int> comp;
    queue<int> q;
    q.push(tableId);
    comp.insert(tableId);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : adj[u]) {
            if (comp.insert(v).second) {
                q.push(v);
            }
        }
    }

    if (comp.size() <= 1) return false;

    // Check if any table in component has seated customers
    for (int tid : comp) {
        Table* t = findTable(b, tid);
        if (t && (!t->isAvailable || !t->customerName.empty())) {
            return false;
        }
    }

    // Reset mergedNextId for all tables in component
    for (int tid : comp) {
        Table* t = findTable(b, tid);
        if (t) t->mergedNextId = -1;
    }
    dataDirty = true;
    return true;
}

// ---------------- Undo (stack) & Table Operations ----------------
stack<Action> history;

void seatCustomer(Branch& b, Table* t, const Customer& c) {
    if (!t) return;
    history.push({"SEAT", b.id, t->id, t->customerName, t->isAvailable, 0.0});
    // Mark the whole chain occupied
    int curId = t->id;
    unordered_set<int> seen;
    while (curId != -1 && seen.insert(curId).second) {
        Table* cur = findTable(b, curId);
        if (!cur) break;
        cur->isAvailable = false;
        cur->customerName = c.name;
        curId = cur->mergedNextId;
    }
    b.customersToday++;
    dataDirty = true;
}

// Seat waitlisted parties in FIFO order onto best-fit free tables; returns how many were seated.
// ponytail: strict FIFO — a party bigger than every free table blocks those behind it.
// Upgrade path: skip to the first party that fits (or clear the queue).
int pullWaitlist(Branch& b) {
    int n = 0;
    while (!b.waitlist.empty()) {
        Table* t = findBestFitTable(b, b.waitlist.front().usualPax);
        if (!t) break;
        Customer c = b.waitlist.front();
        b.waitlist.pop();
        seatCustomer(b, t, c);
        n++;
    }
    return n;
}

// Checkout: free the table, record sales, reseat front if fits, then pull waitlist; returns count of waitlisted parties seated.
int checkoutTable(Branch& b, Table* t, double amount = 0.0) {
    if (!t) return 0;
    // Find the head of the chain containing t->id (in case staff entered follower table ID)
    int headId = t->id;
    unordered_set<int> seenH;
    while (seenH.insert(headId).second) {
        bool foundPred = false;
        for (const auto& other : b.tables) {
            if (other.mergedNextId == headId) {
                headId = other.id;
                foundPred = true;
                break;
            }
        }
        if (!foundPred) break;
    }
    Table* headTable = findTable(b, headId);
    if (!headTable) headTable = t;

    history.push({"CHECKOUT", b.id, headTable->id, headTable->customerName, headTable->isAvailable, amount, headTable->orders});

    // Free all tables in the chain
    int curId = headTable->id;
    unordered_set<int> seenF;
    while (curId != -1 && seenF.insert(curId).second) {
        Table* cur = findTable(b, curId);
        if (!cur) break;
        cur->isAvailable = true;
        cur->customerName.clear();
        cur->orders.clear(); // the bill is settled, so the table's order list is cleared
        curId = cur->mergedNextId;
    }
    b.tableTurnsToday++;
    b.salesToday += amount;
    dataDirty = true;

    int seatedCount = 0;
    // Reseat front of waitlist into this newly freed table unit if they fit
    if (!b.waitlist.empty()) {
        int cap = totalMergedCapacity(b, headTable->id);
        if (b.waitlist.front().usualPax <= cap) {
            Customer frontC = b.waitlist.front();
            b.waitlist.pop();
            seatCustomer(b, headTable, frontC);
            seatedCount++;
        }
    }

    seatedCount += pullWaitlist(b);
    return seatedCount;
}

// Drop every waiting party; returns how many were removed. Not undoable (Action has no queue snapshot).
int clearWaitlist(Branch& b) {
    int n = (int)b.waitlist.size();
    b.waitlist = queue<Customer>();
    if (n) dataDirty = true;
    return n;
}

bool undoLast(vector<Branch>& branches) {
    if (history.empty()) return false;
    Action a = history.top(); history.pop();
    for (auto& b : branches) {
        if (b.id != a.branchIdx) continue;
        Table* t = findTable(b, a.tableId);
        if (!t) return false;
        if (a.type == "SEAT") {
            if (!t->isAvailable) b.customersToday = max(0, b.customersToday - 1);
            int curId = t->id;
            unordered_set<int> seen;
            while (curId != -1 && seen.insert(curId).second) {
                Table* cur = findTable(b, curId);
                if (!cur) break;
                cur->isAvailable = a.prevAvailable;
                cur->customerName = a.prevCustomerName;
                cur->orders.clear();
                curId = cur->mergedNextId;
            }
        }
        if (a.type == "CHECKOUT") {
            if (t->isAvailable) b.tableTurnsToday = max(0, b.tableTurnsToday - 1);
            b.salesToday = max(0.0, b.salesToday - a.billAmount);
            int curId = t->id;
            unordered_set<int> seen;
            while (curId != -1 && seen.insert(curId).second) {
                Table* cur = findTable(b, curId);
                if (!cur) break;
                cur->isAvailable = a.prevAvailable;
                cur->customerName = a.prevCustomerName;
                curId = cur->mergedNextId;
            }
            t->orders = a.prevOrders; // bring back the items that were on the table
        }
        dataDirty = true;
        return true;
    }
    return false;
}

// ---------------- Branch ranking (merge sort) ----------------
// Same merge sort, one scratch buffer for the whole sort instead of a fresh
// vector allocated at every merge step (was O(n log n) allocations).
static void mergeSortBranchesImpl(vector<Branch*>& v, int l, int r, vector<Branch*>& tmp) {
    if (l >= r) return;
    int m = (l + r) / 2;
    mergeSortBranchesImpl(v, l, m, tmp);
    mergeSortBranchesImpl(v, m + 1, r, tmp);
    int i = l, j = m + 1, k = l;
    while (i <= m && j <= r) tmp[k++] = (v[i]->customersToday >= v[j]->customersToday) ? v[i++] : v[j++];
    while (i <= m) tmp[k++] = v[i++];
    while (j <= r) tmp[k++] = v[j++];
    for (int x = l; x <= r; x++) v[x] = tmp[x];
}

void mergeSortBranches(vector<Branch*>& v, int l, int r) {
    if (v.empty()) return;
    vector<Branch*> tmp(v.size());
    mergeSortBranchesImpl(v, l, r, tmp);
}

// ---------------- Promotion (infix -> postfix -> eval) ----------------
static int precedence(const string& op) {
    if (op == "||") return 1;
    if (op == "&&") return 2;
    if (op == "==" || op == "!=") return 3;
    if (op == ">=" || op == "<=" || op == ">" || op == "<") return 4;
    if (op == "+" || op == "-") return 5;
    if (op == "*" || op == "/") return 6;
    return 0;
}

vector<string> tokenize(const string& expr) {
    vector<string> tokens;
    int i = 0, n = (int)expr.size();
    while (i < n) {
        if (isspace((unsigned char)expr[i])) { i++; continue; }
        if (isdigit((unsigned char)expr[i]) || expr[i] == '.') {
            int j = i;
            while (j < n && (isdigit((unsigned char)expr[j]) || expr[j] == '.')) j++;
            tokens.push_back(expr.substr(i, j - i)); i = j;
        } else if (isalpha((unsigned char)expr[i])) {
            int j = i;
            while (j < n && isalnum((unsigned char)expr[j])) j++;
            tokens.push_back(expr.substr(i, j - i)); i = j;
        } else if (expr[i] == '(' || expr[i] == ')') {
            tokens.push_back(string(1, expr[i])); i++;
        } else {
            string two = expr.substr(i, min(2, n - i));
            if (two == ">=" || two == "<=" || two == "==" || two == "!=" || two == "&&" || two == "||") {
                tokens.push_back(two); i += 2;
            } else {
                tokens.push_back(string(1, expr[i])); i++;
            }
        }
    }
    return tokens;
}

vector<string> infixToPostfix(const vector<string>& tokens, bool* ok = nullptr) {
    if (ok) *ok = true;
    vector<string> output;
    stack<string> ops;
    for (auto& tok : tokens) {
        if (isdigit((unsigned char)tok[0]) || (tok.size() > 1 && tok[0] == '.') || isalpha((unsigned char)tok[0])) {
            output.push_back(tok);
        } else if (tok == "(") {
            ops.push(tok);
        } else if (tok == ")") {
            bool foundOpen = false;
            while (!ops.empty()) {
                if (ops.top() == "(") { foundOpen = true; ops.pop(); break; }
                output.push_back(ops.top());
                ops.pop();
            }
            if (!foundOpen && ok) *ok = false;
        } else {
            if (precedence(tok) == 0 && ok) *ok = false;
            while (!ops.empty() && ops.top() != "(" && precedence(ops.top()) >= precedence(tok)) {
                output.push_back(ops.top()); ops.pop();
            }
            ops.push(tok);
        }
    }
    while (!ops.empty()) {
        if (ops.top() == "(" && ok) *ok = false;
        else output.push_back(ops.top());
        ops.pop();
    }
    return output;
}

double evalPostfix(const vector<string>& postfix, map<string, double>& vars, bool* ok = nullptr) {
    if (ok) *ok = true;
    stack<double> st;
    for (auto& tok : postfix) {
        if (isdigit((unsigned char)tok[0]) || (tok.size() > 1 && tok[0] == '.')) {
            try {
                st.push(stod(tok));
            } catch (...) {
                if (ok) *ok = false;
                return 0;
            }
        } else if (isalpha((unsigned char)tok[0])) {
            st.push(vars.count(tok) ? vars[tok] : 0);
        } else {
            if (st.size() < 2) {
                if (ok) *ok = false;
                return 0;
            }
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
            double r = 0;
            if (tok == "+") r = a + b;
            else if (tok == "-") r = a - b;
            else if (tok == "*") r = a * b;
            else if (tok == "/") {
                if (fabs(b) < 1e-9) { if (ok) *ok = false; return 0; }
                r = a / b;
            }
            else if (tok == ">=") r = a >= b;
            else if (tok == "<=") r = a <= b;
            else if (tok == ">") r = a > b;
            else if (tok == "<") r = a < b;
            else if (tok == "==") r = (fabs(a - b) < 1e-9);
            else if (tok == "!=") r = (fabs(a - b) >= 1e-9);
            else if (tok == "&&") r = (a != 0 && b != 0);
            else if (tok == "||") r = (a != 0 || b != 0);
            else {
                if (ok) *ok = false;
                return 0;
            }
            st.push(r);
        }
    }
    if (st.size() != 1) {
        if (ok) *ok = false;
        return 0;
    }
    return st.top();
}

double applyPromotion(const PromotionRule& promo, int pax, double total, bool* ok = nullptr) {
    if (ok) *ok = true;
    map<string, double> vars = {{"pax", (double)pax}, {"total", total}};
    auto tokens = tokenize(promo.expression);
    if (tokens.empty()) {
        if (ok) *ok = false;
        return total;
    }
    bool pOk = true;
    auto postfix = infixToPostfix(tokens, &pOk);
    if (!pOk) {
        if (ok) *ok = false;
        return total;
    }
    bool eOk = true;
    double cond = evalPostfix(postfix, vars, &eOk);
    if (!eOk) {
        if (ok) *ok = false;
        return total;
    }
    if (cond == 0) return total;
    double result = total;
    if (promo.percentDiscount > 0) result -= result * promo.percentDiscount / 100.0;
    if (promo.flatDiscount > 0) result -= promo.flatDiscount;
    return max(0.0, result);
}

// ---------------- Dashboard stats (recursion) ----------------
struct Stats { int totalCustomers = 0; int totalTables = 0; double totalSales = 0; int freeTables = 0; };

Stats aggregateStats(vector<Branch>& branches, int idx = 0) {
    if (idx >= (int)branches.size()) return Stats{};
    Stats s = aggregateStats(branches, idx + 1);
    s.totalCustomers += branches[idx].customersToday;
    s.totalTables += (int)branches[idx].tables.size();
    s.freeTables += freeTableCount(branches[idx]);
    s.totalSales += branches[idx].salesToday;
    return s;
}

// ---------------- Preset menu & HTML table map ----------------
// Preset menu (staff can also type a custom item). Prices in baht.
static const vector<OrderItem> MENU_ITEMS = {
    {"เนื้อวัวสไลซ์", 1, 189}, {"หมูสไลซ์", 1, 129}, {"กุ้งสด", 1, 159},
    {"ผักรวม", 1, 59},        {"เห็ดรวม", 1, 69},   {"วุ้นเส้น", 1, 39},
    {"ไข่ไก่", 1, 15},         {"น้ำจิ้มสุกี้", 1, 20}, {"น้ำดื่ม", 1, 20}
};

const string HTML_FILENAME = "teenoi_map.html";

// Escape text before putting it into HTML (customer names / item names are typed by staff).
static string htmlEsc(const string& s) {
    string o;
    o.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            case '"': o += "&quot;"; break;
            case '\'': o += "&#39;"; break;
            default: o += c;
        }
    }
    return o;
}

static string money2(double v) {
    ostringstream os;
    os << fixed << setprecision(2) << v;
    return os.str();
}

// Writes a self-contained web page: green = free table, red = busy table (with the items on it),
// plus the waiting list and the menu. The page refreshes itself every few seconds, and
// saveData() rewrites it after every change, so it works as a live view in a browser.
bool exportHtmlMap(const string& filename, const vector<Branch>& branches) {
    ofstream o(filename);
    if (!o.is_open()) return false;

    int allFree = 0, allTables = 0, allSeats = 0, allWait = 0;
    for (const auto& b : branches) {
        allFree += freeTableCount(b);
        allTables += (int)b.tables.size();
        allSeats += freeSeatCount(b);
        allWait += (int)b.waitlist.size();
    }
    char stamp[16] = "";
    time_t now = time(nullptr);
    strftime(stamp, sizeof(stamp), "%H:%M:%S", localtime(&now));

    o << R"HTML(<!doctype html>
<html lang="th"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="4">
<title>Teenoi Suki - ผังโต๊ะ</title>
<style>
:root{--bg:#0b1220;--panel:#111a2e;--line:#1f2b45;--text:#e8eefc;--mut:#8b9bbd;--gold:#f2b705}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font-family:"Segoe UI",Tahoma,system-ui,sans-serif}
header{display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between;padding:14px 20px;background:#0e1730;border-bottom:1px solid var(--line);position:sticky;top:0;z-index:5}
h1{font-size:20px;margin:0}
h1 span{color:var(--mut);font-weight:400;margin-left:8px;font-size:15px}
.pills{display:flex;gap:8px;flex-wrap:wrap}
.pill{padding:5px 12px;border-radius:999px;font-size:13px;border:1px solid var(--line);background:#0b1220}
.pill.g{color:#46d17a;border-color:#1f6b3d;background:#0f2a1c}
.pill.r{color:#ff7a7a;border-color:#7a2a2a;background:#2a1212}
.pill.y{color:var(--gold);border-color:#7a5c0a;background:#2a2108}
.wrap{display:grid;grid-template-columns:1fr 320px;gap:16px;padding:16px 20px;max-width:1500px;margin:0 auto}
@media(max-width:1000px){.wrap{grid-template-columns:1fr}}
.branch{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:14px;margin-bottom:16px}
.bh{display:flex;flex-wrap:wrap;justify-content:space-between;align-items:center;gap:8px;margin-bottom:12px}
.bh h2{margin:0;font-size:17px}
.floor{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:12px;padding:12px;border-radius:10px;background-color:#0a0f1d;background-image:linear-gradient(var(--line) 1px,transparent 1px),linear-gradient(90deg,var(--line) 1px,transparent 1px);background-size:24px 24px}
.tbl{border-radius:12px;padding:10px;display:flex;flex-direction:column;gap:4px;min-height:150px;border:2px solid}
.tbl.wide{grid-column:span 2}
.tbl.free{background:linear-gradient(160deg,#2fa75a,#1f7a41);border-color:#46d17a}
.tbl.busy{background:linear-gradient(160deg,#d64545,#9c2b2b);border-color:#ff7a7a}
.tno{font-size:22px;font-weight:700}
.cap{font-size:12px;opacity:.9}
.seats{display:flex;flex-wrap:wrap;gap:4px;margin:2px 0}
.seats i{width:10px;height:10px;border-radius:50%;background:#fff;opacity:.95}
.tbl.free .seats i{background:transparent;border:2px solid #d9ffe8}
.st{font-size:12px;font-weight:700;letter-spacing:.04em}
.lk{font-size:11px;background:rgba(0,0,0,.28);border-radius:6px;padding:2px 6px;align-self:flex-start}
.who{font-size:13px;font-weight:600;word-break:break-word}
.tbl ul{list-style:none;margin:2px 0;padding:0;font-size:12px;display:flex;flex-direction:column;gap:2px}
.tbl li{display:flex;justify-content:space-between;gap:8px;background:rgba(0,0,0,.22);padding:2px 6px;border-radius:5px}
.tot{font-size:12px;font-weight:700;margin-top:auto;text-align:right}
.none{font-size:12px;opacity:.85}
aside .card{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:14px;margin-bottom:16px}
aside h3{margin:0 0 10px;font-size:15px}
aside h4{margin:10px 0 6px;font-size:13px;color:var(--mut);font-weight:600}
.q{display:flex;justify-content:space-between;gap:8px;padding:6px 8px;border-radius:8px;background:#0b1220;margin-bottom:6px;font-size:13px}
.q small{color:var(--mut);display:block}
.empty{color:var(--mut);font-size:13px}
.m{display:flex;justify-content:space-between;gap:8px;padding:5px 0;border-bottom:1px dashed var(--line);font-size:14px}
.pot{text-align:center;margin-bottom:6px}
.stamp{color:var(--mut);font-size:12px}
</style></head><body>
<header><h1>Teenoi Suki<span>ผังโต๊ะ (Table Map)</span></h1><div class="pills">)HTML";
    o << "<span class=\"pill g\">โต๊ะว่าง " << allFree << " / " << allTables << "</span>"
      << "<span class=\"pill r\">มีลูกค้า " << (allTables - allFree) << "</span>"
      << "<span class=\"pill y\">คิวรอ " << allWait << "</span>"
      << "<span class=\"pill\">ที่นั่งว่างรวม " << allSeats << "</span>"
      << "<span class=\"stamp\">อัปเดตล่าสุด " << stamp << "</span>";
    o << "</div></header>\n<div class=\"wrap\"><main>\n";

    for (const auto& b : branches) {
        o << "<section class=\"branch\"><div class=\"bh\"><h2>" << htmlEsc(b.name) << "</h2><div class=\"pills\">"
          << "<span class=\"pill g\">ว่าง " << freeTableCount(b) << " / " << b.tables.size() << " โต๊ะ</span>"
          << "<span class=\"pill\">ที่นั่งว่าง " << freeSeatCount(b) << "</span>"
          << "<span class=\"pill y\">คิวรอ " << b.waitlist.size() << "</span>"
          << "</div></div>\n<div class=\"floor\">\n";
        for (const auto& t : b.tables) {
            bool isFree = t.isAvailable;
            int head = chainHeadId(b, t.id);
            o << "<div class=\"tbl " << (isFree ? "free" : "busy") << (t.capacity >= 6 ? " wide" : "") << "\">"
              << "<div class=\"tno\">T" << t.id << "</div>"
              << "<div class=\"cap\">" << t.capacity << " ที่นั่ง</div><div class=\"seats\">";
            for (int s = 0; s < t.capacity; s++) o << "<i></i>";
            o << "</div><div class=\"st\">" << (isFree ? "FREE · ว่าง" : "BUSY · มีลูกค้า") << "</div>";
            if (t.mergedNextId != -1) o << "<div class=\"lk\">เชื่อม → T" << t.mergedNextId << "</div>";
            else if (head != t.id) o << "<div class=\"lk\">ใช้ร่วมกับ T" << head << "</div>";
            if (!isFree) {
                o << "<div class=\"who\">" << htmlEsc(t.customerName) << "</div>";
                if (head == t.id) {
                    if (t.orders.empty()) {
                        o << "<div class=\"none\">ยังไม่มีรายการอาหาร</div>";
                    } else {
                        o << "<ul>";
                        for (const auto& it : t.orders)
                            o << "<li><span>" << htmlEsc(it.name) << " ×" << it.qty << "</span><b>"
                              << money2(it.qty * it.price) << "</b></li>";
                        o << "</ul><div class=\"tot\">รวม " << money2(orderTotal(t.orders)) << " บาท</div>";
                    }
                }
            }
            o << "</div>\n";
        }
        o << "</div></section>\n";
    }

    o << "</main><aside>\n<div class=\"card\"><h3>คิวรอ (Waiting List)</h3>";
    bool anyWait = false;
    for (const auto& b : branches) {
        if (b.waitlist.empty()) continue;
        anyWait = true;
        o << "<h4>" << htmlEsc(b.name) << "</h4>";
        queue<Customer> q = b.waitlist;
        for (int i = 1; !q.empty(); i++, q.pop())
            o << "<div class=\"q\"><div>" << i << ". " << htmlEsc(q.front().name) << "<small>"
              << htmlEsc(q.front().phone) << "</small></div><b>" << q.front().usualPax << " ท่าน</b></div>";
    }
    if (!anyWait) o << "<div class=\"empty\">ไม่มีคิวรอ</div>";
    o << "</div>\n<div class=\"card\"><div class=\"pot\">"
         "<svg viewBox=\"0 0 120 64\" width=\"96\" height=\"52\">"
         "<path d=\"M20 27h80v10a30 25 0 0 1-30 25H50A30 25 0 0 1 20 37z\" fill=\"#d64545\"/>"
         "<rect x=\"14\" y=\"23\" width=\"92\" height=\"6\" rx=\"3\" fill=\"#f2b705\"/>"
         "<path d=\"M45 16c-4-5 4-7 0-12M60 16c-4-5 4-7 0-12M75 16c-4-5 4-7 0-12\" "
         "stroke=\"#8b9bbd\" fill=\"none\" stroke-width=\"2\" stroke-linecap=\"round\"/></svg></div>"
         "<h3>เมนูอาหาร (Menu)</h3>";
    for (const auto& m : MENU_ITEMS)
        o << "<div class=\"m\"><span>" << htmlEsc(m.name) << "</span><b>" << money2(m.price) << "</b></div>";
    o << "</div>\n</aside></div></body></html>\n";
    return true;
}

// Open the page in the default browser (best effort, never fatal).
static void openInBrowser(const string& path) {
#ifdef _WIN32
    string cmd = "start \"\" \"" + path + "\"";
#elif defined(__APPLE__)
    string cmd = "open \"" + path + "\" >/dev/null 2>&1 &";
#else
    string cmd = "xdg-open \"" + path + "\" >/dev/null 2>&1 &";
#endif
    int rc = system(cmd.c_str());
    (void)rc;
}

// ---------------- Save & Load (File Storage) ----------------
// Orders are stored in one extra field per table line: name:qty:price;name:qty:price;...
// Older save files without this field still load (the table simply has no orders).
static string encodeOrders(const vector<OrderItem>& orders) {
    ostringstream os;
    os << setprecision(12);
    for (size_t i = 0; i < orders.size(); i++) {
        if (i) os << ';';
        os << orders[i].name << ':' << orders[i].qty << ':' << orders[i].price;
    }
    return os.str();
}

static vector<OrderItem> decodeOrders(string_view sv) {
    vector<OrderItem> out;
    size_t start = 0;
    while (start < sv.size()) {
        size_t end = sv.find(';', start);
        if (end == string_view::npos) end = sv.size();
        string item(sv.substr(start, end - start));
        start = end + 1;
        size_t p2 = item.rfind(':');
        if (p2 == string::npos || p2 == 0) continue;
        size_t p1 = item.rfind(':', p2 - 1);
        if (p1 == string::npos || p1 == 0) continue;
        OrderItem o;
        o.name = item.substr(0, p1);
        o.qty = atoi(item.substr(p1 + 1, p2 - p1 - 1).c_str());
        o.price = atof(item.substr(p2 + 1).c_str());
        if (o.qty > 0 && o.price >= 0 && !o.name.empty()) out.push_back(o);
    }
    return out;
}

bool saveData(const string& filename, const vector<Branch>& branches, CustNode* custRoot) {
    ofstream ofs(filename);
    if (!ofs.is_open()) return false;

    // 1. Customers
    vector<Customer> custList;
    bstCollect(custRoot, custList);
    ofs << "[CUSTOMERS] " << custList.size() << "\n";
    for (const auto& c : custList) {
        ofs << c.phone << "|" << c.name << "|" << c.visitCount << "|" << c.usualPax << "\n";
    }

    // 2. Branches
    ofs << "[BRANCHES] " << branches.size() << "\n";
    for (const auto& b : branches) {
        ofs << b.id << "|" << b.name << "|" << b.x << "|" << b.y << "|"
            << b.salesToday << "|" << b.customersToday << "|" << b.tableTurnsToday << "\n";

        // Tables
        ofs << "[TABLES] " << b.tables.size() << "\n";
        for (const auto& t : b.tables) {
            ofs << t.id << "|" << t.capacity << "|" << (t.isAvailable ? 1 : 0) << "|"
                << t.mergedNextId << "|" << t.customerName << "|" << encodeOrders(t.orders) << "\n";
        }

        queue<Customer> qCopy = b.waitlist;
        ofs << "[WAITLIST] " << qCopy.size() << "\n";
        while (!qCopy.empty()) {
            const Customer& wc = qCopy.front();
            ofs << wc.phone << "|" << wc.name << "|" << wc.visitCount << "|" << wc.usualPax << "\n";
            qCopy.pop();
        }
    }
    ofs.flush();
    // Keep the web page in step with the saved data (only for the real data file, not test files).
    if (filename == DATA_FILENAME) exportHtmlMap(HTML_FILENAME, branches);
    return true;
}

bool loadData(const string& filename, vector<Branch>& branches, CustNode*& custRoot) {
    ifstream ifs(filename);
    if (!ifs.is_open()) return false;
    // 32KB buffer reduces syscalls from ~1/line to ~1/chunk
    char fileBuf[32768];
    ifs.rdbuf()->pubsetbuf(fileBuf, sizeof(fileBuf));

    auto getCleanLine = [](ifstream& f, string& out) -> bool {
        if (!getline(f, out)) return false;
        if (!out.empty() && out.back() == '\r') out.pop_back();
        return true;
    };

    // Strip UTF-8 BOM if present at start of file
    if (!ifs.eof()) {
        int c = ifs.peek();
        if (c == 0xEF) {
            char b[3];
            ifs.read(b, 3);
            if (!((unsigned char)b[0] == 0xEF && (unsigned char)b[1] == 0xBB && (unsigned char)b[2] == 0xBF)) {
                for (int i = (int)ifs.gcount() - 1; i >= 0; --i) ifs.putback(b[i]);
            }
        }
    }

    // string_view split — zero heap alloc per field.
    // Relies on `line` staying alive while views are used.
    auto splitFast = [](const string& s, vector<string_view>& out) {
        out.clear();
        size_t start = 0, end;
        while ((end = s.find('|', start)) != string::npos) {
            out.emplace_back(s.data() + start, end - start);
            start = end + 1;
        }
        out.emplace_back(s.data() + start, s.size() - start);
    };

    // fast int parse from string_view (no exception overhead vs stoi)
    auto svToInt = [](string_view sv) -> int {
        if (sv.empty()) return 0;
        // atoi needs null-terminated; sv points into `line` which is,
        // but the view may end before the null. For sub-views ending at '|',
        // the character after sv is '|' or '\0' — but atoi stops at non-digit
        // anyway, so this is safe for numeric fields.
        return atoi(sv.data());
    };
    auto svToDouble = [](string_view sv) -> double {
        if (sv.empty()) return 0.0;
        return atof(sv.data());
    };

    string line;
    vector<string_view> parts;
    parts.reserve(10);

    // parse "[TAG] N" headers without stringstream (avoids heap alloc)
    auto parseHeader = [](const string& s, const char* expected) -> int {
        size_t sp = s.find(' ');
        if (sp == string::npos) return -1;
        if (s.compare(0, sp, expected) != 0) return -1;
        return atoi(s.c_str() + sp + 1);
    };

    // 1. Customers
    if (!getCleanLine(ifs, line)) return false;
    int count = parseHeader(line, "[CUSTOMERS]");
    if (count < 0) return false;

    // saveData writes customers in sorted (in-order) order, so
    // reinserting them in file order would rebuild the exact same degenerate
    // BST on every single restart (not just once — this is the steady-state
    // case for a long-running restaurant, not an edge case). Collect first,
    // shuffle, then insert, so lookups stay O(log n) on average after a
    // reload. If this file could ever come from an untrusted source, a
    // shuffle is a mitigation, not a guarantee — swap the plain BST for an
    // AVL/red-black tree if that trust boundary applies here.
    bstClear(custRoot);
    vector<Customer> custList;
    custList.reserve(count);
    for (int i = 0; i < count; i++) {
        if (!getCleanLine(ifs, line)) break;
        splitFast(line, parts);
        if (parts.size() >= 4 && !parts[0].empty()) {
            custList.push_back({string(parts[0]), string(parts[1]), svToInt(parts[2]), svToInt(parts[3])});
        }
    }
    if (custList.size() > 1) {
        static mt19937 loadRng{(unsigned)chrono::steady_clock::now().time_since_epoch().count()};
        shuffle(custList.begin(), custList.end(), loadRng);
    }
    for (auto& c : custList) custRoot = bstInsert(custRoot, c);

    // 2. Branches
    if (!getCleanLine(ifs, line)) return false;
    count = parseHeader(line, "[BRANCHES]");
    if (count < 0) return false;

    vector<Branch> loadedBranches;
    loadedBranches.reserve(count);
    for (int i = 0; i < count; i++) {
        if (!getCleanLine(ifs, line)) break;
        splitFast(line, parts);
        if (parts.size() < 7) continue;

        Branch b;
        b.id = svToInt(parts[0]);
        b.name = string(parts[1]);
        b.x = svToDouble(parts[2]);
        b.y = svToDouble(parts[3]);
        b.salesToday = svToDouble(parts[4]);
        b.customersToday = svToInt(parts[5]);
        b.tableTurnsToday = svToInt(parts[6]);

        // Tables
        if (!getCleanLine(ifs, line)) break;
        int tCount = parseHeader(line, "[TABLES]");
        if (tCount < 0) tCount = 0;
        b.tables.reserve(tCount);
        for (int j = 0; j < tCount; j++) {
            if (!getCleanLine(ifs, line)) break;
            splitFast(line, parts);
            if (parts.size() < 5) continue;
            Table t;
            t.id = svToInt(parts[0]);
            t.capacity = svToInt(parts[1]);
            t.isAvailable = (parts[2] == "1");
            t.mergedNextId = svToInt(parts[3]);
            t.customerName = string(parts[4]);
            if (parts.size() >= 6) t.orders = decodeOrders(parts[5]);
            b.tables.push_back(t);
        }

        // Waitlist
        if (!getCleanLine(ifs, line)) break;
        int wCount = parseHeader(line, "[WAITLIST]");
        if (wCount < 0) wCount = 0;
        for (int j = 0; j < wCount; j++) {
            if (!getCleanLine(ifs, line)) break;
            splitFast(line, parts);
            if (parts.size() < 4) continue;
            Customer wc{string(parts[0]), string(parts[1]), svToInt(parts[2]), svToInt(parts[3])};
            b.waitlist.push(wc);
        }
        loadedBranches.push_back(move(b));
    }

    if (!loadedBranches.empty()) {
        branches = move(loadedBranches);
    }
    return true;
}

// !!!!!!======================================================================!!!!!!
// Benchmark Mode (Measure Time Complexity with 3 Data Sizes)
// !!!!!!======================================================================!!!!!!
static void benchmarkPhase(int n) {
    cout << ">> Generating dataset size N = " << n << "...\n";
    mt19937 rng(42);
    uniform_int_distribution<int> pDist(1, 10);

    // 1. BST Insert (Empty to N)
    // Root cause of the old N=10000 run taking ~900ms: phones were inserted
    // in strictly increasing order, which degenerates an unbalanced BST into
    // a linked list (O(n^2) total). Real phone numbers arrive in no
    // particular order, so shuffle the insert order to measure the realistic
    // (average) case instead of the adversarial one.
    vector<int> order(n);
    iota(order.begin(), order.end(), 0);
    shuffle(order.begin(), order.end(), rng);
    auto t1 = chrono::high_resolution_clock::now();
    CustNode* root = nullptr;
    for (int idx : order) {
        string phone = to_string(800000000 + idx);
        root = bstInsert(root, {phone, "Cust" + to_string(idx), 1, pDist(rng)});
    }
    auto t2 = chrono::high_resolution_clock::now();
    
    // 2. Nearest Branch Min-Heap (N branches)
    vector<Branch> branches;
    branches.reserve(n);
    for (int i = 0; i < n; i++) branches.push_back({i, "B"+to_string(i), (double)(rng()%100), (double)(rng()%100)});
    auto t3 = chrono::high_resolution_clock::now();
    int nearest = nearestBranchIdx(branches, 50, 50);
    auto t4 = chrono::high_resolution_clock::now();
    
    // 3. Best Fit Table (Sort + Binary Search on N tables)
    branches[0].tables.reserve(n);
    for (int i = 0; i < n; i++) branches[0].tables.push_back({i, pDist(rng), true, "", -1});
    auto t5 = chrono::high_resolution_clock::now();
    Table* fit = findBestFitTable(branches[0], 4);
    auto t6 = chrono::high_resolution_clock::now();
    
    // Output Results
    cout << "   - BST Insert (" << n << " items): " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << " us\n";
    cout << "   - Nearest Branch (Heap, " << n << " items): " << chrono::duration_cast<chrono::microseconds>(t4 - t3).count() << " us\n";
    cout << "   - Best Fit Table (Sort+BS, " << n << " items): " << chrono::duration_cast<chrono::microseconds>(t6 - t5).count() << " us\n\n";
    
    bstClear(root);
}

static void runBenchmark() {
    cout << "========================================================\n";
    cout << " Performance Benchmark (Testing Big-O empirical time)   \n";
    cout << "========================================================\n";
    benchmarkPhase(100);
    benchmarkPhase(1000);
    benchmarkPhase(10000);
    cout << "Benchmark complete.\n";
}

// !!!!!!======================================================================!!!!!!
// Self-check (run with --test). Smallest thing that fails if logic breaks.
// !!!!!!======================================================================!!!!!!
static void runSelfTest() {
    // Edge Cases (Empty & Single Data)
    CustNode* emptyRoot = nullptr;
    assert(bstFind(emptyRoot, "999") == nullptr); // Empty tree safe
    emptyRoot = bstInsert(emptyRoot, {"0899", "Single", 1, 2});
    assert(bstFind(emptyRoot, "0899") != nullptr); // Single data safe
    bstClear(emptyRoot);
    vector<Branch> emptyBranches;
    assert(nearestBranchIdx(emptyBranches, 0, 0) == -1); // Empty heap safe

    // Normal BST
    CustNode* root = nullptr;
    root = bstInsert(root, {"0899999999", "Somchai", 3, 4});
    root = bstInsert(root, {"0811111111", "Nid", 1, 2});
    root = bstInsert(root, {"0922222222", "Bee", 5, 6});
    assert(bstFind(root, "0811111111")->name == "Nid");
    assert(bstFind(root, "0000000000") == nullptr);

    // nearest branch
    vector<Branch> branches = {
        {1, "A", 0, 0}, {2, "B", 10, 10}, {3, "C", 9, 9}
    };
    assert(nearestBranchIdx(branches, 8.5, 8.5) == 2); // branch C closest

    // best-fit table
    branches[0].tables = {{101, 2}, {102, 4}, {103, 8}};
    Table* fit = findBestFitTable(branches[0], 3);
    assert(fit && fit->capacity == 4);

    // merge tables (linked list) & cycle prevention & unmerge
    assert(mergeTables(branches[0], 101, 102));
    assert(totalMergedCapacity(branches[0], 101) == 6);
    assert(!mergeTables(branches[0], 101, 102)); // already merged
    assert(!mergeTables(branches[0], 102, 101)); // cycle backwards blocked
    assert(unmergeTables(branches[0], 102));     // unmerge using any table in group
    assert(totalMergedCapacity(branches[0], 101) == 2);
    assert(totalMergedCapacity(branches[0], 102) == 4);
    assert(!unmergeTables(branches[0], 102));    // already unmerged
    assert(mergeTables(branches[0], 101, 102));  // re-merge for table group

    // seat + checkout (with bill amount) + waitlist FIFO pull + undo (with sales rollback)
    Branch& b = branches[0];
    Table* t103 = findTable(b, 103);
    b.waitlist.push({"0833333333", "Waiting1", 0, 6});
    seatCustomer(b, t103, {"0844444444", "Guest1", 0, 5});
    assert(!t103->isAvailable && b.customersToday == 1);
    checkoutTable(b, t103, 1200.0); // frees then re-seats the queued party (fits: 6<=8) with sales
    assert(fabs(b.salesToday - 1200.0) < 1e-9);
    assert(!t103->isAvailable && t103->customerName == "Waiting1");
    assert(b.waitlist.empty());
    undoLast(branches); // undoes the auto-reseat (SEAT)
    assert(t103->isAvailable && fabs(b.salesToday - 1200.0) < 1e-9);
    undoLast(branches); // undoes the checkout (restores salesToday to 0)
    assert(fabs(b.salesToday - 0.0) < 1e-9);
    undoLast(branches); // undoes the original seat
    assert(t103->customerName.empty());

    // Test seating & checkout across merged table chain
    assert(findTable(b, 101)->isAvailable && findTable(b, 102)->isAvailable);
    Table* fitMerged = findBestFitTable(b, 5); // 5 fits merged group 101 (cap 6)
    assert(fitMerged && fitMerged->id == 101);
    seatCustomer(b, fitMerged, {"0855555555", "BigGroup", 0, 5});
    assert(!findTable(b, 101)->isAvailable && !findTable(b, 102)->isAvailable);
    assert(findTable(b, 102)->customerName == "BigGroup");
    checkoutTable(b, fitMerged, 1500.0);
    assert(findTable(b, 101)->isAvailable && findTable(b, 102)->isAvailable);
    assert(unmergeTables(b, 101));

    // validation & capacity helpers & queue cancel
    assert(validPhone("0812345678"));
    assert(!validPhone("0812abc"));
    assert(!validPhone(""));
    assert(validField("Somchai"));
    assert(!validField("Som|chai"));
    assert(!validField("   "));
    assert(branchCapacity(branches[0]) == 14); // 2 + 4 + 8
    assert(!inAnyWaitlist(branches, "0812345678"));
    b.waitlist.push({"0899990001", "CancelMe", 0, 2});
    assert(inAnyWaitlist(branches, "0899990001"));
    b.waitlist.pop(); // cancel / drop
    assert(!inAnyWaitlist(branches, "0899990001"));

    // member lookup / insert verification
    root = bstInsert(root, {"0899990002", "NewMember", 0, 4});
    assert(bstFind(root, "0899990002") != nullptr);
    assert(bstFind(root, "0899990002")->name == "NewMember");

    // pull + clear waitlist (strict FIFO: the oversized front blocks the rest)
    {
        Branch& w = branches[0];
        w.waitlist = {};
        for (auto& t : w.tables) { t.isAvailable = true; t.customerName.clear(); }
        w.waitlist.push({"1", "Big", 0, 20});   // fits no table
        w.waitlist.push({"2", "Small", 0, 2});
        assert(pullWaitlist(w) == 0 && w.waitlist.size() == 2);
        w.waitlist.pop();                          // front removed -> Small now fronts
        assert(pullWaitlist(w) == 1 && w.waitlist.empty());
        assert(findTable(w, 101)->customerName == "Small");
        w.waitlist.push({"3", "X", 0, 2});
        w.waitlist.push({"4", "Y", 0, 2});
        assert(clearWaitlist(w) == 2 && w.waitlist.empty() && clearWaitlist(w) == 0);
        for (auto& t : w.tables) { t.isAvailable = true; t.customerName.clear(); }
        w.customersToday = 0;
    }

    // merge sort ranking
    branches[0].customersToday = 5;
    branches[1].customersToday = 20;
    branches[2].customersToday = 10;
    vector<Branch*> ranked = {&branches[0], &branches[1], &branches[2]};
    mergeSortBranches(ranked, 0, (int)ranked.size() - 1);
    assert(ranked[0]->id == 2 && ranked[1]->id == 3 && ranked[2]->id == 1);

    // promotion: 10% off when pax >= 4, plus flat 50 baht + syntax error handling
    PromotionRule promo{"pax >= 4", 10, 50};
    double price = applyPromotion(promo, 4, 1000);
    assert(fabs(price - 850) < 1e-9); // 1000*0.9 - 50
    assert(applyPromotion(promo, 2, 1000) == 1000); // condition false -> untouched
    bool exprOk = true;
    applyPromotion({"pax +", 10, 0}, 4, 1000, &exprOk);
    assert(!exprOk); // syntax error caught safely
    exprOk = true;
    applyPromotion({"pax / 0", 10, 0}, 4, 1000, &exprOk);
    assert(!exprOk); // division by zero caught safely

    // recursion stats
    branches[0].tables = {{101, 2}, {102, 4}};
    branches[1].tables = {{201, 4}};
    branches[2].tables = {};
    Stats s = aggregateStats(branches);
    assert(s.totalTables == 3);
    assert(s.totalCustomers == branches[0].customersToday + branches[1].customersToday + branches[2].customersToday);
    assert(s.freeTables == 3); // every table is free at this point

    // free-table report counts + table orders (incl. merged group, checkout clear, undo restore)
    {
        Branch& o = branches[0]; // tables 101 (2 seats), 102 (4 seats)
        assert(freeTableCount(o) == 2 && freeSeatCount(o) == 6);
        assert(!addOrderItem(o, 101, {"Beef", 2, 189.0}));    // table is free -> cannot order
        seatCustomer(o, findTable(o, 101), {"0866666666", "Orderer", 0, 2});
        assert(freeTableCount(o) == 1 && freeSeatCount(o) == 4);
        assert(addOrderItem(o, 101, {"Beef", 2, 189.0}));
        assert(addOrderItem(o, 101, {"Beef", 1, 189.0}));     // same item -> qty merges
        assert(addOrderItem(o, 101, {"Pork", 1, 129.5}));
        assert(!addOrderItem(o, 101, {"Bad:Name", 1, 10}));   // separator char rejected
        assert(!addOrderItem(o, 101, {"Egg", 0, 10}));        // qty must be > 0
        assert(!addOrderItem(o, 999, {"Egg", 1, 10}));        // unknown table
        assert(findTable(o, 101)->orders.size() == 2 && findTable(o, 101)->orders[0].qty == 3);
        assert(fabs(orderTotal(findTable(o, 101)->orders) - (3 * 189.0 + 129.5)) < 1e-9);
        // orders survive save/load
        const string orderFile = "test_teenoi_orders.tmp";
        assert(saveData(orderFile, branches, root));
        vector<Branch> ob;
        CustNode* oroot = nullptr;
        assert(loadData(orderFile, ob, oroot));
        remove(orderFile.c_str());
        bstClear(oroot);
        assert(ob[0].tables[0].orders.size() == 2);
        assert(ob[0].tables[0].orders[0].name == "Beef" && ob[0].tables[0].orders[0].qty == 3);
        assert(fabs(ob[0].tables[0].orders[1].price - 129.5) < 1e-9);
        assert(ob[0].tables[1].orders.empty());
        // checkout clears the orders; undo brings them back
        checkoutTable(o, findTable(o, 101), orderTotal(findTable(o, 101)->orders));
        assert(findTable(o, 101)->orders.empty() && freeTableCount(o) == 2);
        assert(undoLast(branches));
        assert(findTable(o, 101)->orders.size() == 2 && !findTable(o, 101)->isAvailable);
        assert(undoLast(branches)); // undo the seat -> table free again, no orders
        assert(findTable(o, 101)->orders.empty() && freeTableCount(o) == 2);
        // merged group: ordering via the follower table lands on the head table
        assert(mergeTables(o, 101, 102));
        seatCustomer(o, findTable(o, 101), {"0877777777", "Group", 0, 5});
        assert(freeTableCount(o) == 0);
        assert(addOrderItem(o, 102, {"Shrimp", 2, 159.0}));
        assert(findTable(o, 101)->orders.size() == 1 && findTable(o, 102)->orders.empty());
        checkoutTable(o, findTable(o, 102), 318.0);
        assert(findTable(o, 101)->orders.empty() && freeTableCount(o) == 2);
        assert(unmergeTables(o, 101));
    }

    // HTML table map: free/busy classes, items on the table, names are escaped
    {
        Branch& o = branches[0];
        const string htmlFile = "test_teenoi_map.tmp";
        assert(exportHtmlMap(htmlFile, branches));
        auto slurp = [](const string& fn) {
            ifstream f(fn);
            return string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
        };
        string page = slurp(htmlFile);
        assert(page.find("class=\"tbl free") != string::npos && page.find("class=\"tbl busy") == string::npos);
        seatCustomer(o, findTable(o, 101), {"0899000001", "<b>X&Y</b>", 0, 2});
        assert(addOrderItem(o, 101, {"Beef", 1, 189.0}));
        assert(exportHtmlMap(htmlFile, branches));
        page = slurp(htmlFile);
        remove(htmlFile.c_str());
        assert(page.find("class=\"tbl busy") != string::npos);
        assert(page.find("T101") != string::npos && page.find("189.00") != string::npos);
        assert(page.find("&lt;b&gt;X&amp;Y&lt;/b&gt;") != string::npos); // escaped
        assert(page.find("<b>X&Y") == string::npos);                      // never raw
        checkoutTable(o, findTable(o, 101), 0);
        assert(freeTableCount(o) == 2);
    }

    // persistence test
    const string testFile = "test_teenoi_save.tmp";
    assert(saveData(testFile, branches, root));
    vector<Branch> loadedBranches;
    CustNode* loadedRoot = nullptr;
    assert(loadData(testFile, loadedBranches, loadedRoot));
    assert(loadedBranches.size() == branches.size());
    assert(loadedBranches[0].tables.size() == branches[0].tables.size());
    assert(bstFind(loadedRoot, "0899999999") != nullptr);
    assert(bstFind(loadedRoot, "0899999999")->name == "Somchai");
    remove(testFile.c_str());
    bstClear(loadedRoot);
    bstClear(root);

    cout << "All self-checks passed (including persistence test).\n";
}

// Terminal UI Helpers (Clean, uncluttered, easy to use)
static void clearScreen() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (GetConsoleMode(hOut, &mode)) {
        system("cls");
    } else {
        // Piped or redirected output
        cout << "\n--- [ SCREEN REFRESH ] ---\n\n";
    }
#else
    if (isatty(fileno(stdout))) {
        cout << "\033[2J\033[H" << flush;
    } else {
        cout << "\n--- [ SCREEN REFRESH ] ---\n\n";
    }
#endif
}

static void pausePrompt() {
    if (cin.eof()) return;
    cout << "\n  [i] กด Enter เพื่อดำเนินการต่อ (Press Enter to continue)... ";
    string dummy;
    getline(cin, dummy);
}

static string readString(const string& prompt) {
    if (cin.eof()) return "";
    cout << prompt;
    string val;
    if (!getline(cin, val)) return "";
    if (val.size() >= 3 && (unsigned char)val[0] == 0xEF && (unsigned char)val[1] == 0xBB && (unsigned char)val[2] == 0xBF) {
        val = val.substr(3);
    }
    if (!val.empty() && val.back() == '\r') val.pop_back();
    return val;
}

static int readInt(const string& prompt, int defaultVal = -1) {
    string s = readString(prompt);
    if (s.empty()) return defaultVal;
    try {
        return stoi(s);
    } catch (...) {
        return defaultVal;
    }
}

static double readDouble(const string& prompt, double defaultVal = 0.0) {
    string s = readString(prompt);
    if (s.empty()) return defaultVal;
    try {
        return stod(s);
    } catch (...) {
        return defaultVal;
    }
}

static void printHeader(const string& title) {
    clearScreen();
    cout << "+================================================================+\n";
    cout << "| " << left << setw(62) << title << " |\n";
    cout << "+================================================================+\n\n";
}

static vector<Branch> seedBranches() {
    vector<Branch> bs;
    bs.push_back({1, "Teenoi Suki - Central", 0, 0});
    bs.push_back({2, "Teenoi Suki - Siam", 5, 5});
    bs.push_back({3, "Teenoi Suki - Rangsit", -3, 8});
    for (auto& b : bs) {
        b.tables = {{b.id * 100 + 1, 2}, {b.id * 100 + 2, 4}, {b.id * 100 + 3, 6}, {b.id * 100 + 4, 8}};
    }
    return bs;
}

static void displayTablesTable(const Branch& b) {
    cout << "+-----------------------------------------------------------------------------------+\n";
    cout << "| สาขา: " << left << setw(75) << b.name << " |\n";
    cout << "+------+-----------+-----------------+-----------------------+----------------------+\n";
    cout << "| โต๊ะ | ขนาด(คน)  | สถานะ           | ผู้ใช้งาน / ลูกค้า    | โต๊ะที่เชื่อมต่อ      |\n";
    cout << "+------+-----------+-----------------+-----------------------+----------------------+\n";
    for (const auto& t : b.tables) {
        string statusStr = t.isAvailable ? "[ ว่าง ]" : "[ ไม่ว่าง ]";
        string occupant = t.isAvailable ? "-" : t.customerName;
        if (occupant.length() > 21) occupant = occupant.substr(0, 18) + "...";
        string mergeStr = "-";
        if (t.mergedNextId != -1) {
            mergeStr = "เชื่อม -> โต๊ะ " + to_string(t.mergedNextId);
        } else {
            for (const auto& other : b.tables) {
                if (other.mergedNextId == t.id) {
                    mergeStr = "<- พ่วงกับโต๊ะ " + to_string(other.id);
                    break;
                }
            }
        }
        
        cout << "| " << right << setw(4) << t.id << " | "
             << right << setw(6) << t.capacity << " คน | "
             << left << setw(17) << statusStr << " | "
             << left << setw(21) << occupant << " | "
             << left << setw(20) << mergeStr << " |\n";
    }
    cout << "+------+-----------+-----------------+-----------------------+----------------------+\n";
    cout << "  * โต๊ะว่าง: " << freeTableCount(b) << " / " << b.tables.size() << " โต๊ะ"
         << " (ที่นั่งว่างรวม " << freeSeatCount(b) << " ที่นั่ง)\n";
    cout << "  * คิวรอขณะนี้: " << b.waitlist.size() << " คิว\n\n";
}

// Report of free tables for every branch: how many are free, which ones, and how many seats.
static void displayFreeTableReport(const vector<Branch>& branches) {
    int allFree = 0, allTables = 0, allSeats = 0;
    cout << "+----------------------------------------------------------------+\n";
    cout << "| รายงานโต๊ะว่างทุกสาขา (FREE TABLE REPORT)                       |\n";
    cout << "+----------------------------------------------------------------+\n";
    for (const auto& b : branches) {
        int total = (int)b.tables.size();
        int freeN = freeTableCount(b);
        int seats = freeSeatCount(b);
        allFree += freeN; allTables += total; allSeats += seats;
        cout << "\n" << b.name << "\n";
        cout << "  โต๊ะว่าง " << freeN << " / " << total << " โต๊ะ"
             << "  (ที่นั่งว่างรวม " << seats << " ที่นั่ง, คิวรอ " << b.waitlist.size() << " คิว)\n";
        cout << "  โต๊ะที่ว่าง: ";
        if (freeN == 0) {
            cout << "- (เต็มทุกโต๊ะ)";
        } else {
            bool first = true;
            for (const auto& t : b.tables) {
                if (!t.isAvailable) continue;
                if (!first) cout << ", ";
                cout << t.id << " (" << t.capacity << " ที่)";
                first = false;
            }
        }
        cout << "\n";
    }
    cout << "\n----------------------------------------------------------------\n";
    cout << "รวมทุกสาขา: โต๊ะว่าง " << allFree << " / " << allTables << " โต๊ะ (ที่นั่งว่างรวม " << allSeats << " ที่นั่ง)\n";
}

// Items on each occupied table (merged groups shown once, under their head table).
// onlyHeadId >= 0 limits the output to that one table group.
static void displayTableOrders(Branch& b, int onlyHeadId = -1) {
    cout << "รายการอาหารบนโต๊ะ - " << b.name << "\n";
    cout << "----------------------------------------------------------------\n";
    int shown = 0;
    double grand = 0;
    cout << fixed << setprecision(2);
    for (const auto& t : b.tables) {
        if (t.isAvailable) continue;
        if (chainHeadId(b, t.id) != t.id) continue; // follower tables are shown under their head
        if (onlyHeadId >= 0 && t.id != onlyHeadId) continue;
        shown++;
        cout << "โต๊ะ " << t.id;
        if (t.mergedNextId != -1) {
            cout << " (เชื่อมกับโต๊ะ";
            unordered_set<int> seen;
            seen.insert(t.id);
            int nid = t.mergedNextId;
            while (nid != -1 && seen.insert(nid).second) {
                cout << " " << nid;
                Table* nt = findTable(b, nid);
                nid = nt ? nt->mergedNextId : -1;
            }
            cout << ")";
        }
        cout << " - ลูกค้า: " << t.customerName << "\n";
        if (t.orders.empty()) {
            cout << "    (ยังไม่มีรายการอาหาร)\n";
        } else {
            for (const auto& o : t.orders)
                cout << "    - " << o.name << " x" << o.qty << " @" << o.price << " = " << o.qty * o.price << " บาท\n";
            double sub = orderTotal(t.orders);
            grand += sub;
            cout << "    รวมโต๊ะนี้: " << sub << " บาท\n";
        }
        cout << "\n";
    }
    if (shown == 0) cout << "  ไม่มีโต๊ะที่มีลูกค้านั่งอยู่\n\n";
    else if (onlyHeadId < 0) cout << "ยอดรวมรายการอาหารทั้งสาขา: " << grand << " บาท\n\n";
}

// Picture of the floor: every table drawn as a box (FREE / BUSY), seats as o (free) or @ (taken).
// Box contents are ASCII-only on purpose so the boxes stay aligned in any console font.
static void displayTableMap(Branch& b) {
    const int W = 16;       // inner width of one table box
    const size_t perRow = 4;
    auto pad = [&](string s) {
        if ((int)s.size() > W) s = s.substr(0, W);
        int left = (W - (int)s.size()) / 2;
        return string(left, ' ') + s + string(W - (int)s.size() - left, ' ');
    };

    cout << "ผังโต๊ะ - " << b.name << "\n";
    cout << "  o = ที่นั่งว่าง   @ = มีลูกค้านั่ง   ==> / <== = โต๊ะที่เชื่อมกัน\n\n";

    for (size_t start = 0; start < b.tables.size(); start += perRow) {
        size_t end = min(b.tables.size(), start + perRow);
        string top, l1, l2, l3, l4, l5;
        for (size_t i = start; i < end; i++) {
            const Table& t = b.tables[i];
            top += "+" + string(W, '-') + "+  ";
            l1 += "|" + pad("T" + to_string(t.id) + " (" + to_string(t.capacity) + ")") + "|  ";
            l2 += "|" + pad(t.isAvailable ? "[ FREE ]" : "[ BUSY ]") + "|  ";

            string seats;
            for (int s = 0; s < t.capacity; s++) {
                if (s) seats += ' ';
                seats += t.isAvailable ? 'o' : '@';
            }
            l3 += "|" + pad(seats) + "|  ";

            string link;
            if (t.mergedNextId != -1) {
                link = "==> T" + to_string(t.mergedNextId);
            } else {
                for (const auto& other : b.tables)
                    if (other.mergedNextId == t.id) { link = "<== T" + to_string(other.id); break; }
            }
            l4 += "|" + pad(link) + "|  ";

            string info;
            if (!t.isAvailable) {
                if (chainHeadId(b, t.id) == t.id) info = "items: " + to_string(t.orders.size());
                else info = "(joined)";
            }
            l5 += "|" + pad(info) + "|  ";
        }
        cout << "  " << top << "\n  " << l1 << "\n  " << l2 << "\n  " << l3
             << "\n  " << l4 << "\n  " << l5 << "\n  " << top << "\n\n";
    }
    cout << "  โต๊ะว่าง " << freeTableCount(b) << " / " << b.tables.size() << " โต๊ะ"
         << " (ที่นั่งว่างรวม " << freeSeatCount(b) << " ที่นั่ง)\n\n";
    displayTableOrders(b); // what is on each busy table
}

// Menu board with a small hot-pot picture. Thai names have no fixed width in a console,
// so only the top/bottom borders are drawn and prices follow a dotted leader.
static void displayMenuBoard() {
    cout << "\n";
    cout << "              )  )  )\n";
    cout << "            .---------.\n";
    cout << "           /   SUKI    \\\n";
    cout << "          (_____________)\n";
    cout << "           \\_____o_____/\n\n";
    cout << "  +--------------------------------------------+\n";
    cout << "     TEENOI SUKI  -  เมนูอาหาร (MENU)\n";
    cout << "  +--------------------------------------------+\n";
    cout << fixed << setprecision(2);
    for (size_t i = 0; i < MENU_ITEMS.size(); i++) {
        cout << "   " << right << setw(2) << (i + 1) << ". " << MENU_ITEMS[i].name << " ........ "
             << MENU_ITEMS[i].price << " บาท\n";
    }
    cout << "  +--------------------------------------------+\n\n";
}

// Let staff add items to an occupied table. Returns how many items were added.
static int addOrdersFlow(Branch& b) {
    displayTablesTable(b);
    int id = readInt("ระบุรหัสโต๊ะที่ลูกค้านั่งอยู่ (0 เพื่อยกเลิก): ");
    if (id <= 0) return 0;
    Table* t = findTable(b, id);
    if (!t) { cout << "\n>> ไม่พบโต๊ะรหัส " << id << "\n"; return 0; }
    if (t->isAvailable) { cout << "\n>> โต๊ะ " << id << " ว่างอยู่ ไม่มีลูกค้าให้สั่งอาหาร\n"; return 0; }

    int added = 0;
    int n = (int)MENU_ITEMS.size();
    while (true) {
        cout << "\nเมนูอาหาร:\n";
        cout << fixed << setprecision(2);
        for (int i = 0; i < n; i++)
            cout << "  " << (i + 1) << ") " << MENU_ITEMS[i].name << " - " << MENU_ITEMS[i].price << " บาท\n";
        cout << "  " << (n + 1) << ") อื่น ๆ (ระบุชื่อและราคาเอง)\n";
        cout << "  0) เสร็จสิ้น\n";
        int c = readInt("เลือกรายการ: ");
        if (c == 0 || (cin.eof() && c < 0)) break;

        OrderItem item;
        if (c >= 1 && c <= n) {
            item = MENU_ITEMS[c - 1];
        } else if (c == n + 1) {
            item.name = readString("ชื่อรายการ: ");
            item.price = readDouble("ราคาต่อหน่วย (บาท): ", -1);
        } else {
            cout << ">> ตัวเลือกไม่ถูกต้อง\n";
            continue;
        }
        item.qty = readInt("จำนวน: ");
        if (addOrderItem(b, id, item)) {
            added++;
            cout << ">> [สำเร็จ] เพิ่ม " << item.name << " x" << item.qty << " ให้โต๊ะ " << id << " แล้ว\n";
        } else {
            cout << ">> ข้อมูลไม่ถูกต้อง (ชื่อห้ามว่างและห้ามมี | : ;  จำนวนต้อง > 0  ราคาต้อง >= 0)\n";
        }
    }
    if (added > 0) {
        cout << "\n";
        displayTableOrders(b, chainHeadId(b, id));
    }
    return added;
}

static void displayWaitlist(const Branch& b) {
    cout << "คิวรอที่สาขา " << b.name << " (" << b.waitlist.size() << " คิว):\n";
    queue<Customer> q = b.waitlist;
    for (int i = 1; !q.empty(); i++, q.pop())
        cout << "  " << i << ") " << q.front().name << " (" << q.front().phone << ") - " << q.front().usualPax << " ท่าน\n";
    cout << "\n";
}

static Branch* pickBranch(vector<Branch>& branches) {
    cout << "รายชื่อสาขา:\n";
    for (auto& b : branches) {
        cout << "  [" << b.id << "] " << b.name 
             << " (พิกัด: " << b.x << ", " << b.y << ")"
             << " - โต๊ะทั้งหมด " << b.tables.size() << " โต๊ะ\n";
    }
    cout << "\n";
    int id = readInt("เลือกหมายเลขสาขา (Branch ID, 0 เพื่อยกเลิก): ");
    if (id == 0) return nullptr;
    for (auto& b : branches) if (b.id == id) return &b;
    cout << ">> ไม่พบสาขาที่ระบุ\n";
    return nullptr;
}

// ---------------- UI Menus ----------------
static void customerMenu(vector<Branch>& branches, CustNode*& custRoot) {
    while (true) {
        printHeader("เมนูลูกค้า (CUSTOMER MENU)");
        cout << "  1) ค้นหาสาขาที่ใกล้ที่สุด (Find Nearest Branch)\n";
        cout << "  2) จองโต๊ะ / เข้าคิวรอ (Book Table / Join Waitlist)\n";
        cout << "  3) ตรวจสอบประวัติสมาชิก (Member Lookup by Phone)\n";
        cout << "  0) ย้อนกลับ (Back)\n\n";

        int choice = readInt("เลือกเมนู: ");
        if (choice == 0) break;

        if (choice == 1) {
            printHeader("ค้นหาสาขาที่ใกล้ที่สุด (MIN-HEAP)");
            double x = readDouble("กรอกพิกัด X ของคุณ: ");
            double y = readDouble("กรอกพิกัด Y ของคุณ: ");
            int idx = nearestBranchIdx(branches, x, y);
            if (idx >= 0) {
                double d = sqrt(distSq(x, y, branches[idx].x, branches[idx].y));
                cout << "\n>> สาขาที่ใกล้คุณที่สุด: " << branches[idx].name << "\n";
                cout << ">> ระยะทางประมาณ: " << fixed << setprecision(2) << d << " หน่วย\n";
            } else {
                cout << "\n>> ไม่พบข้อมูลสาขาในระบบ\n";
            }
            pausePrompt();
        } else if (choice == 2) {
            printHeader("จองโต๊ะ / เข้าคิวรอ (BEST-FIT & WAITLIST)");
            Branch* b = pickBranch(branches);
            if (b) {
                string phone = readString("เบอร์โทรศัพท์ (Phone): ");
                string name = readString("ชื่อลูกค้า (Name): ");
                int pax = readInt("จำนวนผู้ใช้บริการ (Pax): ");

                if (pax <= 0 || !validPhone(phone) || !validField(name)) {
                    cout << "\n>> ข้อมูลไม่ถูกต้อง (เบอร์ต้องเป็นตัวเลขล้วน, ชื่อห้ามว่างและห้ามมี '|', จำนวนคนต้องมากกว่า 0)\n";
                } else if (pax > branchCapacity(*b)) {
                    cout << "\n>> สาขานี้รับได้สูงสุด " << branchCapacity(*b) << " ท่าน (รวมทุกโต๊ะ) ไม่สามารถรับกลุ่ม "
                         << pax << " ท่านได้\n";
                } else if (inAnyWaitlist(branches, phone)) {
                    Branch* queuedBranch = nullptr;
                    int queuePos = 0;
                    int queuePax = 0;
                    for (auto& br : branches) {
                        queue<Customer> q = br.waitlist;
                        int pos = 1;
                        while (!q.empty()) {
                            if (q.front().phone == phone) {
                                queuedBranch = &br;
                                queuePos = pos;
                                queuePax = q.front().usualPax;
                                break;
                            }
                            pos++;
                            q.pop();
                        }
                        if (queuedBranch) break;
                    }
                    cout << "\n+------------------------------------------------+\n";
                    cout << "| สถานะคิวรอปัจจุบัน                            |\n";
                    cout << "+------------------------------------------------+\n";
                    if (queuedBranch) {
                        cout << "  สาขาที่รอ: " << queuedBranch->name << "\n";
                        cout << "  ลำดับคิวของคุณ: คิวที่ " << queuePos << " จากทั้งหมด " << queuedBranch->waitlist.size() << " คิว\n";
                        cout << "  จำนวนที่นั่ง: " << queuePax << " ท่าน\n";
                        cout << "+------------------------------------------------+\n";
                        string cancelAns = readString("  ต้องการยกเลิกคิวนี้หรือไม่? (y/N): ");
                        if (cancelAns == "y" || cancelAns == "Y") {
                            queue<Customer> newQ;
                            while (!queuedBranch->waitlist.empty()) {
                                if (queuedBranch->waitlist.front().phone != phone) {
                                    newQ.push(queuedBranch->waitlist.front());
                                }
                                queuedBranch->waitlist.pop();
                            }
                            queuedBranch->waitlist = newQ;
                            dataDirty = true;
                            saveData(DATA_FILENAME, branches, custRoot);
                            cout << "\n>> [สำเร็จ] ยกเลิกคิวเรียบร้อยแล้ว\n";
                            int pulled = pullWaitlist(*queuedBranch);
                            if (pulled > 0) cout << ">> [ระบบดึงคิว] จัดคิวถัดไปเข้าโต๊ะว่างแล้ว " << pulled << " คิว\n";
                        }
                    } else {
                        cout << "  พบเบอร์นี้อยู่ในคิวรอของระบบแล้ว\n";
                        cout << "+------------------------------------------------+\n";
                    }
                } else {
                    Customer* existing = bstFind(custRoot, phone);
                    Customer c = existing ? *existing : Customer{phone, name, 0, pax};
                    c.name = name; // update name if changed
                    c.visitCount++;
                    c.usualPax = pax;

                    // FIFO fairness: if anyone is already waiting, a newcomer must not jump the queue.
                    Table* t = b->waitlist.empty() ? findBestFitTable(*b, pax) : nullptr;
                    if (t) {
                        seatCustomer(*b, t, c);
                        cout << "\n>> [สำเร็จ] จัดโต๊ะ " << t->id << " (ขนาด " << totalMergedCapacity(*b, t->id)
                             << " ท่าน) ให้คุณ " << c.name << " เรียบร้อยแล้ว!\n";
                    } else {
                        b->waitlist.push(c);
                        cout << "\n>> ยังไม่มีโต๊ะที่เหมาะสม/มีคิวก่อนหน้า เพิ่มคุณ " << c.name << " เข้าคิวรอที่สาขา " << b->name << "\n";
                        cout << ">> ลำดับคิวของคุณ: คิวที่ " << b->waitlist.size() << "\n";
                        if (pullWaitlist(*b) > 0)
                            cout << ">> [ระบบดึงคิว] มีโต๊ะว่างรองรับคิวแล้ว ดูผังโต๊ะที่เมนูพนักงาน\n";
                        if (findBestFitTable(*b, 1) && !b->waitlist.empty() && b->waitlist.front().usualPax > 8)
                            cout << ">> [หมายเหตุ] กลุ่มใหญ่ต้องให้พนักงานเชื่อมโต๊ะ (เมนู 2) แล้วกดดึงคิว (เมนู 5)\n";
                    }
                    custRoot = bstInsert(custRoot, c);
                    dataDirty = true;
                    saveData(DATA_FILENAME, branches, custRoot);
                }
            }
            pausePrompt();
        } else if (choice == 3) {
            printHeader("ค้นหาประวัติสมาชิก (BST LOOKUP)");
            string phone = readString("กรอกเบอร์โทรศัพท์: ");
            Customer* c = bstFind(custRoot, phone);
            if (c) {
                cout << "\n+------------------------------------------------+\n";
                cout << "| ข้อมูลสมาชิก: " << left << setw(32) << c->name << " |\n";
                cout << "+------------------------------------------------+\n";
                cout << "  เบอร์โทร: " << c->phone << "\n";
                cout << "  จำนวนครั้งที่มาทาน: " << c->visitCount << " ครั้ง\n";
                cout << "  จำนวนท่านปกติ: " << c->usualPax << " ท่าน\n";
                cout << "+------------------------------------------------+\n";
            } else {
                cout << "\n>> ไม่พบข้อมูลสมาชิกเบอร์นี้ในระบบ\n";
                string regAns = readString("ต้องการสมัครสมาชิกใหม่หรือไม่? (y/N): ");
                if (regAns == "y" || regAns == "Y") {
                    string newName = readString("ชื่อ-นามสกุลสมาชิก: ");
                    int usualPax = readInt("จำนวนผู้ใช้บริการปกติ (Pax, ค่าเริ่มต้น 2): ");
                    if (usualPax <= 0) usualPax = 2;
                    if (!validField(newName)) {
                        cout << ">> ข้อมูลชื่อไม่ถูกต้อง (ห้ามว่างและห้ามมี '|') ยกเลิกการสมัคร\n";
                    } else {
                        Customer newCust{phone, newName, 0, usualPax};
                        custRoot = bstInsert(custRoot, newCust);
                        dataDirty = true;
                        saveData(DATA_FILENAME, branches, custRoot);
                        cout << ">> [สำเร็จ] สมัครสมาชิกใหม่ให้คุณ " << newName << " เรียบร้อยแล้ว!\n";
                    }
                }
            }
            pausePrompt();
        }
    }
}

static void staffMenu(vector<Branch>& branches, CustNode*& custRoot) {
    while (true) {
        printHeader("เมนูพนักงาน (STAFF OPERATIONS)");
        cout << "  1) ดูผังโต๊ะและสถานะ (View Tables & Status)\n";
        cout << "  2) เชื่อมโต๊ะ (Merge Tables - Linked List)\n";
        cout << "  3) เช็คบิล / คืนโต๊ะ (Checkout Table & Auto-seat Waitlist)\n";
        cout << "  4) เลิกทำรายการล่าสุด (Undo Last Action - Stack)\n";
        cout << "  5) ดึงคิวที่ค้างอยู่เข้าโต๊ะว่าง (Pull Waitlist into Free Tables)\n";
        cout << "  6) จัดการคิวรอ / ลบคิว (Manage / Drop Waitlist)\n";
        cout << "  7) แยกโต๊ะที่เชื่อมไว้ (Unmerge Tables)\n";
        cout << "  8) จัดโต๊ะลูกค้า Walk-in (Seat Walk-in Customer)\n";
        cout << "  9) รายงานโต๊ะว่างทุกสาขา (Free Table Report)\n";
        cout << " 10) สั่งอาหารเพิ่มที่โต๊ะ (Add Menu Items to Table)\n";
        cout << " 11) ดูรายการอาหารบนโต๊ะ (View Table Orders)\n";
        cout << " 12) ผังโต๊ะแบบภาพ ว่าง/ไม่ว่าง (Table Map)\n";
        cout << " 13) ดูกระดานเมนูอาหาร (Menu Board)\n";
        cout << " 14) เปิดผังโต๊ะบนหน้าเว็บ สีเขียว/แดง (Open Table Map in Browser)\n";
        cout << "  0) ย้อนกลับ (Back)\n\n";

        int choice = readInt("เลือกเมนู: ");
        if (choice == 0) break;

        if (choice == 1) {
            printHeader("ดูผังโต๊ะของสาขา");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTablesTable(*b);
            }
            pausePrompt();
        } else if (choice == 2) {
            printHeader("เชื่อมโต๊ะสำหรับกลุ่มใหญ่ (TABLE MERGE)");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTablesTable(*b);
                int id1 = readInt("ระบุรหัสโต๊ะที่ 1: ");
                int id2 = readInt("ระบุรหัสโต๊ะที่ 2: ");
                if (mergeTables(*b, id1, id2)) {
                    cout << "\n>> [สำเร็จ] เชื่อมต่อโต๊ะ " << id1 << " กับ " << id2 << " เรียบร้อย!\n";
                    cout << ">> ความจุรวมของชุดโต๊ะนี้: " << totalMergedCapacity(*b, id1) << " ที่นั่ง\n";
                    saveData(DATA_FILENAME, branches, custRoot);
                } else {
                    cout << "\n>> ไม่สามารถเชื่อมโต๊ะได้ (ไม่พบโต๊ะ / อยู่ชุดเดียวกันแล้ว / มีโต๊ะที่ลูกค้านั่งอยู่)\n";
                }
            }
            pausePrompt();
        } else if (choice == 3) {
            printHeader("เช็คบิล / ปล่อยโต๊ะ (CHECKOUT)");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTablesTable(*b);
                int id = readInt("ระบุรหัสโต๊ะที่ต้องการเช็คบิล: ");
                Table* t = findTable(*b, id);
                if (t) {
                    if (t->isAvailable) {
                        cout << "\n>> โต๊ะ " << id << " ว่างอยู่แล้ว ไม่จำเป็นต้องเช็คบิล\n";
                    } else {
                        string prevGuest = t->customerName;
                        Table* head = findTable(*b, chainHeadId(*b, id));
                        double orderSum = head ? orderTotal(head->orders) : 0.0;
                        double amount = 0.0;
                        if (head && !head->orders.empty()) {
                            displayTableOrders(*b, head->id);
                            string amtStr = readString("ยอดบิลที่ชำระ (บาท, กด Enter = ใช้ยอดตามรายการอาหาร, ใส่ 0 ถ้าไม่บันทึกยอด): ");
                            if (amtStr.empty()) {
                                amount = orderSum;
                            } else {
                                try { amount = stod(amtStr); } catch (...) { amount = orderSum; }
                            }
                        } else {
                            amount = readDouble("ยอดบิลที่ชำระ (บาท, ใส่ 0 ถ้าไม่บันทึกยอด): ");
                        }
                        if (amount < 0) amount = 0;
                        int seated = checkoutTable(*b, t, amount);
                        cout << "\n>> [สำเร็จ] เช็คบิลโต๊ะ " << id << " เรียบร้อย (ลูกค้าเดิม: " << prevGuest
                             << ", ยอด " << fixed << setprecision(2) << amount << " บาท)\n";
                        if (seated > 0) {
                            cout << ">> [ระบบดึงคิวอัตโนมัติ] จัดคิวรอเข้าโต๊ะว่างแล้ว " << seated << " คิว\n";
                        }
                        saveData(DATA_FILENAME, branches, custRoot);
                    }
                } else {
                    cout << "\n>> ไม่พบโต๊ะรหัส " << id << "\n";
                }
            }
            pausePrompt();
        } else if (choice == 4) {
            printHeader("เลิกทำรายการล่าสุด (UNDO LAST ACTION)");
            if (undoLast(branches)) {
                cout << "\n>> [สำเร็จ] ย้อนกลับการกระทำล่าสุดเรียบร้อยแล้ว\n";
                saveData(DATA_FILENAME, branches, custRoot);
            } else {
                cout << "\n>> ไม่มีประวัติรายการให้ย้อนกลับ (History is empty)\n";
            }
            pausePrompt();
        } else if (choice == 5) {
            printHeader("ดึงคิวที่ค้างอยู่ (PULL WAITLIST)");
            Branch* b = pickBranch(branches);
            if (b) {
                if (b->waitlist.empty()) {
                    cout << "\n>> ไม่มีคิวรอที่สาขานี้\n";
                } else {
                    displayWaitlist(*b);
                    int n = pullWaitlist(*b);
                    if (n > 0) {
                        cout << ">> [สำเร็จ] ดึงคิวเข้าโต๊ะแล้ว " << n << " คิว (เหลือคิวรอ " << b->waitlist.size() << " คิว)\n";
                        saveData(DATA_FILENAME, branches, custRoot);
                    } else {
                        cout << ">> ยังไม่มีโต๊ะว่างที่รองรับคิวแรก (" << b->waitlist.front().usualPax << " ท่าน)\n";
                    }
                }
            }
            pausePrompt();
        } else if (choice == 6) {
            printHeader("จัดการคิวรอ (CLEAR / DROP WAITLIST)");
            Branch* b = pickBranch(branches);
            if (b) {
                if (b->waitlist.empty()) {
                    cout << "\n>> ไม่มีคิวรอที่สาขานี้\n";
                } else {
                    displayWaitlist(*b);
                    cout << "  - พิมพ์ 'all' หรือ 'y' เพื่อล้างคิวทั้งหมดของสาขา\n";
                    cout << "  - หรือพิมพ์เบอร์โทรศัพท์ เพื่อตัดคิวเฉพาะรายที่สละสิทธิ์/ไม่มาแสดงตัว\n";
                    string ans = readString("ระบุคำสั่ง (all / เบอร์โทร / กด Enter เพื่อยกเลิก): ");
                    if (ans == "all" || ans == "y" || ans == "Y") {
                        cout << ">> [สำเร็จ] ล้างคิวแล้ว " << clearWaitlist(*b) << " คิว\n";
                        saveData(DATA_FILENAME, branches, custRoot);
                    } else if (validPhone(ans)) {
                        bool found = false;
                        queue<Customer> newQ;
                        while (!b->waitlist.empty()) {
                            if (b->waitlist.front().phone == ans) {
                                found = true;
                            } else {
                                newQ.push(b->waitlist.front());
                            }
                            b->waitlist.pop();
                        }
                        b->waitlist = newQ;
                        if (found) {
                            cout << ">> [สำเร็จ] ตัดคิวเบอร์ " << ans << " ออกจากคิวรอแล้ว\n";
                            dataDirty = true;
                            saveData(DATA_FILENAME, branches, custRoot);
                            int pulled = pullWaitlist(*b);
                            if (pulled > 0) cout << ">> [ระบบดึงคิว] จัดคิวถัดไปเข้าโต๊ะว่างแล้ว " << pulled << " คิว\n";
                        } else {
                            cout << ">> ไม่พบคิวของเบอร์ " << ans << " ในสาขานี้\n";
                        }
                    } else {
                        cout << ">> ยกเลิก ไม่มีการเปลี่ยนแปลง\n";
                    }
                }
            }
            pausePrompt();
        } else if (choice == 7) {
            printHeader("แยกโต๊ะที่เชื่อมไว้ (UNMERGE TABLES)");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTablesTable(*b);
                int id = readInt("ระบุรหัสโต๊ะใดก็ได้ในชุดที่ต้องการแยก: ");
                if (unmergeTables(*b, id)) {
                    cout << "\n>> [สำเร็จ] แยกชุดโต๊ะที่มีโต๊ะ " << id << " ออกเป็นโต๊ะเดี่ยวแล้ว\n";
                    saveData(DATA_FILENAME, branches, custRoot);
                } else {
                    cout << "\n>> แยกไม่ได้ (ไม่พบโต๊ะ / โต๊ะนี้ไม่ได้เชื่อมอยู่ / ชุดโต๊ะยังมีลูกค้านั่งอยู่)\n";
                }
            }
            pausePrompt();
        } else if (choice == 8) {
            printHeader("จัดโต๊ะให้ลูกค้า Walk-in (SEAT WALK-IN)");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTablesTable(*b);
                string phone = readString("เบอร์โทรศัพท์ (Phone): ");
                string name = readString("ชื่อลูกค้า (Name): ");
                int pax = readInt("จำนวนผู้ใช้บริการ (Pax): ");

                if (pax <= 0 || !validPhone(phone) || !validField(name)) {
                    cout << "\n>> ข้อมูลไม่ถูกต้อง (เบอร์ต้องเป็นตัวเลขล้วน, ชื่อห้ามว่างและห้ามมี '|', คนต้อง > 0)\n";
                } else if (pax > branchCapacity(*b)) {
                    cout << "\n>> สาขานี้รับได้สูงสุด " << branchCapacity(*b) << " ท่าน ไม่สามารถรับกลุ่มนี้ได้\n";
                } else if (inAnyWaitlist(branches, phone)) {
                    cout << "\n>> เบอร์นี้อยู่ในคิวรอแล้ว กรุณาดึงคิวจากระบบคิวรอ\n";
                } else {
                    Customer* existing = bstFind(custRoot, phone);
                    Customer c = existing ? *existing : Customer{phone, name, 0, pax};
                    c.name = name;
                    c.visitCount++;
                    c.usualPax = pax;

                    Table* t = b->waitlist.empty() ? findBestFitTable(*b, pax) : nullptr;
                    if (t) {
                        seatCustomer(*b, t, c);
                        cout << "\n>> [สำเร็จ] จัดโต๊ะ " << t->id << " (ขนาด " << totalMergedCapacity(*b, t->id)
                             << " ท่าน) ให้คุณ " << c.name << " เรียบร้อยแล้ว!\n";
                    } else {
                        cout << "\n>> ขณะนี้ไม่มีโต๊ะว่างที่เหมาะสมหรือมีคิวก่อนหน้า\n";
                        string waitAns = readString("ต้องการเพิ่มเข้าคิวรอหรือไม่? (Y/n): ");
                        if (waitAns != "n" && waitAns != "N") {
                            b->waitlist.push(c);
                            cout << ">> เพิ่มคุณ " << c.name << " เข้าคิวรอแล้ว (ลำดับคิวที่ " << b->waitlist.size() << ")\n";
                        }
                    }
                    custRoot = bstInsert(custRoot, c);
                    dataDirty = true;
                    saveData(DATA_FILENAME, branches, custRoot);
                }
            }
            pausePrompt();
        } else if (choice == 9) {
            printHeader("รายงานโต๊ะว่าง (FREE TABLE REPORT)");
            displayFreeTableReport(branches);
            pausePrompt();
        } else if (choice == 10) {
            printHeader("สั่งอาหารเพิ่มที่โต๊ะ (ADD ORDERS)");
            Branch* b = pickBranch(branches);
            if (b) {
                if (addOrdersFlow(*b) > 0) saveData(DATA_FILENAME, branches, custRoot);
            }
            pausePrompt();
        } else if (choice == 11) {
            printHeader("รายการอาหารบนโต๊ะ (TABLE ORDERS)");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTableOrders(*b);
            }
            pausePrompt();
        } else if (choice == 12) {
            printHeader("ผังโต๊ะแบบภาพ (TABLE MAP)");
            Branch* b = pickBranch(branches);
            if (b) {
                displayTableMap(*b);
            }
            pausePrompt();
        } else if (choice == 13) {
            printHeader("กระดานเมนูอาหาร (MENU BOARD)");
            displayMenuBoard();
            pausePrompt();
        } else if (choice == 14) {
            printHeader("ผังโต๊ะบนหน้าเว็บ (WEB TABLE MAP)");
            if (exportHtmlMap(HTML_FILENAME, branches)) {
                cout << ">> สร้างไฟล์ " << HTML_FILENAME << " แล้ว กำลังเปิดในเบราว์เซอร์...\n";
                cout << ">> หน้าเว็บจะรีเฟรชเองทุก 4 วินาที และอัปเดตทุกครั้งที่โปรแกรมบันทึกข้อมูล\n";
                cout << ">> ถ้าเบราว์เซอร์ไม่เปิดเอง ให้ดับเบิลคลิกไฟล์ " << HTML_FILENAME << " ในโฟลเดอร์เดียวกับโปรแกรม\n";
                openInBrowser(HTML_FILENAME);
            } else {
                cout << ">> [เกิดข้อผิดพลาด] สร้างไฟล์ " << HTML_FILENAME << " ไม่ได้\n";
            }
            pausePrompt();
        }
    }
}

static void adminMenu(vector<Branch>& branches) {
    while (true) {
        printHeader("ผู้จัดการและสถิติ (ADMIN & ANALYTICS)");
        cout << "  1) จัดอันดับสาขายอดนิยม (Branch Ranking - Merge Sort)\n";
        cout << "  2) คำนวณโปรโมชั่น (Promotion Engine - Shunting-Yard)\n";
        cout << "  3) แดชบอร์ดภาพรวมทุกสาขา (Overview Dashboard - Recursion)\n";
        cout << "  0) ย้อนกลับ (Back)\n\n";

        int choice = readInt("เลือกเมนู: ");
        if (choice == 0) break;

        if (choice == 1) {
            printHeader("จัดอันดับสาขาตามจำนวนลูกค้าวันนี้ (MERGE SORT)");
            vector<Branch*> ranked;
            for (auto& b : branches) ranked.push_back(&b);
            mergeSortBranches(ranked, 0, (int)ranked.size() - 1);

            cout << "+------+------------------------------------+-------------------+\n";
            cout << "| อันดับ| สาขา                               | ลูกค้าวันนี้ (คน)  |\n";
            cout << "+------+------------------------------------+-------------------+\n";
            for (int i = 0; i < (int)ranked.size(); i++) {
                cout << "|  #" << left << setw(3) << (i + 1) << " | "
                     << left << setw(34) << ranked[i]->name << " | "
                     << right << setw(13) << ranked[i]->customersToday << " ท่าน |\n";
            }
            cout << "+------+------------------------------------+-------------------+\n";
            pausePrompt();
        } else if (choice == 2) {
            printHeader("คำนวณส่วนลดโปรโมชั่น (INFIX EVALUATOR)");
            cout << "  ตัวอย่างเงื่อนไข: pax >= 4, total > 1000, (pax >= 2 && total >= 500)\n\n";
            string expr = readString("กรอกเงื่อนไขโปรโมชั่น: ");
            if (expr.empty()) expr = "pax >= 4";
            double pct = readDouble("เปอร์เซ็นต์ส่วนลด (%): ");
            double flat = readDouble("ส่วนลดเงินสด (บาท): ");
            int pax = readInt("จำนวนคน (pax): ");
            double total = readDouble("ยอดบิลก่อนลด (บาท): ");

            PromotionRule promo{expr, pct, flat};
            bool exprOk = true;
            double finalPrice = applyPromotion(promo, pax, total, &exprOk);

            cout << "\n+------------------------------------------------+\n";
            cout << "| สรุปการคิดส่วนลด                                |\n";
            cout << "+------------------------------------------------+\n";
            cout << "  เงื่อนไข: " << expr << "\n";
            cout << "  ยอดตั้งต้น: " << fixed << setprecision(2) << total << " บาท\n";
            if (!exprOk) {
                cout << "  >> [ผิดพลาด] เงื่อนไขไม่ถูกต้องตามรูปแบบ (เช่น ขาดตัวเลข/วงเล็บไม่ครบ) ไม่คิดส่วนลด\n";
            } else if (finalPrice < total) {
                cout << "  >> ได้รับส่วนลด! ลดไป: " << (total - finalPrice) << " บาท\n";
                cout << "  >> ยอดสุทธิที่ต้องชำระ: " << finalPrice << " บาท\n";
            } else {
                cout << "  >> ไม่ตรงตามเงื่อนไขโปรโมชั่น (ชำระราคาเต็ม: " << total << " บาท)\n";
            }
            cout << "+------------------------------------------------+\n";
            pausePrompt();
        } else if (choice == 3) {
            printHeader("แดชบอร์ดภาพรวมระบบ (RECURSIVE AGGREGATION)");
            Stats s = aggregateStats(branches);
            cout << "+------------------------------------------------+\n";
            cout << "| ข้อมูลสถิติภาพรวมทุกสาขา                        |\n";
            cout << "+------------------------------------------------+\n";
            cout << "  จำนวนลูกค้าที่มาใช้บริการวันนี้ : " << s.totalCustomers << " ท่าน\n";
            cout << "  จำนวนโต๊ะทั้งหมดในระบบ         : " << s.totalTables << " โต๊ะ\n";
            cout << "  จำนวนโต๊ะที่ว่างตอนนี้          : " << s.freeTables << " โต๊ะ\n";
            cout << "  ยอดขายรวมทั้งหมด              : " << fixed << setprecision(2) << s.totalSales << " บาท\n";
            cout << "+------------------------------------------------+\n";
            pausePrompt();
        }
    }
}


// ============================================================================
// Activity 2: Search Performance Evaluation (Integrated Module)
// รายวิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา
// กิจกรรมนำเสนอโครงงาน: แข่งขันวัดเวลาการค้นหา 5 ครั้ง พร้อม Trimmed Mean
// ============================================================================
namespace Activity2 {
    const int MAXN = 100000;
    const int REPEAT = 10000;
    const int NUM_RUNS = 5;

    static int data[MAXN];
    static int sorted_data[MAXN];

    struct BSTNode {
        int key;
        BSTNode *left;
        BSTNode *right;
    };

    static BSTNode bst_pool[MAXN];
    static int bst_pool_idx = 0;
    static BSTNode *bst_root = nullptr;

    // cap = capacity of arr: a header count bigger than the buffer must not overflow it.
    // Returns how many values were actually read (not the header), so a short file can't leave garbage.
    static int LoadData(const char *filename, int arr[], int cap) {
        FILE *fp = fopen(filename, "r");
        if (fp == nullptr) {
            printf("เปิดไฟล์ %s ไม่ได้\n", filename);
            return -1;
        }
        int count = 0;
        if (fscanf(fp, "%d", &count) != 1) {
            fclose(fp);
            return -1;
        }
        if (count > cap) count = cap;
        int i = 0;
        while (i < count && fscanf(fp, "%d", &arr[i]) == 1) i++;
        fclose(fp);
        return i;
    }

    // (a > b) - (a < b): plain a - b overflows for far-apart ints and mis-sorts.
    static int CompareInt(const void *a, const void *b) {
        int x = *(const int *)a, y = *(const int *)b;
        return (x > y) - (x < y);
    }

    static int SequentialSearch(int arr[], int size, int target) {
        for (int i = 0; i < size; i++)
            if (arr[i] == target) return i;
        return -1;
    }

    static int BinarySearch(int arr[], int size, int target) {
        int first = 0, last = size - 1;
        while (first <= last) {
            int mid = (first + last) / 2;
            if (target > arr[mid])      first = mid + 1;
            else if (target < arr[mid]) last  = mid - 1;
            else                        return mid;
        }
        return -1;
    }

    static BSTNode* BuildBalancedBST(int arr[], int start, int end) {
        if (start > end) return nullptr;
        int mid = (start + end) / 2;
        BSTNode *node = &bst_pool[bst_pool_idx++];
        node->key = arr[mid];
        node->left = BuildBalancedBST(arr, start, mid - 1);
        node->right = BuildBalancedBST(arr, mid + 1, end);
        return node;
    }

    static void InitBST(int arr[], int size) {
        bst_pool_idx = 0;
        bst_root = BuildBalancedBST(arr, 0, size - 1);
    }

    static int BSTSearch(BSTNode *root, int target) {
        BSTNode *curr = root;
        while (curr != nullptr) {
            if (target == curr->key) return 1;
            if (target < curr->key)  curr = curr->left;
            else                     curr = curr->right;
        }
        return -1;
    }

    static int MySearch(int arr[], int size, int target) {
        if (bst_root == nullptr) {
            InitBST(sorted_data, size);
        }
        return BSTSearch(bst_root, target);
    }

    static double MeasureMillisec(int (*SearchFunc)(int[], int, int),
                                  int arr[], int size, int targets[], int tcount) {
        clock_t start, end;
        double  total_sec;
        int     result = 0;

        start = clock();
        for (int r = 0; r < REPEAT; r++)
            for (int i = 0; i < tcount; i++)
                result += SearchFunc(arr, size, targets[i]);
        end = clock();

        if (result == -99999999) printf(" ");

        total_sec = (double)(end - start) / CLOCKS_PER_SEC;
        return total_sec * 1000.0 / (REPEAT * tcount);
    }

    static double CalcTrimmedMean(double arr[], int count) {
        double sorted[NUM_RUNS];
        for (int i = 0; i < count; i++) sorted[i] = arr[i];
        for (int i = 0; i < count - 1; i++) {
            for (int j = i + 1; j < count; j++) {
                if (sorted[i] > sorted[j]) {
                    double tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }
        double sum = 0.0;
        for (int i = 1; i < count - 1; i++) {
            sum += sorted[i];
        }
        return sum / (count - 2);
    }

    static void runDatasetEvaluation(const char* data_file, const char* targets_file) {
        int targets[100];
        int n = LoadData(data_file, data, MAXN);
        int tcount = LoadData(targets_file, targets, 100);
        if (n <= 0 || tcount <= 0) {
            printf(">> ข้ามชุดข้อมูล %s (ไม่พบไฟล์)\n\n", data_file);
            return;
        }

        for (int i = 0; i < n; i++) sorted_data[i] = data[i];
        qsort(sorted_data, n, sizeof(int), CompareInt);
        InitBST(sorted_data, n);

        printf("=====================================================\n");
        printf(" ไฟล์ข้อมูล      : %s\n", data_file);
        printf(" จำนวนข้อมูล n   : %d\n", n);
        printf(" จำนวนค่าที่ค้นหา : %d\n", tcount);
        printf(" จำนวนรอบที่วัด  : %d รอบต่อค่า\n", REPEAT);
        printf("=====================================================\n");

        printf("\n[ ตรวจความถูกต้องของผลการค้นหา ]\n");
        printf(" ค่าที่ค้นหา Sequential Binary     MySearch\n");
        for (int i = 0; i < tcount; i++) {
            int a = SequentialSearch(data, n, targets[i]);
            int b = BinarySearch(sorted_data, n, targets[i]);
            int c = MySearch(data, n, targets[i]);
            printf(" %-12d %s %s %s\n", targets[i],
                   (a >= 0) ? "พบ    " : "ไม่พบ ",
                   (b >= 0) ? "พบ    " : "ไม่พบ ",
                   (c >= 0) ? "พบ" : "ไม่พบ");
        }

        double seq_times[NUM_RUNS], bin_times[NUM_RUNS], my_times[NUM_RUNS];
        for (int run = 0; run < NUM_RUNS; run++) {
            seq_times[run] = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
            bin_times[run] = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
            my_times[run]  = MeasureMillisec(MySearch,         data,        n, targets, tcount);
        }

        printf("\n[ ผลการวัดเวลา 5 ครั้ง ]\n");
        printf(" ครั้งที่   Sequential (ms)    Binary Search (ms)       MySearch (ms)\n");
        printf(" --------------------------------------------------------------------\n");
        for (int run = 0; run < NUM_RUNS; run++) {
            printf("  [%d]           %.6f             %.6f             %.6f\n",
                   run + 1, seq_times[run], bin_times[run], my_times[run]);
        }
        printf(" --------------------------------------------------------------------\n");
        printf(" ค่าเฉลี่ย 3 ครั้งที่เหลือ (Trimmed Mean):\n");
        printf("  Sequential Search : %.6f ms\n", CalcTrimmedMean(seq_times, NUM_RUNS));
        printf("  Binary Search     : %.6f ms\n", CalcTrimmedMean(bin_times, NUM_RUNS));
        printf("  MySearch (BST)    : %.6f ms\n", CalcTrimmedMean(my_times, NUM_RUNS));
        printf("=====================================================\n\n");
    }

    static void runAll() {
        runDatasetEvaluation("data_1000.txt", "targets_1000.txt");
        runDatasetEvaluation("data_10000.txt", "targets_10000.txt");
        runDatasetEvaluation("data_100000.txt", "targets_100000.txt");
    }
}

// Main Entrypoint
int main(int argc, char** argv) {
    bool isTerminal = true;
#ifdef _WIN32
    isTerminal = _isatty(_fileno(stdin));
#else
    isTerminal = isatty(fileno(stdin));
#endif

    if (!isTerminal) {
        ios_base::sync_with_stdio(false);
        cin.tie(NULL);
    }

#ifdef _WIN32
    if (isTerminal) {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    }
#endif

    if (argc > 1) {
        string arg = argv[1];
        if (arg == "--test") {
            runSelfTest();
            return 0;
        } else if (arg == "--benchmark") {
            runBenchmark();
            return 0;
        } else if (arg == "--eval" || arg == "--activity2" || arg == "--timing") {
            Activity2::runAll();
            return 0;
        }
    }

    vector<Branch> branches;
    CustNode* custRoot = nullptr;

    // A save file that parses but holds zero branches would leave the app with nothing to book into.
    if (!loadData(DATA_FILENAME, branches, custRoot) || branches.empty()) {
        branches = seedBranches();
    }

    while (true) {
        if (isTerminal) {
            printHeader("TEENOI SUKI - RESTAURANT MANAGEMENT SYSTEM");
            cout << "  1) เมนูลูกค้า (Customer)\n";
            cout << "  2) เมนูพนักงาน (Staff)\n";
            cout << "  3) สถิติและผู้จัดการ (Admin & Analytics)\n";
            cout << "  4) ประเมินผลการค้นหา กิจกรรมที่ 2 (Activity 2: Search Evaluation)\n";
            cout << "  5) บันทึกข้อมูลทันที (Save Data to " << DATA_FILENAME << ")\n";
            cout << "  0) บันทึกและออกจากระบบ (Save & Exit)\n\n";
        }

        int choice = readInt(isTerminal ? "เลือกเมนูหลัก: " : "");
        if (choice == 0) {
            if (dataDirty) saveData(DATA_FILENAME, branches, custRoot);
            if (isTerminal) {
                clearScreen();
                cout << "\n+======================================================+\n";
                cout << "| บันทึกข้อมูลเรียบร้อย ขอบคุณที่ใช้บริการ Teenoi Suki!  |\n";
                cout << "+======================================================+\n\n";
            }
            break;
        } else if (choice == 1) {
            customerMenu(branches, custRoot);
        } else if (choice == 2) {
            staffMenu(branches, custRoot);
        } else if (choice == 3) {
            adminMenu(branches);
        } else if (choice == 4) {
            if (isTerminal) {
                printHeader("ประเมินผลการค้นหา กิจกรรมที่ 2 (ACTIVITY 2 EVALUATION)");
            }
            Activity2::runAll();
            if (isTerminal) pausePrompt();
        } else if (choice == 5) {
            bool saved = saveData(DATA_FILENAME, branches, custRoot);
            if (isTerminal) {
                if (saved) cout << "\n>> [สำเร็จ] บันทึกข้อมูลเรียบร้อยแล้ว!\n";
                else cout << "\n>> [เกิดข้อผิดพลาด] ไม่สามารถบันทึกไฟล์ได้\n";
                pausePrompt();
            }
        }
    }

    return 0;
}
