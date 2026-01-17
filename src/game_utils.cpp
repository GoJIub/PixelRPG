#include "../include/game_utils.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unordered_set>
#include <algorithm>
#include <sstream>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <optional>
#include <array>
#include <thread>

using namespace std::chrono_literals;
std::mutex print_mutex;
std::mutex global_npcs_mutex;

// ---------------- Константы FileObserver ----------------
const int FileObserver::W1 = 18;
const int FileObserver::W2 = 10;
const int FileObserver::WH = 8;
const int FileObserver::WP = 11;
const int FileObserver::WA = 10;
const int FileObserver::W3 = 18;
const int FileObserver::W4 = 10;

// ---------------- Наблюдатели ----------------
std::shared_ptr<IInteractionObserver> ConsoleObserver::get() {
    static ConsoleObserver instance;
    return std::shared_ptr<IInteractionObserver>(&instance, [](IInteractionObserver*) {});
}

void ConsoleObserver::on_interaction(const std::shared_ptr<NPC>& actor,
                               const std::shared_ptr<NPC>& target,
                               InteractionOutcome outcome)
{
    if (!actor || !target) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    switch (outcome) {
    case InteractionOutcome::TargetKilled:
        std::cout << ">>> "
                  << actor->name << " (" << type_to_string(actor->type) << ")"
                  << " killed "
                  << target->name << " (" << type_to_string(target->type) << ")\n";
        break;

    case InteractionOutcome::TargetHurted:
        std::cout << ">>> "
                  << actor->name << " (" << type_to_string(actor->type) << ")"
                  << " hurted "
                  << target->name << " (" << type_to_string(target->type) << ")\n";
        break;

    case InteractionOutcome::TargetEscaped:
        std::cout << ">>> "
                  << target->name << " (" << type_to_string(target->type) << ")"
                  << " escaped from "
                  << actor->name << " (" << type_to_string(actor->type) << ")\n";
        break;

    case InteractionOutcome::TargetHealed:
        std::cout << ">>> "
                << actor->name << " (" << type_to_string(actor->type) << ")"
                << "healed"
                << target->name << " (" << type_to_string(target->type) << ")\n";
        break;

    case InteractionOutcome::NoInteraction:
        break;
    }
}

FileObserver::FileObserver(const std::string& filename) : fname(filename)
{
    std::ofstream f(fname, std::ios::trunc);
    if (!f.good()) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    f << std::left
      << std::setw(W1)  << "Actor"
      << std::setw(W2)  << "Type"
      << std::setw(WH)  << "Health"
      << std::setw(WP)  << "Pos"
      << std::setw(WA)  << "Action"
      << std::setw(W3)  << "Target"
      << std::setw(W4)  << "Type"
      << std::setw(WH)  << "Health"
      << std::setw(WP)  << "Pos"
      << "\n";

    f << std::string(W1 + W2 + WH + WP + WA + W3 + W4, '-') << "\n";
}

std::shared_ptr<IInteractionObserver> FileObserver::get(const std::string& filename)
{
    static std::shared_ptr<IInteractionObserver> instance =
        std::shared_ptr<IInteractionObserver>(
            new FileObserver(filename),
            [](IInteractionObserver*){}
        );

    return instance;
}

void FileObserver::on_interaction(const std::shared_ptr<NPC>& actor,
                            const std::shared_ptr<NPC>& target,
                            InteractionOutcome outcome)
{
    if (!actor || !target) return;

    std::ofstream f(fname, std::ios::app);
    if (!f.good()) return;

    std::lock_guard<std::mutex> lck(print_mutex);

    std::ostringstream ss;
    ss << '(' << actor->x << ',' << actor->y << ')';
    std::string aPos = ss.str();
    ss.str("");
    ss << '(' << target->x << ',' << target->y << ')';
    std::string tPos = ss.str();

    switch (outcome) {
    case InteractionOutcome::TargetKilled:
        f << std::left
          << std::setw(W1) << actor->name
          << std::setw(W2) << type_to_string(actor->type)
          << std::setw(WH) << actor->health
          << std::setw(WP) << aPos
          << std::setw(WA) << "killed"
          << std::setw(W3) << target->name
          << std::setw(W4) << type_to_string(target->type)
          << std::setw(WH) << target->health
          << std::setw(WP) << tPos
          << "\n";
        break;

    case InteractionOutcome::TargetHurted:
        f << std::left
          << std::setw(W1) << actor->name
          << std::setw(W2) << type_to_string(actor->type)
          << std::setw(WH) << actor->health
          << std::setw(WP) << aPos
          << std::setw(WA) << "hurted"
          << std::setw(W3) << target->name
          << std::setw(W4) << type_to_string(target->type)
          << std::setw(WH) << target->health
          << std::setw(WP) << tPos
          << "\n";
        break;

    case InteractionOutcome::TargetEscaped:
        f << std::left
          << std::setw(W1) << target->name
          << std::setw(W2) << type_to_string(target->type)
          << std::setw(WH) << target->health
          << std::setw(WP) << tPos
          << std::setw(WA) << "escaped"
          << std::setw(W3) << actor->name
          << std::setw(W4) << type_to_string(actor->type)
          << std::setw(WH) << actor->health
          << std::setw(WP) << aPos
          << "\n";
        break;

    case InteractionOutcome::TargetHealed:
        f << std::left
          << std::setw(W1) << actor->name
          << std::setw(W2) << type_to_string(actor->type)
          << std::setw(WH) << actor->health
          << std::setw(WP) << aPos
          << std::setw(WA) << "healed"
          << std::setw(W3) << target->name
          << std::setw(W4) << type_to_string(target->type)
          << std::setw(WH) << target->health
          << std::setw(WP) << tPos
          << "\n";
        break;

    case InteractionOutcome::NoInteraction:
        break;
    }
}

