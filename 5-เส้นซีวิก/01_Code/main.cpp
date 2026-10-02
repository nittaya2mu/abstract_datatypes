// ============================================================================
// โครงงาน: ระบบช่วยเคลียร์ตู้เย็นและแนะนำเมนูอาหารอัจฉริยะ (Smart Fridge Clearer)
// คอมไพล์: g++ -std=c++17 -O2 -o fridge main1.cpp
// รันปกติ:  fridge.exe
// รันเทส:   fridge.exe --test
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <map>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <limits>
#include <cassert>
#include <cctype>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

// ============================================================================
// [Data Entity] โครงสร้างข้อมูลอาหารในตู้เย็น
// ============================================================================
struct FoodItem {
    int id;
    std::string name;
    std::string category; // meat, vegetable, dairy, seafood, fruit, other
    std::string expiry;   // รูปแบบ: YYYY-MM-DD
};

// ============================================================================
// [ADT 1: Stack ADT] จำลองชั้นวางตู้เย็นตามหลัก LIFO (Last-In, First-Out)
// ของที่เพิ่งซื้อมาใส่ทีหลังจะอยู่หน้าสุด/บนสุด บังของเก่าที่อยู่ข้างใน/ก้นตู้
// ============================================================================
class ShelfStack {
private:
    std::vector<int> m_shelf; // เก็บ ID ตามลำดับการวาง (ก้นตู้ -> หน้าตู้)

public:
    void push(int id) {
        m_shelf.push_back(id);
    }

    bool pop(int& outId) {
        if (m_shelf.empty()) return false;
        outId = m_shelf.back();
        m_shelf.pop_back();
        return true;
    }

    bool empty() const {
        return m_shelf.empty();
    }

    size_t size() const {
        return m_shelf.size();
    }

    void removeId(int id) {
        m_shelf.erase(std::remove(m_shelf.begin(), m_shelf.end(), id), m_shelf.end());
    }

    const std::vector<int>& getRawOrder() const {
        return m_shelf;
    }

    void clear() {
        m_shelf.clear();
    }
};

// ============================================================================
// [ADT 2: Priority Queue / Min-Heap] คัดกรองของใกล้หมดอายุ
// วันหมดอายุน้อยที่สุด (ใกล้หมดอายุที่สุด) จะอยู่บนยอด Heap (top()) เสมอ
// ============================================================================
struct ExpiryMinHeapCompare {
    bool operator()(const FoodItem& a, const FoodItem& b) const {
        return a.expiry > b.expiry; // น้อยกว่าอยู่บนสุด (Min-Heap)
    }
};
using ExpiryMinHeap = std::priority_queue<FoodItem, std::vector<FoodItem>, ExpiryMinHeapCompare>;

// ============================================================================
// [ADT 3: Binary Search Tree (BST)] ค้นหาและจัดการวัตถุดิบด้วย ID
// รองรับ Insert, Search, Delete, In-Order Traversal ในเวลาเฉลี่ย O(log n)
// พร้อมระบบ Dynamic Memory Management (ป้องกัน Memory Leak 100%)
// ============================================================================
struct BSTNode {
    int id;
    FoodItem item;
    BSTNode* left;
    BSTNode* right;

    BSTNode(int _id, const FoodItem& _it)
        : id(_id), item(_it), left(nullptr), right(nullptr) {}
};

class FoodBST {
private:
    BSTNode* root;
    size_t nodeCount;

    void clearSubtree(BSTNode* node) {
        if (!node) return;
        clearSubtree(node->left);
        clearSubtree(node->right);
        delete node;
    }

    BSTNode* insertRec(BSTNode* node, const FoodItem& it) {
        if (!node) return new BSTNode(it.id, it);
        if (it.id < node->id)
            node->left = insertRec(node->left, it);
        else if (it.id > node->id)
            node->right = insertRec(node->right, it);
        else
            node->item = it; // update ข้อมูลหาก ID ซ้ำ
        return node;
    }

    BSTNode* findMin(BSTNode* node) const {
        while (node && node->left) node = node->left;
        return node;
    }

