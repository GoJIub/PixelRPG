#include "../include/visual_wrapper.h"
#include "../include/game_utils.h"
#include <iostream>
#include <cmath>
#include <mutex>
#include <random>

// Singleton implementation
std::shared_ptr<IInteractionObserver> VisualObserver::get() {
    static VisualObserver instance;
    return std::shared_ptr<IInteractionObserver>(&instance, [](IInteractionObserver*) {});
}

void VisualObserver::on_interaction(const std::shared_ptr<NPC>& actor,
                                  const std::shared_ptr<NPC>& target,
                                  InteractionOutcome outcome) {
    if (!actor || !target) return;
    
    std::lock_guard<std::mutex> lck(message_mutex);
    
    // Получить интерполированные позиции для эффектов
    auto [actor_x, actor_y] = actor->get_visual_position(300.0f);  // Было 500.0f
    auto [target_x, target_y] = target->get_visual_position(300.0f);
    
    switch (outcome) {
        case InteractionOutcome::TargetKilled:
            lastInteractionMessage = actor->name + " (" + type_to_string(actor->type) + ") killed " 
                                   + target->name + " (" + type_to_string(target->type) + ")";
            std::cout << ">>> " << lastInteractionMessage << std::endl;
            
            // Визуальные эффекты
            addEffect(EffectType::Attack, target_x, target_y, 300.0f, sf::Color::Red);
            addEffect(EffectType::Kill, target_x, target_y, 600.0f, sf::Color(255, 100, 0));
            addParticles(target_x, target_y, 15, sf::Color::Red);
            break;
            
        case InteractionOutcome::TargetEscaped:
            lastInteractionMessage = target->name + " (" + type_to_string(target->type) + ") escaped from " 
                                   + actor->name + " (" + type_to_string(actor->type) + ")";
            std::cout << ">>> " << lastInteractionMessage << std::endl;
            
            // Зелёные следы побега
            addEffect(EffectType::Escape, target_x, target_y, 500.0f, sf::Color::Green);
            addParticles(target_x, target_y, 8, sf::Color(100, 255, 100));
            break;
            
        case InteractionOutcome::TargetHealed:
            lastInteractionMessage = actor->name + " (Druid) healed " 
                                   + target->name + " (" + type_to_string(target->type) + ")";
            std::cout << ">>> " << lastInteractionMessage << std::endl;
            
            // Голубое сияние исцеления
            addEffect(EffectType::Heal, target_x, target_y, 800.0f, sf::Color::Cyan);
            addParticles(target_x, target_y, 12, sf::Color(100, 200, 255));
            break;
            
        case InteractionOutcome::NoInteraction:
            break;
    }
}

std::string VisualObserver::getLastInteractionMessage() const {
    std::lock_guard<std::mutex> lck(message_mutex);
    return lastInteractionMessage;
}

void VisualObserver::addEffect(EffectType type, float x, float y, float duration_ms, sf::Color color) {
    std::lock_guard<std::mutex> lck(effects_mutex);
    active_effects.emplace_back(type, x, y, duration_ms, color);
}

void VisualObserver::addParticles(float x, float y, int count, sf::Color color) {
    std::lock_guard<std::mutex> lck(effects_mutex);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> speed_dist(20.0f, 60.0f);
    
    for (int i = 0; i < count; ++i) {
        float angle = angle_dist(gen);
        float speed = speed_dist(gen);
        float vx = std::cos(angle) * speed;
        float vy = std::sin(angle) * speed;
        
        particles.emplace_back(x, y, vx, vy, color, 0.5f);  // 0.5 секунды жизни
    }
}

std::deque<VisualEffect> VisualObserver::getActiveEffects() {
    std::lock_guard<std::mutex> lck(effects_mutex);
    
    // Удалить истёкшие эффекты
    active_effects.erase(
        std::remove_if(active_effects.begin(), active_effects.end(),
                      [](const VisualEffect& e) { return e.isExpired(); }),
        active_effects.end()
    );
    
    return active_effects;
}

std::deque<Particle> VisualObserver::getActiveParticles() {
    std::lock_guard<std::mutex> lck(effects_mutex);
    return particles;
}

void VisualObserver::updateParticles(float dt) {
    std::lock_guard<std::mutex> lck(effects_mutex);
    
    // Обновить все частицы
    for (auto& p : particles) {
        p.update(dt);
    }
    
    // Удалить мёртвые частицы
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
                      [](const Particle& p) { return !p.isAlive(); }),
        particles.end()
    );
}