// ---------------- Логика боя ----------------
AttackVisitor::AttackVisitor(const std::shared_ptr<NPC>& actor_)
    : actor(actor_) {}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Bear& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    NPCType at = actor->type;
    if (at == NPCType::Orc || at == NPCType::Dragon)
        return dice() ? InteractionOutcome::TargetHurted : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Dragon& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    NPCType at = actor->type;
    if (at == NPCType::Orc || at == NPCType::Dragon)
        return dice() ? InteractionOutcome::TargetHurted : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Druid& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    NPCType at = actor->type;
    if (at == NPCType::Orc || at == NPCType::Dragon)
        return dice() ? InteractionOutcome::TargetHurted : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Orc& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    NPCType at = actor->type;
    if (at == NPCType::Orc || at == NPCType::Dragon)
        return dice() ? InteractionOutcome::TargetHurted : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome AttackVisitor::visit([[maybe_unused]] Squirrel& target) {
    if (!actor->is_alive()) return InteractionOutcome::NoInteraction;

    NPCType at = actor->type;
    if (at == NPCType::Bear)
        return dice() ? InteractionOutcome::TargetHurted : InteractionOutcome::TargetEscaped;

    return InteractionOutcome::NoInteraction;
}

bool AttackVisitor::dice() {
    return roll() > roll();
}

SupportVisitor::SupportVisitor(const std::shared_ptr<NPC>& actor_)
    : actor(actor_) {}

InteractionOutcome SupportVisitor::visit(Bear& target) {
    if (actor->type == NPCType::Druid && target.is_alive() && target.get_current_health() != target.get_max_health())
        return InteractionOutcome::TargetHealed;

    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Dragon&) {
    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Druid&) {
    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Orc&) {
    return InteractionOutcome::NoInteraction;
}

InteractionOutcome SupportVisitor::visit(Squirrel& target) {
    if (actor->type == NPCType::Druid && target.is_alive() && target.get_current_health() != target.get_max_health())
        return InteractionOutcome::TargetHealed;

    return InteractionOutcome::NoInteraction;
}

InteractionManager& InteractionManager::instance() {
    static InteractionManager inst;
    return inst;
}

void InteractionManager::push(InteractionEvent ev) {
    std::lock_guard<std::mutex> lock(mtx);
    queue.push(std::move(ev));
}

void InteractionManager::apply_outcome(const std::shared_ptr<NPC>& actor,
                   const std::shared_ptr<NPC>& target,
                   InteractionOutcome outcome)
{
    std::lock_guard<std::mutex> lock(global_npcs_mutex);

    switch (outcome) {
    case InteractionOutcome::TargetHurted:
        {
            int damage = actor->get_damage_amount();
            std::lock_guard<std::mutex> lck(target->mtx);
            target->health -= damage;
            if (target->health <= 0) {
                target->health = 0;
                target->alive = false;
                outcome = InteractionOutcome::TargetKilled;
            }
        }
        actor->notify_interaction(target, outcome);
        break;

    case InteractionOutcome::TargetEscaped:
        actor->notify_interaction(target, outcome);
        break;

    case InteractionOutcome::TargetHealed:
        target->heal();
        actor->notify_interaction(target, outcome);
        break;

    case InteractionOutcome::NoInteraction:
        break;
    }

    effects_cv.notify_one();
}