    BSTNode* removeRec(BSTNode* node, int id, bool& removed) {
        if (!node) return nullptr;

        if (id < node->id) {
            node->left = removeRec(node->left, id, removed);
        } else if (id > node->id) {
            node->right = removeRec(node->right, id, removed);
        } else {
            removed = true;
            // กรณี 1: ไม่มีลูก (Leaf node)
            if (!node->left && !node->right) {
                delete node;
                return nullptr;
            }
            // กรณี 2: มีลูกข้างเดียว
            if (!node->left) {
                BSTNode* temp = node->right;
                delete node;
                return temp;
            } else if (!node->right) {
                BSTNode* temp = node->left;
                delete node;
                return temp;
            }
            // กรณี 3: มีลูก 2 ข้าง (ใช้ Inorder Successor ค่าต่ำสุดฝั่งขวา)
            BSTNode* successor = findMin(node->right);
            node->id = successor->id;
            node->item = successor->item;
            node->right = removeRec(node->right, successor->id, removed);
        }
        return node;
    }

    void inorderRec(BSTNode* node, std::vector<FoodItem>& out) const {
        if (!node) return;
        inorderRec(node->left, out);
        out.push_back(node->item);
        inorderRec(node->right, out);
    }

public:
    FoodBST() : root(nullptr), nodeCount(0) {}

    // ป้องกันการ Copy ป้องกัน Double-Free Bug (Rule of Three)
    FoodBST(const FoodBST&) = delete;
    FoodBST& operator=(const FoodBST&) = delete;

    ~FoodBST() {
        clear();
    }

    void clear() {
        clearSubtree(root);
        root = nullptr;
        nodeCount = 0;
    }

    void insert(const FoodItem& item) {
        bool exists = (search(item.id) != nullptr);
        root = insertRec(root, item);
        if (!exists) ++nodeCount;
    }

    const FoodItem* search(int id) const {
        BSTNode* curr = root;
        while (curr) {
            if (id == curr->id) return &(curr->item);
            if (id < curr->id)  curr = curr->left;
            else                curr = curr->right;
        }
        return nullptr;
    }

    bool remove(int id) {
        bool removed = false;
        root = removeRec(root, id, removed);
        if (removed) --nodeCount;
        return removed;
    }

    std::vector<FoodItem> getInorder() const {
        std::vector<FoodItem> res;
        inorderRec(root, res);
        return res;
    }

    size_t size() const { return nodeCount; }
    bool empty() const { return root == nullptr; }
};

// ============================================================================
// ฐานข้อมูลหมวดหมู่และเมนูอาหาร
// ============================================================================
static const std::vector<std::pair<std::string, std::vector<std::string>>> kKeywordMenus = {
    {"ไข่", {"ไข่เจียว", "ไข่ตุ๋น"}},   {"หมู", {"หมูผัดกระเทียม", "ต้มจืดหมูสับ"}},
    {"ไก่", {"ไก่ผัดขิง", "ต้มยำไก่"}}, {"กุ้ง", {"ต้มยำกุ้ง", "กุ้งผัดกระเทียม"}},
    {"ปลา", {"ปลานึ่งมะนาว", "ต้มยำปลา"}}, {"นม", {"สมูทตี้นม"}},
};

static const std::map<std::string, std::vector<std::string>> kCategoryMenus = {
    {"meat", {"ผัดกะเพรา", "ต้มจืด"}},   {"vegetable", {"ผัดผักรวมมิตร", "แกงจืด"}},
    {"dairy", {"ไข่เจียว"}},             {"seafood", {"ต้มยำทะเล"}},
    {"fruit", {"สลัดผลไม้"}},            {"other", {"เมนูตามใจชอบ"}},
};

static const std::map<std::string, std::string> kCategoryLabel = {
    {"meat", "เนื้อสัตว์"}, {"vegetable", "ผัก"}, {"dairy", "นม/ไข่"},
    {"seafood", "อาหารทะเล"}, {"fruit", "ผลไม้"}, {"other", "อื่นๆ"},
};

