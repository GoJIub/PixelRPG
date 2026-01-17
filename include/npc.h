#pragma once
#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <shared_mutex>
#include <chrono>

struct NPC;

struct Bear;
struct Dragon;
struct Druid;
struct Orc;
struct Squirrel;

enum class NPCType {
    Unknown = 0,
    Bear = 1,
    Dragon = 2,
    Druid = 3,
    Orc = 4,
    Squirrel = 5,
    Count
};

enum class InteractionOutcome {
    TargetKilled,
    TargetHurted,
    TargetEscaped,
    TargetHealed,
    NoInteraction
};

struct IInteractionVisitor {
    virtual InteractionOutcome visit(Bear &target) = 0;
    virtual InteractionOutcome visit(Dragon &target) = 0;
    virtual InteractionOutcome visit(Druid &target) = 0;
    virtual InteractionOutcome visit(Orc &target) = 0;
    virtual InteractionOutcome visit(Squirrel &target) = 0;
    virtual ~IInteractionVisitor() = default;
};

struct IInteractionObserver {
    virtual void on_interaction(const std::shared_ptr<NPC> &actor,
                          const std::shared_ptr<NPC> &target,
                          InteractionOutcome outcome) = 0;
    virtual ~IInteractionObserver() = default;
};

struct NPC : public std::enable_shared_from_this<NPC> {
    mutable std::mutex mtx;
    NPCType type{NPCType::Unknown};
    std::string name;
    int x{0};
    int y{0};
    int health{100};
    bool alive{true};
    std::vector<std::shared_ptr<IInteractionObserver>> observers;
    
    // Для плавного движения
    int prev_x{0};
    int prev_y{0};
    std::chrono::steady_clock::time_point last_move_time;
    
    std::pair<int, int> grid_cell{0, 0};

    NPC() = default;
    NPC(NPCType t, std::string_view nm, int x_, int y_);
    virtual ~NPC() = default;

    virtual InteractionOutcome accept(IInteractionVisitor &visitor) = 0;

    void subscribe(const std::shared_ptr<IInteractionObserver> &obs);
    void notify_interaction(const std::shared_ptr<NPC> &target, InteractionOutcome outcome);

    virtual void save(std::ostream &os) const;
    virtual void print(std::ostream &os) const;

    bool is_close(const std::shared_ptr<NPC> &other, int distance) const;
    int get_distance_to(const std::shared_ptr<NPC> &other) const;
    void move(int shift_x, int shift_y, int max_x, int max_y);

    bool is_alive() const;
    void must_die();
    void heal();
    std::pair<int,int> position() const;
    
    // Новый метод для получения интерполированной позиции
    std::pair<float, float> get_visual_position(float interpolation_time_ms = 500.0f) const;
    
    std::string get_color(NPCType t) const;
    int get_move_distance() const;
    int get_interaction_distance() const;
    int get_max_health() const;
    int get_damage_amount() const;
    bool get_state(int& x_, int& y_) const;
    int get_current_health() const;
};

std::string type_to_string(NPCType t);

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y);
std::shared_ptr<NPC> createNPCFromStream(std::istream &is);