/**
 * ============================================================================
 * Project Name: Classroom & Lab Slot Booking System
 * Core Component: OOP (Abstract Room + Derived Types) & DSA (BST + Sorted Lists)
 * Build Command: g++ -std=c++17 -Wall -Wextra -o booking booking.cpp
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <iomanip>

using namespace std;

// --- Global Constants & Configurations ---
constexpr int DAYS = 6;     // Monday to Saturday
constexpr int PERIODS = 10; // 10 daily time periods

static const char *DAY_NAME[DAYS + 1] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *PERIOD_TIME[PERIODS + 1] = {
    "", "08:00", "08:55", "10:10", "11:05", "12:00",
    "12:55", "14:10", "15:05", "16:00", "16:55"
};

// Purpose categories for bookings
enum class Purpose { Lecture = 1, Tutorial, Practical, Project };

static const char *purposeName(Purpose p) {
    static const char *names[] = {"", "Lecture", "Tutorial", "Practical", "Project"};
    return names[static_cast<int>(p)];
}

// ============================================================================
// Booking Node: Represents an individual reservation in a room's daily list
// ============================================================================
struct Booking {
    int id = 0;
    string user;
    Purpose purpose = Purpose::Lecture;
    int strength = 0;
    int day = 0;
    int from = 0;
    int to = 0;
    Booking *next = nullptr;
};

// ============================================================================
// Abstract Base Class: Room
// ============================================================================
class Room {
protected:
    string id_;
    int capacity_;
    Booking *slots_[DAYS + 1] = {}; // One sorted linked list per day

public:
    Room(string id, int cap) : id_(move(id)), capacity_(cap) {}
    
    virtual ~Room() {
        // Clean up all dynamic booking nodes to prevent memory leaks
        for (int d = 1; d <= DAYS; d++) {
            Booking *current = slots_[d];
            while (current != nullptr) {
                Booking *nextNode = current->next;
                delete current;
                current = nextNode;
            }
        }
    }

    virtual string typeName() const = 0;         // Pure virtual: makes Room abstract
    virtual bool allows(Purpose p) const = 0;    // Polymorphic rules per room type

    void describe() const {
        cout << left << setw(8) << id_ << setw(17) << typeName() << "Capacity: " << capacity_ << "\n";
    }

    const string &id() const { return id_; }
    int capacity() const { return capacity_; }
    Booking *daySlots(int d) const { return slots_[d]; }

    // Check if a desired time slot clashes with any existing booking
    Booking *conflict(int day, int from, int to) const {
        for (Booking *b = slots_[day]; b && b->from <= to; b = b->next) {
            if (from <= b->to) return b; // Overlap found
        }
        return nullptr;
    }

    // Insert a new booking into the per-day sorted linked list
    void insert(Booking *b) {
        Booking **pp = &slots_[b->day];
        while (*pp && (*pp)->from < b->from) {
            pp = &(*pp)->next;
        }
        b->next = *pp;
        *pp = b;
    }
};

// ============================================================================
// Derived Room Classes
// ============================================================================
class LectureTheatre : public Room {
public:
    LectureTheatre(string id, int cap) : Room(move(id), cap) {}
    string typeName() const override { return "Lecture Theatre"; }
    bool allows(Purpose p) const override { return p == Purpose::Lecture; }
};

class Classroom : public Room {
public:
    Classroom(string id, int cap) : Room(move(id), cap) {}
    string typeName() const override { return "Classroom"; }
    bool allows(Purpose p) const override { return p != Purpose::Practical; }
};

class Lab : public Room {
public:
    Lab(string id, int cap) : Room(move(id), cap) {}
    string typeName() const override { return "Lab"; }
    bool allows(Purpose p) const override { return p == Purpose::Practical || p == Purpose::Project; }
};

// ============================================================================
// Binary Search Tree (BST) for Managing Rooms Efficiently
// ============================================================================
class RoomTree {
    struct Node {
        Room *room;
        Node *left = nullptr;
        Node *right = nullptr;
        explicit Node(Room *r) : room(r) {}
    };

    Node *root_ = nullptr;

    static void destroyTree(Node *n) {
        if (!n) return;
        destroyTree(n->left);
        destroyTree(n->right);
        delete n->room;
        delete n;
    }

    template <class Fn>
    static void inOrderWalk(Node *n, Fn &fn) {
        if (!n) return;
        inOrderWalk(n->left, fn);
        fn(*(n->room));
        inOrderWalk(n->right, fn);
    }

public:
    ~RoomTree() { destroyTree(root_); }

    Room *find(const string &id) const {
        Node *current = root_;
        while (current) {
            if (id == current->room->id()) return current->room;
            current = (id < current->room->id()) ? current->left : current->right;
        }
        return nullptr;
    }

    void insert(Room *room) {
        Node **pp = &root_;
        while (*pp) {
            if (room->id() == (*pp)->room->id()) {
                delete room; // Duplicate check
                return;
            }
            pp = (room->id() < (*pp)->room->id()) ? &((*pp)->left) : &((*pp)->right);
        }
        *pp = new Node(room);
    }

    template <class Fn>
    void inorder(Fn fn) const {
        inOrderWalk(root_, fn);
    }
};

// ============================================================================
// Central Booking System Controller
// ============================================================================
class BookingSystem {
    RoomTree rooms_;
    int nextBookingId_ = 1001;

public:
    BookingSystem() {
        // Pre-loading sample campus rooms into our BST
        rooms_.insert(new LectureTheatre("124", 120));
        rooms_.insert(new LectureTheatre("224", 120));
        rooms_.insert(new LectureTheatre("206", 100));
        rooms_.insert(new Classroom("117", 60));
        rooms_.insert(new Classroom("114", 60));
        rooms_.insert(new Lab("009", 40));
        rooms_.insert(new Lab("209", 40));
    }

    void book(const string &roomId, const string &user, int purposeCode, int strength, int day, int from, int to) {
        Room *room = rooms_.find(roomId);
        if (!room) {
            cout << "  [Error] Room " << roomId << " does not exist.\n";
            return;
        }

        if (day < 1 || day > DAYS || from < 1 || to > PERIODS || from > to || purposeCode < 1 || purposeCode > 4) {
            cout << "  [Error] Invalid day, period range, or purpose specification.\n";
            return;
        }

        Purpose purpose = static_cast<Purpose>(purposeCode);

        // Polymorphic validation check
        if (!room->allows(purpose)) {
            cout << "  [Error] " << room->typeName() << " is not allowed to host a " << purposeName(purpose) << ".\n";
            return;
        }

        if (strength > room->capacity()) {
            cout << "  [Error] Group strength (" << strength << ") exceeds room capacity (" << room->capacity() << ").\n";
            return;
        }

        // Check for scheduling conflicts
        if (Booking *clash = room->conflict(day, from, to)) {
            cout << "  [Error] Time clash with existing Booking #" << clash->id 
                 << " (" << clash->user << ", Periods " << clash->from << "-" << clash->to << ").\n";
            return;
        }

        // Create and register the new booking
        Booking *newBooking = new Booking{nextBookingId_++, user, purpose, strength, day, from, to, nullptr};
        room->insert(newBooking);

        cout << "  [Success] Booking confirmed! ID: #" << newBooking->id 
             << " | Room: " << room->id() 
             << " | " << DAY_NAME[day] << " (Periods " << from << "-" << to << " -> " 
             << PERIOD_TIME[from] << " to " << PERIOD_TIME[to] << ") "
             << "for " << user << "\n";
    }

    void showDay(const string &roomId, int day) const {
        Room *room = rooms_.find(roomId);
        if (!room) {
            cout << "  [Error] Room " << roomId << " does not exist.\n";
            return;
        }
        if (day < 1 || day > DAYS) {
            cout << "  [Error] Invalid day selected.\n";
            return;
        }

        bool usedPeriods[PERIODS + 2] = {};
        
        cout << "\n----------------------------------------\n";
        cout << " Schedule for Room " << room->id() << " on " << DAY_NAME[day] << ":\n";
        cout << "----------------------------------------\n";

        Booking *current = room->daySlots(day);
        if (!current) {
            cout << "  (No bookings scheduled for this day)\n";
        }

        for (; current; current = current->next) {
            cout << "  * P" << current->from << "-" << current->to 
                 << " (" << PERIOD_TIME[current->from] << ") "
                 << " | Booked ID: #" << current->id
                 << " | " << left << setw(10) << purposeName(current->purpose) 
                 << " | User: " << current->user 
                 << " (" << current->strength << " students)\n";

            for (int i = current->from; i <= current->to; i++) {
                usedPeriods[i] = true;
            }
        }

        cout << "\n  Free periods available:";
        bool hasFree = false;
        for (int i = 1; i <= PERIODS; i++) {
            if (!usedPeriods[i]) {
                cout << " " << i;
                hasFree = true;
            }
        }
        if (!hasFree) cout << " None";
        cout << "\n----------------------------------------\n";
    }

    void listRooms() const {
        cout << "\n========================================\n";
        cout << "         AVAILABLE CAMPUS ROOMS         \n";
        cout << "========================================\n";
        rooms_.inorder([](Room &r) {
            r.describe();
        });
        cout << "========================================\n";
    }
};

// ============================================================================
// Helper Input Functions
// ============================================================================
static bool readInt(const string &prompt, int &value) {
    cout << prompt;
    return static_cast<bool>(cin >> value);
}

static bool readStr(const string &prompt, string &str) {
    cout << prompt;
    return static_cast<bool>(cin >> str);
}

// ============================================================================
// Main Application Loop
// ============================================================================
int main() {
    BookingSystem bookingSystem;

    cout << "==================================================\n";
    cout << "       CLASSROOM & LAB SLOT BOOKING SYSTEM        \n";
    cout << "==================================================\n";
    cout << " Quick Guide:\n";
    cout << " - Days    : 1 = Mon, 2 = Tue, 3 = Wed, 4 = Thu, 5 = Fri, 6 = Sat\n";
    cout << " - Periods : 1 to 10 (08:00 AM to 05:50 PM)\n";
    cout << " - Purposes: 1 = Lecture, 2 = Tutorial, 3 = Practical, 4 = Project\n";
    cout << "==================================================\n";

    while (true) {
        cout << "\nMain Menu:\n";
        cout << "  [1] Book a slot\n";
        cout << "  [2] View room schedule for a day\n";
        cout << "  [3] List all rooms\n";
        cout << "  [0] Exit application\n";
        cout << "Select an option: ";

        int choice;
        if (!(cin >> choice) || choice == 0) {
            cout << "\nExiting application. Have a great day!\n";
            break;
        }

        string roomId, user;
        int purpose, strength, day, fromPeriod, toPeriod;
        bool inputSuccess = true;

        switch (choice) {
        case 1:
            inputSuccess = readStr("Enter Room ID: ", roomId) &&
                           readStr("Your Name / ID (no spaces): ", user) &&
                           readInt("Purpose (1-4): ", purpose) &&
                           readInt("Group Strength: ", strength) &&
                           readInt("Day (1-6): ", day) &&
                           readInt("From Period (1-10): ", fromPeriod) &&
                           readInt("To Period (1-10): ", toPeriod);
            
            if (inputSuccess) {
                bookingSystem.book(roomId, user, purpose, strength, day, fromPeriod, toPeriod);
            }
            break;

        case 2:
            inputSuccess = readStr("Enter Room ID: ", roomId) &&
                           readInt("Enter Day (1-6): ", day);
            
            if (inputSuccess) {
                bookingSystem.showDay(roomId, day);
            }
            break;

        case 3:
            bookingSystem.listRooms();
            break;

        default:
            cout << "  [Error] Invalid menu choice. Please try again.\n";
        }

        if (!inputSuccess) {
            cout << "\n[Notice] Input stream closed or interrupted. Shutting down cleanly.\n";
            break;
        }
    }

    return 0;
}