// ============================================================================
// [Greedy Algorithm] แนะนำเมนูอาหารที่เคลียร์วัตถุดิบใกล้หมดอายุได้มากที่สุด
// ============================================================================
struct ComboMenu {
    std::string name;
    std::vector<std::string> categories;
};

static const std::vector<ComboMenu> kComboMenus = {
    {"ผัดรวมมิตร", {"meat", "vegetable", "seafood"}},
    {"แกงจืดรวม", {"meat", "vegetable", "dairy"}},
    {"สุกี้รวมมิตร", {"meat", "seafood", "vegetable", "dairy"}},
    {"ต้มยำรวม", {"seafood", "meat", "vegetable"}},
    {"สลัดผลไม้รวม", {"fruit", "dairy"}},
};

struct ComboResult {
    std::string menuName;
    std::vector<std::string> cleared;
};

std::vector<ComboResult> greedySuggestCombos(const std::vector<FoodItem>& urgent) {
    std::vector<ComboResult> results;
    std::vector<bool> used(urgent.size(), false);
    std::vector<bool> menuUsed(kComboMenus.size(), false);

    while (true) {
        int bestIdx = -1;
        std::vector<size_t> bestCover;

        for (size_t m = 0; m < kComboMenus.size(); ++m) {
            if (menuUsed[m]) continue;
            std::vector<size_t> cover;
            for (size_t i = 0; i < urgent.size(); ++i) {
                if (used[i]) continue;
                const auto& cats = kComboMenus[m].categories;
                if (std::find(cats.begin(), cats.end(), urgent[i].category) != cats.end()) {
                    cover.push_back(i);
                }
            }
            if (cover.size() > bestCover.size()) {
                bestCover = cover;
                bestIdx = (int)m;
            }
        }

        if (bestIdx == -1 || bestCover.empty()) break;

        ComboResult r{kComboMenus[bestIdx].name, {}};
        for (size_t idx : bestCover) {
            r.cleared.push_back(urgent[idx].name);
            used[idx] = true;
        }
        menuUsed[bestIdx] = true;
        results.push_back(r);

        if (std::all_of(used.begin(), used.end(), [](bool u) { return u; })) break;
    }
    return results;
}

// ============================================================================
// ฟังก์ชันจัดการวันที่และเวลา (Robust Date & Time Validation)
// ============================================================================
bool isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

int daysInMonth(int y, int m) {
    if (m < 1 || m > 12) return 0;
    static const int dom[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && isLeapYear(y)) return 29;
    return dom[m];
}

bool parseAndValidateDate(const std::string& str, int& y, int& m, int& d) {
    if (str.length() != 10) return false;
    if (str[4] != '-' || str[7] != '-') return false;

    for (size_t i = 0; i < str.length(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(str[i]))) return false;
    }

    if (sscanf(str.c_str(), "%4d-%2d-%2d", &y, &m, &d) != 3) return false;
    if (y < 1900 || y > 2100) return false;
    if (m < 1 || m > 12) return false;
    if (d < 1 || d > daysInMonth(y, m)) return false;

    return true;
}

long daysLeft(const std::string& expiry) {
    int y, m, d;
    if (!parseAndValidateDate(expiry, y, m, d)) return 0;

    struct tm t{};
    t.tm_year = y - 1900;
    t.tm_mon  = m - 1;
    t.tm_mday = d;
    t.tm_isdst = -1;

    time_t expiryTime = mktime(&t);
    time_t now = time(nullptr);
    struct tm today = *localtime(&now);
    today.tm_hour = today.tm_min = today.tm_sec = 0;
    today.tm_isdst = -1;

    double diffSec = difftime(expiryTime, mktime(&today));
    return static_cast<long>(std::round(diffSec / 86400.0));
}

std::string statusText(long d) {
    if (d < 0) return "หมดอายุแล้ว " + std::to_string(-d) + " วัน";
    if (d == 0) return "หมดอายุวันนี้";
    return "เหลือ " + std::to_string(d) + " วัน";
}

