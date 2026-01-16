#include <cmath>
#include <sstream>
#include <stdexcept>
#include <random>
#include "../include/npc.h"
#include "../include/bear.h"
#include "../include/dragon.h"
#include "../include/druid.h"
#include "../include/orc.h"
#include "../include/squirrel.h"

NPC::NPC(NPCType t, std::string_view nm, int x_, int y_)
    : type(t), name(nm), x(x_), y(y_), prev_x(x_), prev_y(y_), grid_cell(x_ / 5, y_ / 5)
{
    health = get_max_health();
    last_move_time = std::chrono::steady_clock::now();
}

void NPC::subscribe(const std::shared_ptr<IInteractionObserver> &obs) {
    if (!obs) return;
    observers.push_back(obs);
}

void NPC::notify_interaction(const std::shared_ptr<NPC>& other,
                       InteractionOutcome outcome)
{

    for (auto &o : observers)
        o->on_interaction(std::shared_ptr<NPC>(this, [](NPC *) {}), other, outcome);
}

void NPC::save(std::ostream &os) const {
    os << static_cast<int>(type) << ' ' << name << ' ' << x << ' ' << y << '\n';
}

std::string type_to_string(NPCType t) {
    switch (t) {
        case NPCType::Bear:     return "Bear";
        case NPCType::Dragon:   return "Dragon";
        case NPCType::Druid:    return "Druid";
        case NPCType::Orc:      return "Orc";
        case NPCType::Squirrel: return "Squirrel";
        default:                return "Unknown";
    }
}

void NPC::print(std::ostream &os) const {
    os << name << " [" << type_to_string(type) << "] at (" << x << "," << y << ")";
}

// ИСПРАВЛЕНО: Добавлена защита mutex
bool NPC::is_close(const std::shared_ptr<NPC> &other, int distance) const {
    // Блокировка обоих NPC для атомарного чтения координат
    std::lock_guard<std::mutex> lck1(mtx);
    std::lock_guard<std::mutex> lck2(other->mtx);
    
    int dx = x - other->x;
    int dy = y - other->y;
    
    return (dx * dx + dy * dy) <= (distance * distance);
}

void NPC::move(int shift_x, int shift_y, int max_x, int max_y) {
    std::lock_guard<std::mutex> lck(mtx);
    
    // Сохранить предыдущую позицию для интерполяции
    prev_x = x;
    prev_y = y;
    last_move_time = std::chrono::steady_clock::now();

    if ((x + shift_x >= 0) && (x + shift_x <= max_x))
        x += shift_x;
    if ((y + shift_y >= 0) && (y + shift_y <= max_y))
        y += shift_y;
    grid_cell = {x / 5, y / 5};
}

std::pair<float, float> NPC::get_visual_position(float interpolation_time_ms) const {
    std::lock_guard<std::mutex> lck(mtx);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_move_time).count();
    
    float t = std::min(1.0f, static_cast<float>(elapsed) / interpolation_time_ms);
    
    // Ease-out quartic для более плавного движения
    t = 1.0f - std::pow(1.0f - t, 4.0f);
    
    float visual_x = prev_x + (x - prev_x) * t;
    float visual_y = prev_y + (y - prev_y) * t;
    
    return {visual_x, visual_y};
}

bool NPC::is_alive() const {
    std::lock_guard<std::mutex> lck(mtx);
    return alive;
}

void NPC::must_die() {
    std::lock_guard<std::mutex> lck(mtx);
    alive = false;
}

void NPC::heal() {
    std::lock_guard<std::mutex> lck(mtx);
    health = get_max_health();
}

std::pair<int,int> NPC::position() const {
    std::lock_guard<std::mutex> lck(mtx);
    return {x, y};
}

std::string NPC::get_color(NPCType t) const {
    std::lock_guard<std::mutex> lck(mtx);
    switch (t) {
        case NPCType::Bear:     return "\033[33m";
        case NPCType::Dragon:   return "\033[0;33m";
        case NPCType::Druid:    return "\033[36m";
        case NPCType::Orc:      return "\033[31m";
        case NPCType::Squirrel: return "\033[32m";
        default:                return "\033[35m";
    }
}

// Увеличенные дистанции движения для меньшей карты
int NPC::get_move_distance() const {
    switch (type) {
        case NPCType::Bear:     return 2;
        case NPCType::Dragon:   return 12;
        case NPCType::Druid:    return 4;
        case NPCType::Orc:      return 8;
        case NPCType::Squirrel: return 2;
        default:                return 0;
    }
}

// Увеличенные дистанции взаимодействия для более частых контактов
int NPC::get_interaction_distance() const {
    switch (type) {
        case NPCType::Bear:     return 12;
        case NPCType::Dragon:   return 20;
        case NPCType::Druid:    return 15;
        case NPCType::Orc:      return 15;
        case NPCType::Squirrel: return 8;
        default:                return 0;
    }
}

int NPC::get_max_health() const {
    switch (type) {
        case NPCType::Bear:     return 150;
        case NPCType::Dragon:   return 300;
        case NPCType::Druid:    return 100;
        case NPCType::Orc:      return 120;
        case NPCType::Squirrel: return 50;
        default:                return 100;
    }
}

int NPC::get_damage_amount() const {
    switch (type) {
        case NPCType::Bear:     return 25;
        case NPCType::Dragon:   return 80;
        case NPCType::Druid:    return 0;
        case NPCType::Orc:      return 70;
        case NPCType::Squirrel: return 0;
        default:                return 5;
    }
}

bool NPC::get_state(int& x_, int& y_) const {
    std::lock_guard<std::mutex> lck(mtx);
    if (!alive) return false;
    x_ = x;
    y_ = y;
    return true;
}

// НОВЫЙ МЕТОД: Получить расстояние до другого NPC
int NPC::get_distance_to(const std::shared_ptr<NPC> &other) const {
    std::lock_guard<std::mutex> lck1(mtx);
    std::lock_guard<std::mutex> lck2(other->mtx);
    
    int dx = x - other->x;
    int dy = y - other->y;
    
    return static_cast<int>(std::sqrt(dx * dx + dy * dy));
}

int NPC::get_current_health() const {
    std::lock_guard<std::mutex> lck(mtx);
    return health;
}

std::shared_ptr<NPC> createNPC(NPCType type, const std::string &name, int x, int y) {
    switch (type) {
        case NPCType::Bear:     return std::make_shared<Bear>(name, x, y);
        case NPCType::Dragon:   return std::make_shared<Dragon>(name, x, y);
        case NPCType::Druid:    return std::make_shared<Druid>(name, x, y);
        case NPCType::Orc:      return std::make_shared<Orc>(name, x, y);
        case NPCType::Squirrel: return std::make_shared<Squirrel>(name, x, y);
        default: return nullptr;
    }
}

std::shared_ptr<NPC> createNPCFromStream(std::istream &is) {
    int t;
    std::string name;
    int x, y;
    if (!(is >> t >> name >> x >> y)) return nullptr;
    return createNPC(static_cast<NPCType>(t), name, x, y);
}