// ========== VisualWrapper ==========

VisualWrapper::VisualWrapper(int width, int height) 
    : npcs(nullptr),
      messageDisplayTime(sf::Time::Zero) {
    
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 0;
    settings.majorVersion = 2;
    settings.minorVersion = 1;
    
    window.create(sf::VideoMode(width, height), "PixelRPG - Visual Wrapper", 
                  sf::Style::Default, settings);
    
    window.setFramerateLimit(60);
}

bool VisualWrapper::initialize() {
    std::cout << "Initializing VisualWrapper..." << std::endl;
    
    if (!font.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf")) {
        std::cout << "Warning: Could not load font, trying alternative..." << std::endl;
        if (!font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Error: Could not load any font!" << std::endl;
            return false;
        }
    }
    std::cout << "Font loaded successfully" << std::endl;

    createDefaultTextures();
    std::cout << "Textures created successfully" << std::endl;

    interactionText.setFont(font);
    interactionText.setCharacterSize(16);
    interactionText.setFillColor(sf::Color::White);
    interactionText.setPosition(10, 10);

    interactionBox.setSize(sf::Vector2f(400, 50));
    interactionBox.setPosition(10, 10);
    interactionBox.setFillColor(sf::Color(0, 0, 0, 180));
    interactionBox.setOutlineColor(sf::Color::White);
    interactionBox.setOutlineThickness(1);
    
    // Настройка счётчика статистики
    statsText.setFont(font);
    statsText.setCharacterSize(14);
    statsText.setFillColor(sf::Color::White);
    statsText.setPosition(10, window.getSize().y - 40);
    
    statsBox.setSize(sf::Vector2f(200, 30));
    statsBox.setPosition(10, window.getSize().y - 40);
    statsBox.setFillColor(sf::Color(0, 0, 0, 150));
    statsBox.setOutlineColor(sf::Color::Cyan);
    statsBox.setOutlineThickness(1);

    std::cout << "VisualWrapper initialized successfully" << std::endl;
    return true;
}

void VisualWrapper::createDefaultTextures() {
    const int size = 32;
    
    sf::Image orcImg;
    orcImg.create(size, size, sf::Color::Red);
    orcTexture.loadFromImage(orcImg);
    
    sf::Image squirrelImg;
    squirrelImg.create(size, size, sf::Color(139, 69, 19));
    squirrelTexture.loadFromImage(squirrelImg);
    
    sf::Image bearImg;
    bearImg.create(size, size, sf::Color(101, 67, 33));
    bearTexture.loadFromImage(bearImg);
    
    sf::Image druidImg;
    druidImg.create(size, size, sf::Color::Green);
    druidTexture.loadFromImage(druidImg);
    
    sf::Image bgImg;
    bgImg.create(800, 600, sf::Color(50, 50, 100));
    backgroundTexture.loadFromImage(bgImg);
}

sf::Color VisualWrapper::getColorForNPC(NPCType type) const {
    switch (type) {
        case NPCType::Orc: return sf::Color::Red;
        case NPCType::Squirrel: return sf::Color::Green;
        case NPCType::Bear: return sf::Color(101, 67, 33);
        case NPCType::Druid: return sf::Color::Cyan;
        default: return sf::Color::White;
    }
}

void VisualWrapper::setNPCs(std::vector<std::shared_ptr<NPC>>& npcs_list) {
    npcs = &npcs_list;
}

bool VisualWrapper::isWindowOpen() const {
    return window.isOpen();
}

void VisualWrapper::run() {
    while (window.isOpen()) {
        handleEvents();
        render();
        
        if (!lastInteractionMessage.empty()) {
            if (clock.getElapsedTime() > sf::seconds(3)) {
                lastInteractionMessage.clear();
            }
        }
        
        sf::sleep(sf::milliseconds(16));
    }
}

void VisualWrapper::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
        }
    }
}

void VisualWrapper::drawAttackEffect(float x, float y, float progress, sf::Color color) {
    // Пульсирующее кольцо атаки
    float radius = 15.0f + progress * 20.0f;
    sf::CircleShape ring(radius);
    ring.setOrigin(radius, radius);
    ring.setPosition(x, y);
    ring.setFillColor(sf::Color::Transparent);
    
    sf::Uint8 alpha = static_cast<sf::Uint8>(255 * (1.0f - progress));
    color.a = alpha;
    ring.setOutlineColor(color);
    ring.setOutlineThickness(3.0f);
    
    window.draw(ring);
}