std::string categoryLabel(const std::string& c) {
    auto it = kCategoryLabel.find(c);
    return it != kCategoryLabel.end() ? it->second : c;
}

std::vector<std::string> suggestMenusFor(const FoodItem& item) {
    for (const auto& kv : kKeywordMenus) {
        if (item.name.find(kv.first) != std::string::npos) return kv.second;
    }
    auto it = kCategoryMenus.find(item.category);
    return it != kCategoryMenus.end() ? it->second : std::vector<std::string>{"เมนูตามใจชอบ"};
}

// ============================================================================
// Input Validation Helper Functions (ป้องกัน Infinite Loop & Crash)
// ============================================================================
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool safeReadLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    if (!std::getline(std::cin, out)) {
        return false; // เกิด EOF หรือสตรีมมีปัญหา
    }
    out = trim(out);
    return true;
}

int safeReadInt(const std::string& prompt, int lo, int hi) {
    while (true) {
        std::string line;
        if (!safeReadLine(prompt, line)) {
            // กรณีผู้ใช้กด Ctrl+Z / EOF ให้คืนค่า 0 เพื่อออกจากลูปอย่างปลอดภัย
            return 0;
        }
        if (line.empty()) {
            std::cout << "  -> กรุณากรอกตัวเลขระหว่าง " << lo << " ถึง " << hi << "\n";
            continue;
        }

        try {
            size_t idx = 0;
            int val = std::stoi(line, &idx);
            // ต้องแปลงได้หมดทั้งสตริง ห้ามมีตัวอักษรต่อท้าย เช่น 1abc
            if (idx == line.length() && val >= lo && val <= hi) {
                return val;
            }
        } catch (...) {
            // ดักจับ std::invalid_argument หรือ std::out_of_range
        }
        std::cout << "  -> ข้อมูลไม่ถูกต้อง! กรุณากรอกตัวเลขระหว่าง " << lo << " ถึง " << hi << "\n";
    }
}

void printItem(const FoodItem& it) {
    std::cout << "  [" << it.id << "] " << it.name << " (" << categoryLabel(it.category)
              << ") หมดอายุ " << it.expiry << " - " << statusText(daysLeft(it.expiry)) << "\n";
}

// ============================================================================
// Global State & Storage Management
// ============================================================================
static std::vector<FoodItem> g_items;
static ShelfStack            g_shelf;
static FoodBST               g_bst;
static int                   g_nextId = 1;
static const char*           kDataFile = "fridge_data.txt";

ExpiryMinHeap buildExpiryMinHeap() {
    ExpiryMinHeap heap;
    for (const auto& it : g_items) heap.push(it);
    return heap;
}

void rebuildBST() {
    g_bst.clear();
    for (const auto& it : g_items) {
        g_bst.insert(it);
    }
}

void saveToFile() {
    std::ofstream out(kDataFile);
    if (!out) return;
    for (const auto& it : g_items) {
        out << it.id << "|" << it.name << "|" << it.category << "|" << it.expiry << "\n";
    }
}

bool loadFromFile() {
    std::ifstream in(kDataFile);
    if (!in) return false;

    g_items.clear();
    g_shelf.clear();
    g_bst.clear();

    std::string line;
    bool loadedAny = false;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string idStr, name, category, expiry;
        if (!std::getline(iss, idStr, '|')) continue;
        if (!std::getline(iss, name, '|')) continue;
        if (!std::getline(iss, category, '|')) continue;
        if (!std::getline(iss, expiry, '|')) continue;

        try {
            int id = std::stoi(trim(idStr));
            FoodItem item{id, trim(name), trim(category), trim(expiry)};
            g_items.push_back(item);
            g_shelf.push(id);
            g_bst.insert(item);
            g_nextId = std::max(g_nextId, id + 1);
            loadedAny = true;
        } catch (...) {
            continue; // ข้ามบรรทัดที่ข้อมูลผิดพลาด
        }
    }
    return loadedAny;
}

