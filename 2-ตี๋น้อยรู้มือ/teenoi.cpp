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

struct Table {
    int id;
    int capacity;
    bool isAvailable = true;
    string customerName;
    int mergedNextId = -1; // linked-list pointer via id, -1 = none
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

// ---------------- Best-fit table (sort + binary search) ----------------
Table* findBestFitTable(Branch& b, int pax) {
    vector<Table*> avail;
    avail.reserve(b.tables.size());
    for (auto& t : b.tables) if (t.isAvailable) avail.push_back(&t);
    sort(avail.begin(), avail.end(), [](Table* a, Table* c) { return a->capacity < c->capacity; });
    auto it = lower_bound(avail.begin(), avail.end(), pax,
                           [](Table* t, int p) { return t->capacity < p; });
    return it != avail.end() ? *it : nullptr;
}

// ---------------- Table merge (linked-list pointer chain) ----------------
Table* findTable(Branch& b, int id) {
    for (auto& t : b.tables) if (t.id == id) return &t;
    return nullptr;
}

int totalMergedCapacity(Branch& b, int startId) {
    int cap = 0, id = startId;
    while (id != -1) {
        Table* t = findTable(b, id);
        if (!t) break;
        cap += t->capacity;
        id = t->mergedNextId;
    }
    return cap;
}

bool mergeTables(Branch& b, int id1, int id2) {
    Table* t1 = findTable(b, id1);
    Table* t2 = findTable(b, id2);
    if (!t1 || !t2 || id1 == id2) return false;
    while (t1->mergedNextId != -1) {
        if (t1->mergedNextId == id2) return false; // already merged
        t1 = findTable(b, t1->mergedNextId);
    }
    t1->mergedNextId = id2;
    return true;
}

// ---------------- Undo (stack) ----------------
stack<Action> history;
bool dataDirty = false; // skip disk write on exit if nothing changed

void seatCustomer(Branch& b, Table* t, const Customer& c) {
    history.push({"SEAT", b.id, t->id, t->customerName, t->isAvailable});
    t->isAvailable = false;
    t->customerName = c.name;
    b.customersToday++;
    dataDirty = true;
}

// Checkout: free the table, then pull the FRONT waitlist customer if (and
void checkoutTable(Branch& b, Table* t) {
    history.push({"CHECKOUT", b.id, t->id, t->customerName, t->isAvailable});
    t->isAvailable = true;
    t->customerName.clear();
    b.tableTurnsToday++;
    dataDirty = true;
    if (!b.waitlist.empty() && b.waitlist.front().usualPax <= t->capacity) {
        Customer c = b.waitlist.front();
        b.waitlist.pop();
        seatCustomer(b, t, c);
    }
}

bool undoLast(vector<Branch>& branches) {
    if (history.empty()) return false;
    Action a = history.top(); history.pop();
    for (auto& b : branches) {
        if (b.id != a.branchIdx) continue;
        Table* t = findTable(b, a.tableId);
        if (!t) return false;
        if (a.type == "SEAT" && !t->isAvailable) b.customersToday--;
        if (a.type == "CHECKOUT" && t->isAvailable) b.tableTurnsToday--;
        t->isAvailable = a.prevAvailable;
        t->customerName = a.prevCustomerName;
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

vector<string> infixToPostfix(const vector<string>& tokens) {
    vector<string> output;
    stack<string> ops;
    for (auto& tok : tokens) {
        if (isdigit((unsigned char)tok[0]) || tok[0] == '.' || isalpha((unsigned char)tok[0])) {
            output.push_back(tok);
        } else if (tok == "(") {
            ops.push(tok);
        } else if (tok == ")") {
            while (!ops.empty() && ops.top() != "(") { output.push_back(ops.top()); ops.pop(); }
            if (!ops.empty()) ops.pop();
        } else {
            while (!ops.empty() && ops.top() != "(" && precedence(ops.top()) >= precedence(tok)) {
                output.push_back(ops.top()); ops.pop();
            }
            ops.push(tok);
        }
    }
    while (!ops.empty()) { output.push_back(ops.top()); ops.pop(); }
    return output;
}

double evalPostfix(const vector<string>& postfix, map<string, double>& vars) {
    stack<double> st;
    for (auto& tok : postfix) {
        if (isdigit((unsigned char)tok[0]) || tok[0] == '.') {
            st.push(stod(tok));
        } else if (isalpha((unsigned char)tok[0])) {
            st.push(vars.count(tok) ? vars[tok] : 0);
        } else {
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
            double r = 0;
            if (tok == "+") r = a + b;
            else if (tok == "-") r = a - b;
            else if (tok == "*") r = a * b;
            else if (tok == "/") r = (b != 0) ? a / b : 0;
            else if (tok == ">=") r = a >= b;
            else if (tok == "<=") r = a <= b;
            else if (tok == ">") r = a > b;
            else if (tok == "<") r = a < b;
            else if (tok == "==") r = a == b;
            else if (tok == "!=") r = a != b;
            else if (tok == "&&") r = (a != 0 && b != 0);
            else if (tok == "||") r = (a != 0 || b != 0);
            st.push(r);
        }
    }
    return st.empty() ? 0 : st.top();
}

double applyPromotion(const PromotionRule& promo, int pax, double total) {
    map<string, double> vars = {{"pax", (double)pax}, {"total", total}};
    auto postfix = infixToPostfix(tokenize(promo.expression));
    if (evalPostfix(postfix, vars) == 0) return total;
    double result = total;
    if (promo.percentDiscount > 0) result -= result * promo.percentDiscount / 100.0;
    if (promo.flatDiscount > 0) result -= promo.flatDiscount;
    return max(0.0, result);
}

// ---------------- Dashboard stats (recursion) ----------------
struct Stats { int totalCustomers = 0; int totalTables = 0; double totalSales = 0; };

Stats aggregateStats(vector<Branch>& branches, int idx = 0) {
    if (idx >= (int)branches.size()) return Stats{};
    Stats s = aggregateStats(branches, idx + 1);
    s.totalCustomers += branches[idx].customersToday;
    s.totalTables += (int)branches[idx].tables.size();
    s.totalSales += branches[idx].salesToday;
    return s;
}

// ---------------- Save & Load (File Storage) ----------------
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
                << t.mergedNextId << "|" << t.customerName << "\n";
        }

        queue<Customer> qCopy = b.waitlist;
        ofs << "[WAITLIST] " << qCopy.size() << "\n";
        while (!qCopy.empty()) {
            const Customer& wc = qCopy.front();
            ofs << wc.phone << "|" << wc.name << "|" << wc.visitCount << "|" << wc.usualPax << "\n";
            qCopy.pop();
        }
    }
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