void VisualWrapper::drawKillEffect(float x, float y, float progress) {
    // Взрыв - расширяющиеся круги
    for (int i = 0; i < 3; ++i) {
        float offset = i * 0.15f;
        float adjProgress = std::min(1.0f, progress + offset);
        
        float radius = 5.0f + adjProgress * 30.0f;
        sf::CircleShape explosion(radius);
        explosion.setOrigin(radius, radius);
        explosion.setPosition(x, y);
        
        sf::Uint8 alpha = static_cast<sf::Uint8>(200 * (1.0f - adjProgress));
        explosion.setFillColor(sf::Color(255, 100 + i * 30, 0, alpha));
        
        window.draw(explosion);
    }
}

void VisualWrapper::drawEscapeEffect(float x, float y, float progress) {
    // След побега - несколько кругов
    for (int i = 0; i < 4; ++i) {
        float offset = i * 0.2f;
        if (progress < offset) continue;
        
        float adjProgress = (progress - offset) / (1.0f - offset);
        
        float size = 10.0f * (1.0f - adjProgress);
        sf::CircleShape trail(size);
        trail.setOrigin(size, size);
        trail.setPosition(x - i * 8, y - i * 8);
        
        sf::Uint8 alpha = static_cast<sf::Uint8>(150 * (1.0f - adjProgress));
        trail.setFillColor(sf::Color(100, 255, 100, alpha));
        
        window.draw(trail);
    }
}

void VisualWrapper::drawHealEffect(float x, float y, float progress) {
    // Пульсирующее сияние исцеления
    float pulse = std::sin(progress * 3.14159f * 4.0f) * 0.5f + 0.5f;
    float radius = 20.0f + pulse * 10.0f;
    
    sf::CircleShape glow(radius);
    glow.setOrigin(radius, radius);
    glow.setPosition(x, y);
    
    sf::Uint8 alpha = static_cast<sf::Uint8>(150 * (1.0f - progress) * pulse);
    glow.setFillColor(sf::Color(100, 200, 255, alpha));
    
    window.draw(glow);
    
    // Крест исцеления
    sf::RectangleShape hbar(sf::Vector2f(20, 4));
    sf::RectangleShape vbar(sf::Vector2f(4, 20));
    
    hbar.setOrigin(10, 2);
    vbar.setOrigin(2, 10);
    
    hbar.setPosition(x, y);
    vbar.setPosition(x, y);
    
    sf::Uint8 crossAlpha = static_cast<sf::Uint8>(255 * (1.0f - progress));
    hbar.setFillColor(sf::Color(255, 255, 255, crossAlpha));
    vbar.setFillColor(sf::Color(255, 255, 255, crossAlpha));
    
    window.draw(hbar);
    window.draw(vbar);
}

void VisualWrapper::renderEffects(const std::deque<VisualEffect>& effects, float scaleX, float scaleY) {
    for (const auto& effect : effects) {
        float screen_x = effect.x * scaleX;
        float screen_y = effect.y * scaleY;
        float progress = effect.getProgress();
        
        switch (effect.type) {
            case EffectType::Attack:
                drawAttackEffect(screen_x, screen_y, progress, effect.color);
                break;
            case EffectType::Kill:
                drawKillEffect(screen_x, screen_y, progress);
                break;
            case EffectType::Escape:
                drawEscapeEffect(screen_x, screen_y, progress);
                break;
            case EffectType::Heal:
                drawHealEffect(screen_x, screen_y, progress);
                break;
        }
    }
}

void VisualWrapper::renderParticles(const std::deque<Particle>& particles, float scaleX, float scaleY) {
    for (const auto& p : particles) {
        float screen_x = p.x * scaleX;
        float screen_y = p.y * scaleY;
        
        sf::CircleShape particle(2);
        particle.setOrigin(2, 2);
        particle.setPosition(screen_x, screen_y);
        
        sf::Color color = p.color;
        color.a = static_cast<sf::Uint8>(255 * p.getAlpha());
        particle.setFillColor(color);
        
        window.draw(particle);
    }
}