// ============================================================================
// ฟังก์ชันเมนูการทำงานหลัก (ลบการแสดงความไวออกเพื่อความสะอาดตา)
// ============================================================================
void menuAddItem() {
    std::string name;
    while (true) {
        if (!safeReadLine("ชื่อของกิน: ", name)) return;
        if (!name.empty()) break;
        std::cout << "  -> ชื่อต้องไม่เป็นค่าว่าง กรุณากรอกใหม่\n";
    }

    std::vector<std::string> cats = {"meat", "vegetable", "dairy", "seafood", "fruit", "other"};
    for (size_t i = 0; i < cats.size(); ++i) {
        std::cout << "  " << i + 1 << ". " << categoryLabel(cats[i]) << "\n";
    }
    int catChoice = safeReadInt("เลือกประเภท (1-6): ", 1, 6);
    if (catChoice == 0) return;
    std::string category = cats[catChoice - 1];

    std::string expiry;
    int y, m, d;
    while (true) {
        if (!safeReadLine("วันหมดอายุ (รูปแบบ YYYY-MM-DD เช่น 2026-10-15): ", expiry)) return;
        if (parseAndValidateDate(expiry, y, m, d)) break;
        std::cout << "  -> วันที่ผิดรูปแบบหรือไม่มีอยู่จริง (เช่น เดือนต้อง 1-12, ก.พ. ปีอธิกสุรทิน) กรุณากรอกใหม่\n";
    }

    FoodItem item{g_nextId++, name, category, expiry};
    g_items.push_back(item);
    g_shelf.push(item.id); // [STACK] push ของใหม่เข้าชั้นวางหน้าสุด
    g_bst.insert(item);     // [BST] เพิ่มเข้าตารางค้นหา ID

    saveToFile();
    std::cout << "  -> เพิ่ม \"" << name << "\" รหัส [" << item.id << "] สำเร็จ (" << statusText(daysLeft(expiry)) << ")\n";
}

void menuListAll() {
    if (g_items.empty()) {
        std::cout << "  (ยังไม่มีของในตู้เย็น)\n";
        return;
    }

    std::vector<FoodItem> sorted = g_items;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.expiry < b.expiry;
    });

    for (const auto& it : sorted) printItem(it);
}

void menuUrgent() { // [PRIORITY QUEUE]
    if (g_items.empty()) {
        std::cout << "  (ยังไม่มีของในตู้เย็น)\n";
        return;
    }

    ExpiryMinHeap heap = buildExpiryMinHeap();

    std::cout << "\n  --- รายการของด่วนที่สุด (เรียงตามวันหมดอายุด้วย Min-Heap) ---\n";
    for (int n = 0; !heap.empty() && n < 3; ++n) {
        FoodItem it = heap.top();
        heap.pop();
        std::cout << "  * [" << it.id << "] " << it.name << " (" << categoryLabel(it.category)
                  << ") - " << statusText(daysLeft(it.expiry)) << "\n";
        std::cout << "      เมนูแนะนำ: ";
        auto menus = suggestMenusFor(it);
        for (size_t i = 0; i < menus.size(); ++i) std::cout << (i ? ", " : "") << menus[i];
        std::cout << "\n";
    }
}

void menuGreedy() { // [GREEDY]
    if (g_items.empty()) {
        std::cout << "  (ยังไม่มีของในตู้เย็น)\n";
        return;
    }

    ExpiryMinHeap heap = buildExpiryMinHeap();
    std::vector<FoodItem> pool;
    while (!heap.empty() && pool.size() < 6) {
        pool.push_back(heap.top());
        heap.pop();
    }

    auto combos = greedySuggestCombos(pool);
    if (combos.empty()) {
        std::cout << "  (ไม่มีเมนูรวมที่เหมาะสม)\n";
        return;
    }

    std::cout << "\n  --- ผลการวางแผนมื้ออาหารด้วย Greedy Algorithm (เคลียร์ของมากสุด) ---\n";
    for (size_t i = 0; i < combos.size(); ++i) {
        std::cout << "  มื้อที่ " << i + 1 << ": " << combos[i].menuName
                  << " (เคลียร์ได้ " << combos[i].cleared.size() << " ชนิด - ";
        for (size_t j = 0; j < combos[i].cleared.size(); ++j) {
            std::cout << (j ? ", " : "") << combos[i].cleared[j];
        }
        std::cout << ")\n";
    }
}