    // merge tables (linked list)
    assert(mergeTables(branches[0], 101, 102));
    assert(totalMergedCapacity(branches[0], 101) == 6);
    assert(!mergeTables(branches[0], 101, 102)); // already merged

    // seat + checkout + waitlist FIFO pull + undo
    Branch& b = branches[0];
    Table* t103 = findTable(b, 103);
    b.waitlist.push({"0833333333", "Waiting1", 0, 6});
    seatCustomer(b, t103, {"0844444444", "Guest1", 0, 5});
    assert(!t103->isAvailable && b.customersToday == 1);
    checkoutTable(b, t103); // frees then re-seats the queued party (fits: 6<=8)
    assert(!t103->isAvailable && t103->customerName == "Waiting1");
    assert(b.waitlist.empty());
    undoLast(branches); // undoes the auto-reseat (SEAT)
    assert(t103->isAvailable);
    undoLast(branches); // undoes the checkout
    undoLast(branches); // undoes the original seat
    assert(t103->customerName.empty());

    // merge sort ranking
    branches[0].customersToday = 5;
    branches[1].customersToday = 20;
    branches[2].customersToday = 10;
    vector<Branch*> ranked = {&branches[0], &branches[1], &branches[2]};
    mergeSortBranches(ranked, 0, (int)ranked.size() - 1);
    assert(ranked[0]->id == 2 && ranked[1]->id == 3 && ranked[2]->id == 1);

    // promotion: 10% off when pax >= 4, plus flat 50 baht
    PromotionRule promo{"pax >= 4", 10, 50};
    double price = applyPromotion(promo, 4, 1000);
    assert(fabs(price - 850) < 1e-9); // 1000*0.9 - 50
    assert(applyPromotion(promo, 2, 1000) == 1000); // condition false -> untouched

