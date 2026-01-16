#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <atomic>
#include <condition_variable>
#include <random>
#include "npc.h"
#include "bear.h"
#include "dragon.h"
#include "druid.h"
#include "orc.h"
#include "squirrel.h"

// Оптимизированные параметры для частых взаимодействий
constexpr int MAP_X = 50;   // Уменьшено со 100 - теперь NPC ближе друг к другу
constexpr int MAP_Y = 50;   // Уменьшено со 100
constexpr int GRID = 20;
constexpr int CELL_SIZE = 5;  // Для 10x10 grid на 50x50 карте

// Custom hash для std::pair<int, int>
struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);  // Улучшенный hash для избежания коллизий
    }
};

// ---------------- Наблюдатели ----------------
class ConsoleObserver : public IInteractionObserver {
private:
    ConsoleObserver() {};

public:
    static std::shared_ptr<IInteractionObserver> get();
    void on_interaction(const std::shared_ptr<NPC> &actor,
                  const std::shared_ptr<NPC> &target,
                  InteractionOutcome outcome) override;
};

class FileObserver : public IInteractionObserver {
private:
    explicit FileObserver(const std::string& filename);
    std::string fname;
    static const int W1, W2, WH, WP, WA, W3, W4;

public:
    static std::shared_ptr<IInteractionObserver> get(const std::string& filename);
    void on_interaction(const std::shared_ptr<NPC> &actor,
                  const std::shared_ptr<NPC> &target,
                  InteractionOutcome outcome) override;
};

// ---------------- Логика боя ----------------
struct AttackVisitor : public IInteractionVisitor {
    explicit AttackVisitor(const std::shared_ptr<NPC> &actor_);
    InteractionOutcome visit([[maybe_unused]] Bear& target) override;
    InteractionOutcome visit([[maybe_unused]] Dragon& target) override;
    InteractionOutcome visit([[maybe_unused]] Druid& target) override;
    InteractionOutcome visit([[maybe_unused]] Orc& target) override;
    InteractionOutcome visit([[maybe_unused]] Squirrel& target) override;
private:
    std::shared_ptr<NPC> actor;
    bool dice();
};

struct SupportVisitor : public IInteractionVisitor {
    explicit SupportVisitor(const std::shared_ptr<NPC>& actor_);

    InteractionOutcome visit(Bear&) override;
    InteractionOutcome visit(Dragon&) override;
    InteractionOutcome visit(Druid&) override;
    InteractionOutcome visit(Orc&) override;
    InteractionOutcome visit(Squirrel&) override;

private:
    std::shared_ptr<NPC> actor;
};

struct InteractionEvent {
    std::shared_ptr<NPC> actor;
    std::shared_ptr<NPC> target;
};

class InteractionManager {
public:
    static InteractionManager& instance();

    void push(InteractionEvent ev);
    void apply_outcome(const std::shared_ptr<NPC>& actor,
                   const std::shared_ptr<NPC>& target,
                   InteractionOutcome outcome);
    void operator()();
    void stop();
    
    std::mutex* getCVMtx() { return &cv_mtx; }
    std::condition_variable* getEffectsCV() { return &effects_cv; }

private:
    InteractionManager() = default;
    std::queue<InteractionEvent> queue;
    std::mutex mtx;
    std::atomic<bool> running{true};
    std::condition_variable effects_cv;
    std::mutex cv_mtx;  // Мьютекс для condition_variable (отдельный от global_npcs_mutex)
};

// ---------------- Вспомогательные функции ----------------
void save_all(const std::vector<std::shared_ptr<NPC>> &list, const std::string &filename);
std::vector<std::shared_ptr<NPC>> load_all(const std::string &filename);
void print_all(const std::vector<std::shared_ptr<NPC>> &list);
void print_survivors(const std::vector<std::shared_ptr<NPC>>& npcs);
void draw_map(const std::vector<std::shared_ptr<NPC>>& list);
NPCType random_type();
int random_coord(int min, int max);
std::mt19937& rng();
int roll();