void menuShelf() { // [STACK]
    if (g_shelf.empty()) {
        std::cout << "  (ชั้นวางว่างเปล่า)\n";
        return;
    }

    std::stack<int> displayStack;
    for (int id : g_shelf.getRawOrder()) {
        displayStack.push(id);
    }

    std::cout << "\n  --- จำลองตำแหน่งของบนชั้นวาง (LIFO: หน้าตู้ -> ก้นตู้) ---\n";
    int depth = 0;
    while (!displayStack.empty()) {
        int id = displayStack.top();
        displayStack.pop();

        const FoodItem* it = g_bst.search(id);
        if (!it) {
            auto vit = std::find_if(g_items.begin(), g_items.end(), [id](auto& f) { return f.id == id; });
            if (vit != g_items.end()) it = &(*vit);
        }
        if (!it) continue;

        std::string tag = (depth == 0) ? "หน้าสุด (ล่าสุด)" : "ชั้นลึกที่ " + std::to_string(depth + 1) + " จากหน้า";
        std::string warn = "";
        if (daysLeft(it->expiry) < 0) {
            warn = "  [!! หมดอายุแล้วและจมอยู่ก้นตู้ !!]";
        } else if (depth >= 2) {
            warn = "  [ระวังถูกบังลืม]";
        }

        std::cout << "  " << tag << ": [" << it->id << "] " << it->name << " ("
                  << categoryLabel(it->category) << ")" << warn << "\n";
        ++depth;
    }
}

void menuSearchBST() { // [BST SEARCH]
    if (g_items.empty()) {
        std::cout << "  (ยังไม่มีของในตู้เย็น)\n";
        return;
    }

    int targetId = safeReadInt("กรอก ID วัตถุดิบที่ต้องการค้นหา: ", 1, g_nextId + 1000);
    if (targetId == 0) return;

    const FoodItem* item = g_bst.search(targetId);
    if (item) {
        std::cout << "  -> [BST Search: พบข้อมูล!]\n";
        printItem(*item);
        std::cout << "      เมนูแนะนำ: ";
        auto menus = suggestMenusFor(*item);
        for (size_t i = 0; i < menus.size(); ++i) std::cout << (i ? ", " : "") << menus[i];
        std::cout << "\n";
    } else {
        std::cout << "  -> [BST Search: ไม่พบข้อมูลรหัส " << targetId << "]\n";
    }
}

void menuDelete() {
    if (g_items.empty()) {
        std::cout << "  (ยังไม่มีของในตู้เย็น)\n";
        return;
    }
    menuListAll();
    int id = safeReadInt("กรอก ID ที่จะลบ (0 = ยกเลิก): ", 0, g_nextId + 1000);
    if (id == 0) {
        std::cout << "  -> ยกเลิกการลบ\n";
        return;
    }

    auto it = std::find_if(g_items.begin(), g_items.end(), [id](auto& f) { return f.id == id; });
    if (it == g_items.end()) {
        std::cout << "  -> ไม่พบ ID นี้ในระบบ\n";
        return;
    }

    std::string removedName = it->name;
    g_items.erase(it);
    g_shelf.removeId(id);
    g_bst.remove(id);

    saveToFile();
    std::cout << "  -> ลบ \"" << removedName << "\" (ID: " << id << ") ออกจากระบบเรียบร้อยแล้ว\n";
}