    // recursion stats
    branches[0].tables = {{101, 2}, {102, 4}};
    branches[1].tables = {{201, 4}};
    branches[2].tables = {};
    Stats s = aggregateStats(branches);
    assert(s.totalTables == 3);
    assert(s.totalCustomers == branches[0].customersToday + branches[1].customersToday + branches[2].customersToday);

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

static void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

static int readInt(const string& prompt, int defaultVal = -1) {
    if (cin.eof()) return 0;
    cout << prompt;
    int val;
    if (!(cin >> val)) {
        if (cin.eof()) return 0;
        clearInputBuffer();
        return defaultVal;
    }
    clearInputBuffer();
    return val;
}

static double readDouble(const string& prompt, double defaultVal = 0.0) {
    if (cin.eof()) return defaultVal;
    cout << prompt;
    double val;
    if (!(cin >> val)) {
        if (cin.eof()) return defaultVal;
        clearInputBuffer();
        return defaultVal;
    }
    clearInputBuffer();
    return val;
}

static string readString(const string& prompt) {
    if (cin.eof()) return "";
    cout << prompt;
    string val;
    if (!getline(cin, val)) return "";
    if (!val.empty() && val.back() == '\r') val.pop_back();
    return val;
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
        string mergeStr = (t.mergedNextId != -1) ? ("เชื่อม -> โต๊ะ " + to_string(t.mergedNextId)) : "-";
        
        cout << "| " << right << setw(4) << t.id << " | "
             << right << setw(6) << t.capacity << " คน | "
             << left << setw(17) << statusStr << " | "
             << left << setw(21) << occupant << " | "
             << left << setw(20) << mergeStr << " |\n";
    }
    cout << "+------+-----------+-----------------+-----------------------+----------------------+\n";
    cout << "  * คิวรอขณะนี้: " << b.waitlist.size() << " คิว\n\n";
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

                if (pax <= 0 || phone.empty() || name.empty()) {
                    cout << "\n>> ข้อมูลไม่ถูกต้อง กรุณากรอกใหม่อีกครั้ง\n";
                } else {
                    Customer* existing = bstFind(custRoot, phone);
                    Customer c = existing ? *existing : Customer{phone, name, 0, pax};
                    c.name = name; // update name if changed
                    c.visitCount++;
                    c.usualPax = pax;

                    Table* t = findBestFitTable(*b, pax);
                    if (t) {
                        seatCustomer(*b, t, c);
                        cout << "\n>> [สำเร็จ] จัดโต๊ะ " << t->id << " (ขนาด " << t->capacity << " ท่าน) ให้คุณ " << c.name << " เรียบร้อยแล้ว!\n";
                    } else {
                        b->waitlist.push(c);
                        cout << "\n>> โต๊ะที่เหมาะสมเต็มทั้งหมด! เพิ่มคุณ " << c.name << " เข้าคิวรอที่สาขา " << b->name << "\n";
                        cout << ">> ลำดับคิวของคุณ: คิวที่ " << b->waitlist.size() << "\n";
                    }
                    custRoot = bstInsert(custRoot, c);
                    dataDirty = true;
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
                    cout << "\n>> ไม่สามารถเชื่อมโต๊ะได้ (รหัสโต๊ะไม่ถูกต้อง หรือถูกเชื่อมต่ออยู่แล้ว)\n";
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
                        checkoutTable(*b, t);
                        cout << "\n>> [สำเร็จ] เช็คบิลโต๊ะ " << id << " เรียบร้อย (ลูกค้าเดิม: " << prevGuest << ")\n";
                        if (!t->isAvailable) {
                            cout << ">> [ระบบดึงคิวอัตโนมัติ] โต๊ะนี้ถูกจัดให้คิวรอ: คุณ " << t->customerName << " เรียบร้อยแล้ว!\n";
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
            double finalPrice = applyPromotion(promo, pax, total);

            cout << "\n+------------------------------------------------+\n";
            cout << "| สรุปการคิดส่วนลด                                |\n";
            cout << "+------------------------------------------------+\n";
            cout << "  เงื่อนไข: " << expr << "\n";
            cout << "  ยอดตั้งต้น: " << fixed << setprecision(2) << total << " บาท\n";
            if (finalPrice < total) {
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

    static int LoadData(const char *filename, int arr[]) {
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
        for (int i = 0; i < count; i++) {
            if (fscanf(fp, "%d", &arr[i]) != 1) break;
        }
        fclose(fp);
        return count;
    }

    static int CompareInt(const void *a, const void *b) {
        return (*(const int *)a) - (*(const int *)b);
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
        int n = LoadData(data_file, data);
        int tcount = LoadData(targets_file, targets);
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

    if (!loadData(DATA_FILENAME, branches, custRoot)) {
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
