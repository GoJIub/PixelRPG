#pragma once
#include "npc.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>

// Типы визуальных эффектов
enum class EffectType {
    Kill,        // Взрыв частиц
    Hurt,        // Жёлтая вспышка
    Escape,      // Зелёные следы
    Heal         // Голубое сияние
};

// Структура для одного эффекта
struct VisualEffect {
    EffectType type;
    float x, y;
    std::chrono::steady_clock::time_point start_time;
    float duration_ms;
    sf::Color color;
    
    VisualEffect(EffectType t, float x_, float y_, float dur_ms, sf::Color c)
        : type(t), x(x_), y(y_), duration_ms(dur_ms), color(c) {
        start_time = std::chrono::steady_clock::now();
    }
    
    // Проверить, закончился ли эффект
    bool isExpired() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        return elapsed >= duration_ms;
    }
    
    // Получить прогресс анимации (0.0 - 1.0)
    float getProgress() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
        return std::min(1.0f, static_cast<float>(elapsed) / duration_ms);
    }
};

// Структура для частицы
struct Particle {
    float x, y;
    float vx, vy;  // Скорость
    sf::Color color;
    float lifetime;
    float max_lifetime;
    
    Particle(float x_, float y_, float vx_, float vy_, sf::Color c, float life)
        : x(x_), y(y_), vx(vx_), vy(vy_), color(c), lifetime(life), max_lifetime(life) {}
    
    void update(float dt) {
        x += vx * dt;
        y += vy * dt;
        lifetime -= dt;
    }
    
    bool isAlive() const {
        return lifetime > 0;
    }
    
    float getAlpha() const {
        return lifetime / max_lifetime;
    }
};

class VisualObserver : public IInteractionObserver {
private:
    VisualObserver() = default;
    
    std::string lastInteractionMessage;
    mutable std::mutex message_mutex;
    
    // Список активных эффектов
    std::deque<VisualEffect> active_effects;
    std::deque<Particle> particles;
    mutable std::mutex effects_mutex;
    
public:
    static std::shared_ptr<IInteractionObserver> get();
    
    ~VisualObserver() = default;
    
    void on_interaction(const std::shared_ptr<NPC>& actor,
                       const std::shared_ptr<NPC>& target,
                       InteractionOutcome outcome) override;
    
    std::string getLastInteractionMessage() const;
    
    // Добавить визуальный эффект
    void addEffect(EffectType type, float x, float y, float duration_ms, sf::Color color);
    
    // Добавить частицы
    void addParticles(float x, float y, int count, sf::Color color);
    
    // Получить все активные эффекты
    std::deque<VisualEffect> getActiveEffects();
    
    // Получить все активные частицы
    std::deque<Particle> getActiveParticles();
    
    // Обновить частицы
    void updateParticles(float dt);
};

class VisualWrapper {
private:
    sf::RenderWindow window;
    sf::Font font;
    sf::Clock clock;
    sf::Clock frameClock;  // Для delta time
    
    sf::Texture bearTexture;
    sf::Texture dragonTexture;
    sf::Texture druidTexture;
    sf::Texture orcTexture;
    sf::Texture squirrelTexture;
    sf::Texture backgroundTexture;
    
    sf::Text interactionText;
    sf::RectangleShape interactionBox;
    
    // Счётчик статистики
    sf::Text statsText;
    sf::RectangleShape statsBox;
    
    std::vector<std::shared_ptr<NPC>>* npcs;
    mutable std::mutex npcsMutex;
    
    std::string lastInteractionMessage;
    sf::Time messageDisplayTime;

    std::atomic<bool>* paused;  // Pointer to external paused
    std::mutex* cv_mtx_ptr = nullptr;  // Указатель на cv_mtx из InteractionManager
    std::condition_variable* effects_cv_ptr = nullptr;  // Указатель на effects_cv
    std::atomic<bool>* running_ptr = nullptr;
    
    void createPixelArtTextures();
    sf::Color getColorForNPC(NPCType type) const;
    
    // Рендеринг эффектов
    void renderEffects(const std::deque<VisualEffect>& effects, float scaleX, float scaleY);
    void renderParticles(const std::deque<Particle>& particles, float scaleX, float scaleY);
    
    // Отрисовка конкретного эффекта
    void drawKillEffect(float x, float y, float progress);
    void drawHurtEffect(float x, float y, float progress);
    void drawEscapeEffect(float x, float y, float progress);
    void drawHealEffect(float x, float y, float progress);
    
    // Отрисовка полоски здоровья
    void drawHealthBar(float screen_x, float screen_y, int hp, int maxHp);

public:
    VisualWrapper(int width = 800, int height = 600);
    ~VisualWrapper() = default;
    
    bool initialize();
    void setNPCs(std::vector<std::shared_ptr<NPC>>& npcs_list);
    void setInteractionMessage(const std::string& message);
    void setEffectsCVPtr(std::condition_variable* cv, std::mutex* mtx);
    void run();
    void setPausedPtr(std::atomic<bool>* p);
    void setRunningPtr(std::atomic<bool>* r);
    void handleEvents();
    void render();
    bool isWindowOpen() const;
};