void seedSampleData() {
    time_t now = time(nullptr);
    char buf[16];
    auto addDays = [&](int d) {
        time_t t = now + d * 86400;
        strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
        return std::string(buf);
    };

    auto add = [&](const std::string& n, const std::string& c, int d) {
        FoodItem it{g_nextId++, n, c, addDays(d)};
        g_items.push_back(it);
        g_shelf.push(it.id);
        g_bst.insert(it);
    };

    add("หมูสับ", "meat", -1);     // หมดอายุแล้ว จมอยู่ก้นตู้ (สาธิตปัญหา LIFO)
    add("นมสด", "dairy", 1);       // ใกล้หมดอายุ
    add("ผักกาดขาว", "vegetable", 2);
    add("ไข่ไก่", "dairy", 5);
    add("ส้ม", "fruit", 7);
}

// ============================================================================
// [Automated Self-Check / Unit Test Suite]
// รันเมื่อส่งอาร์กิวเมนต์ --test จาก Command Line
// ============================================================================
int runSelfCheckTests() {
    std::cout << "=========================================================\n";
    std::cout << "  [SMART FRIDGE] AUTOMATED SELF-CHECK TEST SUITE (--test)\n";
    std::cout << "=========================================================\n";
    int passedCount = 0;
    int totalTests = 6;

    // Test 1: Date Parsing & Leap Year Validation
    {
        int y, m, d;
        assert(parseAndValidateDate("2026-10-15", y, m, d) == true && y == 2026 && m == 10 && d == 15);
        assert(parseAndValidateDate("2024-02-29", y, m, d) == true);
        assert(parseAndValidateDate("2023-02-29", y, m, d) == false);
        assert(parseAndValidateDate("2026-13-01", y, m, d) == false);
        assert(parseAndValidateDate("2026-04-31", y, m, d) == false);
        assert(parseAndValidateDate("invalid-date", y, m, d) == false);
        std::cout << "  [PASS] 1/6 Date Validation & Leap Year Correctness\n";
        ++passedCount;
    }

    // Test 2: Stack ADT (LIFO & Shelf Simulation)
    {
        ShelfStack stack;
        assert(stack.empty() == true);
        stack.push(101);
        stack.push(102);
        stack.push(103);
        assert(stack.size() == 3);

        int poppedId = 0;
        assert(stack.pop(poppedId) == true && poppedId == 103);
        assert(stack.pop(poppedId) == true && poppedId == 102);
        assert(stack.pop(poppedId) == true && poppedId == 101);
        assert(stack.pop(poppedId) == false);
        assert(stack.empty() == true);

        stack.push(201);
        stack.push(202);
        stack.push(203);
        stack.removeId(202);
        assert(stack.size() == 2);
        assert(stack.pop(poppedId) == true && poppedId == 203);
        assert(stack.pop(poppedId) == true && poppedId == 201);
        std::cout << "  [PASS] 2/6 Stack ADT (LIFO & Shelf Operations)\n";
        ++passedCount;
    }

    // Test 3: Priority Queue / Min-Heap (Expiry Priority)
    {
        ExpiryMinHeap heap;
        heap.push(FoodItem{1, "A", "meat", "2026-10-20"});
        heap.push(FoodItem{2, "B", "dairy", "2026-09-01"});
        heap.push(FoodItem{3, "C", "vegetable", "2026-10-05"});

        assert(!heap.empty());
        FoodItem first = heap.top(); heap.pop();
        assert(first.name == "B");

        FoodItem second = heap.top(); heap.pop();
        assert(second.name == "C");

        FoodItem third = heap.top(); heap.pop();
        assert(third.name == "A");
        assert(heap.empty());
        std::cout << "  [PASS] 3/6 Priority Queue (Min-Heap Expiry Ordering)\n";
        ++passedCount;
    }

    // Test 4: Binary Search Tree (BST ADT & Dynamic Memory)
    {
        FoodBST bst;
        assert(bst.empty() == true);

        bst.insert(FoodItem{50, "Pork", "meat", "2026-10-10"});
        bst.insert(FoodItem{30, "Milk", "dairy", "2026-10-12"});
        bst.insert(FoodItem{70, "Fish", "seafood", "2026-10-14"});
        bst.insert(FoodItem{20, "Egg", "dairy", "2026-10-08"});
        bst.insert(FoodItem{40, "Carrot", "vegetable", "2026-10-09"});

        assert(bst.size() == 5);
        assert(bst.search(50) != nullptr && bst.search(50)->name == "Pork");
        assert(bst.search(20) != nullptr && bst.search(20)->name == "Egg");
        assert(bst.search(999) == nullptr);

        assert(bst.remove(30) == true);
        assert(bst.search(30) == nullptr);
        assert(bst.search(20) != nullptr);
        assert(bst.search(40) != nullptr);
        assert(bst.size() == 4);

        std::vector<FoodItem> in = bst.getInorder();
        assert(in.size() == 4);
        assert(in[0].id == 20 && in[1].id == 40 && in[2].id == 50 && in[3].id == 70);

        bst.clear();
        assert(bst.empty() == true);
        std::cout << "  [PASS] 4/6 Binary Search Tree (Insert, Search, Delete, In-Order)\n";
        ++passedCount;
    }

    // Test 5: Greedy Recipe Suggestion Logic
    {
        std::vector<FoodItem> urgentPool = {
            {1, "เนื้อหมู", "meat", "2026-10-01"},
            {2, "กุ้ง", "seafood", "2026-10-01"},
            {3, "กะหล่ำปลี", "vegetable", "2026-10-01"},
            {4, "นม", "dairy", "2026-10-02"},
            {5, "ส้ม", "fruit", "2026-10-03"}
        };
        auto combos = greedySuggestCombos(urgentPool);
        assert(!combos.empty());
        assert(combos[0].cleared.size() >= 3);
        std::cout << "  [PASS] 5/6 Greedy Recipe Recommendation (Max-Coverage)\n";
        ++passedCount;
    }

    // Test 6: String & Input Sanitization
    {
        assert(trim("   hello world   ") == "hello world");
        assert(trim("\t\r\n") == "");
        assert(categoryLabel("meat") == "เนื้อสัตว์");
        assert(categoryLabel("unknown_cat") == "unknown_cat");
        std::cout << "  [PASS] 6/6 String Sanitization & Category Mapping\n";
        ++passedCount;
    }

    std::cout << "=========================================================\n";
    std::cout << "  [SUCCESS] All " << passedCount << "/" << totalTests
              << " Self-Check Tests Passed! (100% Verification)\n";
    std::cout << "=========================================================\n";
    return 0;
}