void InteractionManager::operator()() {
    using namespace std::chrono_literals;

    while (running) {
        std::optional<InteractionEvent> ev;

        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!queue.empty()) {
                ev = queue.front();
                queue.pop();
            }
        }

        if (ev) {
            auto a = ev->actor;
            auto t = ev->target;

            if (!a || !t) {
                std::this_thread::sleep_for(1ms);
                continue;
            }

            // Проверяем расстояние БЕЗ разрыва между проверкой и действием
            bool alive_a = a->is_alive();
            bool alive_t = t->is_alive();
            
            // Получаем расстояние thread-safe способом
            int distance = -1;
            if (alive_a && alive_t) {
                distance = a->get_distance_to(t);
            }
            
            int interaction_dist = a->get_interaction_distance();
            
            // Проверка близости с логированием для отладки
            if (alive_a && alive_t && distance >= 0 && distance <= interaction_dist) {
                // Атака
                AttackVisitor av1(a);
                InteractionOutcome outcome1 = t->accept(av1);
                apply_outcome(a, t, outcome1);
                
                // Контратака (если target ещё жив)
                if (t->is_alive()) {
                    AttackVisitor av2(t);
                    InteractionOutcome outcome2 = a->accept(av2);
                    apply_outcome(t, a, outcome2);
                }
            }

            // Поддержка (лечение)
            alive_a = a->is_alive();
            alive_t = t->is_alive();
            
            if ((alive_a && alive_t)) {
                distance = a->get_distance_to(t);
                if (distance >= 0 && distance <= interaction_dist) {
                    SupportVisitor sv1(a);
                    InteractionOutcome outcome = t->accept(sv1);
                    apply_outcome(a, t, outcome);

                    SupportVisitor sv2(t);
                    InteractionOutcome outcome2 = a->accept(sv2);
                    apply_outcome(t, a, outcome2);
                }
            }

            std::this_thread::sleep_for(5ms);
        }
        std::this_thread::sleep_for(5ms);
    }
}

void InteractionManager::stop() {
    running = false;
}

// ---------------- Сохранение/Загрузка ----------------
void save_all(const std::vector<std::shared_ptr<NPC>> &list, const std::string &filename) {
    std::ofstream os(filename, std::ios::trunc);
    os << list.size() << '\n';
    for (auto &p : list) p->save(os);
}

std::vector<std::shared_ptr<NPC>> load_all(const std::string &filename) {
    std::vector<std::shared_ptr<NPC>> res;
    std::ifstream is(filename);
    if (!is.good()) return res;

    size_t cnt = 0;
    is >> cnt;
    for (size_t i = 0; i < cnt; ++i) {
        auto p = createNPCFromStream(is);
        if (p) res.push_back(p);
    }
    return res;
}

void print_all(const std::vector<std::shared_ptr<NPC>> &list) {
    std::cout << "\n=== NPCs (" << list.size() << ") ===\n";
    const int W1 = 18, W2 = 10, WH = 6, W3 = 6, W4 = 6;
    std::cout << std::left
              << std::setw(W1) << "Name"
              << std::setw(W2) << "Type"
              << std::setw(WH) << "Health"
              << std::setw(W3) << "X"
              << std::setw(W4) << "Y"
              << "\n";
    std::cout << std::string(W1 + W2 + WH + W3 + W4, '-') << "\n";
    for (auto &p : list) {
        if (!p) continue;
        std::cout << std::left
                  << std::setw(W1) << p->name
                  << std::setw(W2) << type_to_string(p->type)
                  << std::setw(WH) << p->health
                  << std::setw(W3) << p->x
                  << std::setw(W4) << p->y
                  << "\n";
    }
    std::cout << std::string(40, '=') << std::endl << std::endl;
}

void print_survivors(const std::vector<std::shared_ptr<NPC>>& npcs) {
    std::lock_guard<std::mutex> lck(print_mutex);
    std::cout << "\n=== Survivors ===\n";
    for (auto& npc : npcs)
        if (npc->is_alive()) {
            npc->print(std::cout);
            std::cout << '\n';
        }
}

void draw_map(const std::vector<std::shared_ptr<NPC>>& list) {
    std::array<std::pair<std::string, char>, GRID * GRID> field{};
    field.fill({"", ' '});

    for (auto& npc : list) {
        auto [x, y] = npc->position();

        int gx = std::clamp(x * GRID / MAP_X, 0, GRID - 1);
        int gy = std::clamp(y * GRID / MAP_Y, 0, GRID - 1);

        char c;
        if (!npc->is_alive())
            c = '*';
        else {
            switch (npc->type) {
                case NPCType::Bear:     c = 'B'; break;
                case NPCType::Dragon:   c = 'D'; break;
                case NPCType::Druid:    c = 'D'; break;
                case NPCType::Orc:      c = 'O'; break;
                case NPCType::Squirrel: c = 'S'; break;
                default: c = '?';
            }
        }

        field[gx + gy * GRID] = {npc->get_color(npc->type), c};
    }

    std::lock_guard<std::mutex> lck(print_mutex);

    std::cout << std::string(3 * GRID, '=') << "\n";
    for (int y = 0; y < GRID; ++y) {
        for (int x = 0; x < GRID; ++x) {
            auto [color, ch] = field[x + y * GRID];
            std::string reset = "\033[0m";
            std::cout << "[" << color << ch << reset << "]";
        }
        std::cout << '\n';
    }
    std::cout << std::string(3 * GRID, '=') << "\n\n";
}

// ---------------- Функции рандома ----------------
NPCType random_type() {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, static_cast<int>(NPCType::Count) - 1);
    return static_cast<NPCType>(dist(gen));
}

int random_coord(int min, int max) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

std::mt19937& rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

int roll() {
    static std::uniform_int_distribution<int> d(1, 6);
    return d(rng());
}