void VisualWrapper::render() {
    // Delta time для частиц
    float dt = frameClock.restart().asSeconds();
    
    // Обновить частицы в VisualObserver
    auto visualObs = std::static_pointer_cast<VisualObserver>(VisualObserver::get());
    visualObs->updateParticles(dt);
    
    window.clear(sf::Color(50, 50, 100));
    
    sf::Sprite background(backgroundTexture);
    window.draw(background);
    
    const float scaleX = static_cast<float>(window.getSize().x) / MAP_X;
    const float scaleY = static_cast<float>(window.getSize().y) / MAP_Y;
    
    // Draw NPCs and corpses
    int aliveCount = 0;
    int deadCount = 0;
    
    {
        std::lock_guard<std::mutex> lock(npcsMutex);
        if (npcs != nullptr) {
            // Подсчитать живых и мёртвых
            for (const auto& npc : *npcs) {
                if (!npc) continue;
                if (npc->is_alive()) aliveCount++;
                else deadCount++;
            }
            
            // СНАЧАЛА рисуем трупы (они на заднем плане)
            for (const auto& npc : *npcs) {
                if (!npc || npc->is_alive()) continue;  // Только мёртвые
                
                auto [visual_x, visual_y] = npc->get_visual_position(300.0f);  // Было 500.0f
                
                float screen_x = visual_x * scaleX;
                float screen_y = visual_y * scaleY;
                
                // Крест для трупа
                sf::RectangleShape hbar(sf::Vector2f(16, 3));
                sf::RectangleShape vbar(sf::Vector2f(3, 16));
                
                hbar.setOrigin(8, 1.5f);
                vbar.setOrigin(1.5f, 8);
                
                hbar.setPosition(screen_x, screen_y);
                vbar.setPosition(screen_x, screen_y);
                
                // Тёмно-красный цвет для трупа
                sf::Color corpseColor(100, 0, 0, 200);
                hbar.setFillColor(corpseColor);
                vbar.setFillColor(corpseColor);
                
                window.draw(hbar);
                window.draw(vbar);
                
                // Полупрозрачное имя
                sf::Text nameText;
                nameText.setFont(font);
                nameText.setString(npc->name.substr(0, 8));
                nameText.setCharacterSize(10);
                nameText.setFillColor(sf::Color(150, 150, 150, 150));  // Серый, полупрозрачный
                nameText.setPosition(screen_x, screen_y - 20);
                window.draw(nameText);
            }
            
            // ПОТОМ рисуем живых NPC (поверх трупов)
            for (const auto& npc : *npcs) {
                if (!npc || !npc->is_alive()) continue;  // Только живые
                
                auto [visual_x, visual_y] = npc->get_visual_position(300.0f);  // Было 500.0f
                
                sf::CircleShape npcShape(8);
                npcShape.setFillColor(getColorForNPC(npc->type));
                npcShape.setOutlineColor(sf::Color::Black);
                npcShape.setOutlineThickness(1);
                
                float screen_x = visual_x * scaleX;
                float screen_y = visual_y * scaleY;
                
                npcShape.setPosition(screen_x - 8, screen_y - 8);
                
                window.draw(npcShape);
                
                sf::Text nameText;
                nameText.setFont(font);
                nameText.setString(npc->name.substr(0, 8));
                nameText.setCharacterSize(10);
                nameText.setFillColor(sf::Color::White);
                nameText.setPosition(screen_x, screen_y - 20);
                window.draw(nameText);
            }
        }
    }
    
    // Draw visual effects
    auto effects = visualObs->getActiveEffects();
    renderEffects(effects, scaleX, scaleY);
    
    // Draw particles
    auto particles = visualObs->getActiveParticles();
    renderParticles(particles, scaleX, scaleY);
    
    // Draw interaction message
    if (!lastInteractionMessage.empty()) {
        interactionBox.setSize(sf::Vector2f(
            static_cast<float>(lastInteractionMessage.length() * 8 + 20), 
            40.0f
        ));
        window.draw(interactionBox);
        
        interactionText.setString(lastInteractionMessage);
        window.draw(interactionText);
    }
    
    // Draw statistics
    std::string statsStr = "Alive: " + std::to_string(aliveCount) + " | Dead: " + std::to_string(deadCount);
    statsText.setString(statsStr);
    window.draw(statsBox);
    window.draw(statsText);
    
    window.display();
}

void VisualWrapper::setInteractionMessage(const std::string& message) {
    lastInteractionMessage = message;
    clock.restart();
}