// ============================================================================
// Main Function
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. ตรวจสอบโหมดทดสอบอัตโนมัติ (--test)
    if (argc > 1 && std::string(argv[1]) == "--test") {
        return runSelfCheckTests();
    }

    // 2. ตั้งค่า UTF-8 Console บน Windows
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 3. โหลดข้อมูลเดิม หรือเพิ่มข้อมูลจำลอง
    if (!loadFromFile()) {
        seedSampleData();
    } else {
        rebuildBST();
    }

    std::cout << "== ระบบช่วยเคลียร์ตู้เย็นและแนะนำเมนูอาหาร (บันทึกลง " << kDataFile << " อัตโนมัติ) ==\n";

    while (true) {
        std::cout << "\n--------------------------------------------------------------------------------\n";
        std::cout << "1.เพิ่มของ  2.ดูทั้งหมด  3.ของใกล้หมดอายุ(Min-Heap)  4.แนะนำเมนู(Greedy)\n"
                  << "5.ชั้นวาง(Stack)  6.ค้นหาด้วย ID(BST)  7.ลบของ  0.ออก\n";
        std::cout << "--------------------------------------------------------------------------------\n";

        int choice = safeReadInt("เลือกเมนู (0-7): ", 0, 7);
        switch (choice) {
            case 1: menuAddItem(); break;
            case 2: menuListAll(); break;
            case 3: menuUrgent(); break;
            case 4: menuGreedy(); break;
            case 5: menuShelf(); break;
            case 6: menuSearchBST(); break;
            case 7: menuDelete(); break;
            case 0: {
                std::cout << "ขอบคุณที่ใช้งานระบบเคลียร์ตู้เย็นครับ\n";
                return 0;
            }
            default:
                break;
        }
    }
}
