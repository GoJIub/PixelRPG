#include "../include/visual_wrapper.h"
#include "../include/game_utils.h"
#include <iostream>
#include <cmath>
#include <mutex>
#include <random>

#include <thread>
#include <chrono>

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
    
    auto [target_x, target_y] = target->get_visual_position(300.0f);
    
    switch (outcome) {
        case InteractionOutcome::TargetKilled:
            lastInteractionMessage = actor->name + " killed " + target->name;
            std::cout << ">>> " << lastInteractionMessage << std::endl;
            
            addEffect(EffectType::Kill, target_x, target_y, 800.0f, sf::Color(255, 120, 20));
            addParticles(target_x, target_y, 25, sf::Color(255, 50, 0));
            break;

        case InteractionOutcome::TargetHurted:
            {
                lastInteractionMessage = actor->name + " hurt " + target->name;
                std::cout << ">>> " << lastInteractionMessage << std::endl;
                
                addEffect(EffectType::Hurt, target_x, target_y, 400.0f, sf::Color::Yellow);
                addParticles(target_x, target_y, 10, sf::Color(255, 100, 0));
            }
            break;
            
        case InteractionOutcome::TargetEscaped:
            lastInteractionMessage = target->name + " escaped from " + actor->name;
            std::cout << ">>> " << lastInteractionMessage << std::endl;
            
            addEffect(EffectType::Escape, target_x, target_y, 500.0f, sf::Color::Green);
            break;
            
        case InteractionOutcome::TargetHealed:
            {
                lastInteractionMessage = actor->name + " healed " + target->name;
                std::cout << ">>> " << lastInteractionMessage << std::endl;
                
                addEffect(EffectType::Heal, target_x, target_y, 800.0f, sf::Color::Cyan);
                addParticles(target_x, target_y, 15, sf::Color(100, 255, 200));
            }
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
        
        particles.emplace_back(x, y, vx, vy, color, 0.5f);
    }
}

std::deque<VisualEffect> VisualObserver::getActiveEffects() {
    std::lock_guard<std::mutex> lck(effects_mutex);
    
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
    
    for (auto& p : particles) {
        p.update(dt);
    }
    
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
    
    bool loaded = false;

#ifdef _WIN32
    // Windows: попробуем стандартный Arial
    loaded = this->font.loadFromFile("C:/Windows/Fonts/arial.ttf");
#elif __APPLE__
    // macOS: системный шрифт San Francisco
    loaded = this->font.loadFromFile("/System/Library/Fonts/SFNS.ttf");
#else
    // Linux: DejaVu Sans
    loaded = this->font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
#endif

    // fallback на локальный шрифт
    if (!loaded) {
        loaded = this->font.loadFromFile("assets/fonts/LiberationSans-Regular.ttf");
    }

    if (!loaded) {
        std::cerr << "Error: Could not load any font! Text rendering will fail." << std::endl;
    } else {
        std::cout << "Font loaded successfully" << std::endl;
    }

    createPixelArtTextures();
    std::cout << "Pixel art textures created successfully" << std::endl;

    interactionText.setFont(this->font);  // Now uses member font
    interactionText.setCharacterSize(16);
    interactionText.setFillColor(sf::Color::White);
    interactionText.setPosition(10, 10);

    interactionBox.setSize(sf::Vector2f(400, 50));
    interactionBox.setPosition(10, 10);
    interactionBox.setFillColor(sf::Color(0, 0, 0, 180));
    interactionBox.setOutlineColor(sf::Color::White);
    interactionBox.setOutlineThickness(1);
    
    statsText.setFont(this->font);
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

// Вспомогательная функция для установки пикселя
void setPixel(sf::Image& img, int x, int y, const sf::Color& color) {
    if (x >= 0 && x < static_cast<int>(img.getSize().x) && 
        y >= 0 && y < static_cast<int>(img.getSize().y)) {
        img.setPixel(x, y, color);
    }
}

// Генератор пиксель-арт текстур
void VisualWrapper::createPixelArtTextures() {
    const int size = 32;
    sf::Color transparent(0, 0, 0, 0);
    
    // === ОРК ===
    {
        sf::Image orcImg;
        orcImg.create(size, size, transparent);
        
        sf::Color orcBody(200, 50, 50);      // Красный
        sf::Color orcDark(140, 30, 30);      // Тёмно-красный
        sf::Color orcEye(255, 255, 0);       // Жёлтый глаз
        sf::Color orcTooth(255, 255, 255);   // Белый зуб
        
        // Тело (грубый овал)
        for (int y = 10; y < 26; ++y) {
            for (int x = 8; x < 24; ++x) {
                int dx = x - 16;
                int dy = y - 18;
                if (dx*dx/64.0 + dy*dy/64.0 < 1.0) {
                    setPixel(orcImg, x, y, orcBody);
                }
            }
        }
        
        // Голова
        for (int y = 6; y < 14; ++y) {
            for (int x = 10; x < 22; ++x) {
                int dx = x - 16;
                int dy = y - 10;
                if (dx*dx/36.0 + dy*dy/16.0 < 1.0) {
                    setPixel(orcImg, x, y, orcBody);
                }
            }
        }
        
        // Глаза
        setPixel(orcImg, 13, 9, orcEye);
        setPixel(orcImg, 19, 9, orcEye);
        
        // Зубы (клыки)
        setPixel(orcImg, 14, 12, orcTooth);
        setPixel(orcImg, 18, 12, orcTooth);
        setPixel(orcImg, 14, 13, orcTooth);
        setPixel(orcImg, 18, 13, orcTooth);
        
        // Тени для объёма
        for (int y = 20; y < 26; ++y) {
            for (int x = 8; x < 12; ++x) {
                if (orcImg.getPixel(x, y).a > 0) {
                    setPixel(orcImg, x, y, orcDark);
                }
            }
        }
        
        orcTexture.loadFromImage(orcImg);
    }
    
    // === БЕЛКА ===
    {
        sf::Image squirrelImg;
        squirrelImg.create(size, size, transparent);
        
        sf::Color sqBody(180, 90, 40);       // Коричневый
        sf::Color sqDark(120, 60, 20);       // Тёмно-коричневый
        sf::Color sqEye(0, 0, 0);            // Чёрный глаз
        sf::Color sqNose(255, 150, 150);     // Розовый нос
        
        // Тело (маленькое)
        for (int y = 14; y < 24; ++y) {
            for (int x = 12; x < 20; ++x) {
                int dx = x - 16;
                int dy = y - 19;
                if (dx*dx/16.0 + dy*dy/25.0 < 1.0) {
                    setPixel(squirrelImg, x, y, sqBody);
                }
            }
        }
        
        // Голова
        for (int y = 8; y < 16; ++y) {
            for (int x = 12; x < 20; ++x) {
                int dx = x - 16;
                int dy = y - 12;
                if (dx*dx/16.0 + dy*dy/16.0 < 1.0) {
                    setPixel(squirrelImg, x, y, sqBody);
                }
            }
        }
        
        // Уши (треугольные)
        setPixel(squirrelImg, 13, 7, sqBody);
        setPixel(squirrelImg, 13, 6, sqBody);
        setPixel(squirrelImg, 19, 7, sqBody);
        setPixel(squirrelImg, 19, 6, sqBody);
        
        // Пушистый хвост (большой и кудрявый)
        for (int y = 16; y < 28; ++y) {
            for (int x = 18; x < 28; ++x) {
                int dx = x - 22;
                int dy = y - 22;
                if (dx*dx/36.0 + dy*dy/36.0 < 1.0) {
                    setPixel(squirrelImg, x, y, sqDark);
                }
            }
        }
        
        // Глаза
        setPixel(squirrelImg, 14, 11, sqEye);
        setPixel(squirrelImg, 18, 11, sqEye);
        
        // Нос
        setPixel(squirrelImg, 16, 13, sqNose);
        
        squirrelTexture.loadFromImage(squirrelImg);
    }
    
    // === МЕДВЕДЬ ===
    {
        sf::Image bearImg;
        bearImg.create(size, size, transparent);
        
        sf::Color bearBody(101, 67, 33);     // Коричневый
        sf::Color bearDark(70, 45, 20);      // Тёмно-коричневый
        sf::Color bearEye(0, 0, 0);          // Чёрный
        sf::Color bearNose(50, 50, 50);      // Серый нос
        
        // Тело (крупное)
        for (int y = 12; y < 28; ++y) {
            for (int x = 6; x < 26; ++x) {
                int dx = x - 16;
                int dy = y - 20;
                if (dx*dx/100.0 + dy*dy/64.0 < 1.0) {
                    setPixel(bearImg, x, y, bearBody);
                }
            }
        }
        
        // Голова (большая круглая)
        for (int y = 4; y < 16; ++y) {
            for (int x = 10; x < 22; ++x) {
                int dx = x - 16;
                int dy = y - 10;
                if (dx*dx/36.0 + dy*dy/36.0 < 1.0) {
                    setPixel(bearImg, x, y, bearBody);
                }
            }
        }
        
        // Уши (круглые)
        for (int y = 3; y < 7; ++y) {
            for (int x = 10; x < 14; ++x) {
                int dx = x - 12;
                int dy = y - 5;
                if (dx*dx/4.0 + dy*dy/4.0 < 1.0) {
                    setPixel(bearImg, x, y, bearDark);
                }
            }
        }
        for (int y = 3; y < 7; ++y) {
            for (int x = 18; x < 22; ++x) {
                int dx = x - 20;
                int dy = y - 5;
                if (dx*dx/4.0 + dy*dy/4.0 < 1.0) {
                    setPixel(bearImg, x, y, bearDark);
                }
            }
        }
        
        // Глаза
        setPixel(bearImg, 13, 9, bearEye);
        setPixel(bearImg, 14, 9, bearEye);
        setPixel(bearImg, 18, 9, bearEye);
        setPixel(bearImg, 19, 9, bearEye);
        
        // Морда
        setPixel(bearImg, 15, 12, bearDark);
        setPixel(bearImg, 16, 12, bearNose);
        setPixel(bearImg, 17, 12, bearDark);
        setPixel(bearImg, 16, 13, bearDark);
        
        // Тени
        for (int y = 22; y < 28; ++y) {
            for (int x = 6; x < 12; ++x) {
                if (bearImg.getPixel(x, y).a > 0) {
                    setPixel(bearImg, x, y, bearDark);
                }
            }
        }
        
        bearTexture.loadFromImage(bearImg);
    }
    
    // === ДРУИД ===
    {
        sf::Image druidImg;
        druidImg.create(size, size, transparent);
        
        sf::Color druidRobe(50, 150, 100);   // Зелёная роба
        sf::Color druidDark(30, 100, 60);    // Тёмно-зелёный
        sf::Color druidSkin(255, 220, 180);  // Кожа
        sf::Color druidHair(100, 70, 40);    // Волосы
        sf::Color druidEye(100, 150, 255);   // Голубые глаза
        sf::Color druidLeaf(100, 255, 100);  // Яркий лист
        
        // Роба (треугольная форма)
        for (int y = 14; y < 28; ++y) {
            int width = (y - 14) / 2 + 4;
            for (int x = 16 - width; x < 16 + width; ++x) {
                setPixel(druidImg, x, y, druidRobe);
            }
        }
        
        // Голова
        for (int y = 6; y < 14; ++y) {
            for (int x = 12; x < 20; ++x) {
                int dx = x - 16;
                int dy = y - 10;
                if (dx*dx/16.0 + dy*dy/16.0 < 1.0) {
                    setPixel(druidImg, x, y, druidSkin);
                }
            }
        }
        
        // Волосы/борода
        for (int x = 11; x < 21; ++x) {
            setPixel(druidImg, x, 6, druidHair);
            setPixel(druidImg, x, 7, druidHair);
        }
        setPixel(druidImg, 12, 13, druidHair);
        setPixel(druidImg, 13, 13, druidHair);
        setPixel(druidImg, 18, 13, druidHair);
        setPixel(druidImg, 19, 13, druidHair);
        
        // Глаза
        setPixel(druidImg, 13, 10, druidEye);
        setPixel(druidImg, 19, 10, druidEye);
        
        // Магический листок над головой (символ природы)
        setPixel(druidImg, 16, 3, druidLeaf);
        setPixel(druidImg, 15, 4, druidLeaf);
        setPixel(druidImg, 16, 4, druidLeaf);
        setPixel(druidImg, 17, 4, druidLeaf);
        setPixel(druidImg, 16, 5, druidLeaf);
        
        // Посох в руке (сбоку от робы)
        for (int y = 16; y < 28; ++y) {
            setPixel(druidImg, 22, y, druidHair);
        }
        setPixel(druidImg, 21, 15, druidLeaf);
        setPixel(druidImg, 22, 15, druidLeaf);
        setPixel(druidImg, 23, 15, druidLeaf);
        
        // Тени на робе
        for (int y = 20; y < 28; ++y) {
            setPixel(druidImg, 16 - (y-14)/2, y, druidDark);
        }
        
        druidTexture.loadFromImage(druidImg);
    }
    
    // === ДРАКОН ===
    {
        sf::Image dragonImg;
        dragonImg.create(size, size, transparent);
        
        sf::Color dragonBody(180, 50, 50);      // Тёмно-красный
        sf::Color dragonDark(120, 30, 30);      // Очень тёмно-красный
        sf::Color dragonScale(220, 80, 80);     // Светлые чешуйки
        sf::Color dragonEye(255, 200, 0);       // Золотой глаз
        sf::Color dragonFire(255, 150, 0);      // Огонь
        sf::Color dragonHorn(240, 240, 240);    // Рога
        
        // Тело (массивное)
        for (int y = 15; y < 28; ++y) {
            for (int x = 8; x < 24; ++x) {
                int dx = x - 16;
                int dy = y - 21;
                if (dx*dx/64.0 + dy*dy/49.0 < 1.0) {
                    setPixel(dragonImg, x, y, dragonBody);
                }
            }
        }
        
        // Голова (большая, угловатая)
        for (int y = 6; y < 17; ++y) {
            for (int x = 10; x < 22; ++x) {
                int dx = x - 16;
                int dy = y - 11;
                if (dx*dx/36.0 + dy*dy/25.0 < 1.0) {
                    setPixel(dragonImg, x, y, dragonBody);
                }
            }
        }
        
        // Морда (вытянутая)
        for (int x = 16; x < 20; ++x) {
            setPixel(dragonImg, x, 14, dragonDark);
            setPixel(dragonImg, x, 15, dragonDark);
        }
        
        // Рога (два острых)
        setPixel(dragonImg, 12, 5, dragonHorn);
        setPixel(dragonImg, 12, 4, dragonHorn);
        setPixel(dragonImg, 12, 3, dragonHorn);
        setPixel(dragonImg, 11, 4, dragonHorn);
        
        setPixel(dragonImg, 20, 5, dragonHorn);
        setPixel(dragonImg, 20, 4, dragonHorn);
        setPixel(dragonImg, 20, 3, dragonHorn);
        setPixel(dragonImg, 21, 4, dragonHorn);
        
        // Глаз (светящийся)
        setPixel(dragonImg, 13, 10, dragonEye);
        setPixel(dragonImg, 14, 10, dragonEye);
        setPixel(dragonImg, 13, 11, dragonEye);
        setPixel(dragonImg, 14, 11, dragonEye);
        
        // Зрачок
        setPixel(dragonImg, 13, 10, sf::Color(0, 0, 0));
        
        // Крылья (острые, сложенные)
        // Левое крыло
        for (int y = 12; y < 22; ++y) {
            for (int x = 4; x < 10; ++x) {
                int dx = x - 7;
                int dy = y - 17;
                if (dx*dx/9.0 + dy*dy/25.0 < 1.0) {
                    setPixel(dragonImg, x, y, dragonDark);
                }
            }
        }
        
        // Правое крыло
        for (int y = 12; y < 22; ++y) {
            for (int x = 22; x < 28; ++x) {
                int dx = x - 25;
                int dy = y - 17;
                if (dx*dx/9.0 + dy*dy/25.0 < 1.0) {
                    setPixel(dragonImg, x, y, dragonDark);
                }
            }
        }
        
        // Чешуйки на теле (для деталей)
        setPixel(dragonImg, 12, 18, dragonScale);
        setPixel(dragonImg, 14, 20, dragonScale);
        setPixel(dragonImg, 16, 22, dragonScale);
        setPixel(dragonImg, 18, 20, dragonScale);
        setPixel(dragonImg, 20, 18, dragonScale);
        
        setPixel(dragonImg, 11, 20, dragonScale);
        setPixel(dragonImg, 13, 22, dragonScale);
        setPixel(dragonImg, 19, 22, dragonScale);
        setPixel(dragonImg, 21, 20, dragonScale);
        
        // Пламя изо рта
        setPixel(dragonImg, 20, 14, dragonFire);
        setPixel(dragonImg, 21, 14, dragonFire);
        setPixel(dragonImg, 22, 14, dragonFire);
        setPixel(dragonImg, 21, 13, dragonFire);
        setPixel(dragonImg, 22, 13, sf::Color(255, 220, 100));
        
        // Хвост (острый кончик сзади)
        setPixel(dragonImg, 23, 26, dragonDark);
        setPixel(dragonImg, 24, 27, dragonDark);
        setPixel(dragonImg, 25, 28, dragonDark);
        setPixel(dragonImg, 26, 29, dragonDark);
        
        // Шипы на спине
        setPixel(dragonImg, 14, 16, dragonHorn);
        setPixel(dragonImg, 16, 17, dragonHorn);
        setPixel(dragonImg, 18, 16, dragonHorn);
        
        dragonTexture.loadFromImage(dragonImg);
    }
    
    // Фон
    sf::Image bgImg;
    bgImg.create(800, 600, sf::Color(40, 45, 60));
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

void VisualWrapper::drawHealthBar(float screen_x, float screen_y, int hp, int maxHp) {
    float ratio = static_cast<float>(hp) / maxHp;

    float barWidth = 32.0f;
    float barHeight = 5.0f;
    float x = screen_x - barWidth / 2;
    float y = screen_y - 20; // Поднимаем выше, чтобы не перекрывать спрайт

    sf::RectangleShape back(sf::Vector2f(barWidth, barHeight));
    back.setPosition(x, y);
    back.setFillColor(sf::Color(0, 0, 0, 180));
    back.setOutlineColor(sf::Color(60, 60, 60));
    back.setOutlineThickness(0.5f);

    sf::Color fillColor;
    if (ratio > 0.6f)       fillColor = sf::Color(70, 255, 70);
    else if (ratio > 0.3f)  fillColor = sf::Color(255, 200, 50);
    else                    fillColor = sf::Color(255, 70, 50);

    sf::RectangleShape fill(sf::Vector2f(barWidth * ratio, barHeight));
    fill.setPosition(x, y);
    fill.setFillColor(fillColor);

    window.draw(back);
    window.draw(fill);
}

void VisualWrapper::setNPCs(std::vector<std::shared_ptr<NPC>>& npcs_list) {
    npcs = &npcs_list;
}

bool VisualWrapper::isWindowOpen() const {
    return window.isOpen();
}

void VisualWrapper::setEffectsCVPtr(std::condition_variable* cv, std::mutex* mtx) {
    effects_cv_ptr = cv;
    cv_mtx_ptr = mtx;
}

void VisualWrapper::setRunningPtr(std::atomic<bool>* r) {
    running_ptr = r;
}

void VisualWrapper::run() {
    while (window.isOpen() && (!running_ptr || *running_ptr)) {
        handleEvents();
        
        float dt = frameClock.restart().asSeconds();
        
        if (effects_cv_ptr && cv_mtx_ptr) {
            std::unique_lock<std::mutex> cv_lock(*cv_mtx_ptr);
            effects_cv_ptr->wait_for(cv_lock, std::chrono::milliseconds(16));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        auto visualObs = std::static_pointer_cast<VisualObserver>(VisualObserver::get());
        
        if (!lastInteractionMessage.empty()) {
            if (clock.getElapsedTime() > sf::seconds(3)) {
                lastInteractionMessage.clear();
            }
        }
        
        visualObs->updateParticles(dt);
        render();
    }
    
    if (window.isOpen()) {
        window.close();
    }
}

void VisualWrapper::setPausedPtr(std::atomic<bool>* p) {
    paused = p;
}

void VisualWrapper::handleEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
            if (running_ptr) {  // Проверяем, чтобы избежать null-дереференса
                *running_ptr = false;  // Завершаем все потоки
            }
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space) {
            if (paused) *paused = !*paused;  // Уже есть, оставляем
        }
    }
}

void VisualWrapper::drawKillEffect(float x, float y, float progress) {
    float radius = 10.0f + progress * 60.0f;

    sf::CircleShape c(radius);
    c.setOrigin(radius, radius);
    c.setPosition(x, y);

    sf::Color col(255, 80, 20, static_cast<sf::Uint8>(255 * (1 - progress)));
    c.setFillColor(col);

    window.draw(c);

    float waveRadius = radius + 10;
    sf::CircleShape wave(waveRadius);
    wave.setOrigin(waveRadius, waveRadius);
    wave.setPosition(x, y);

    wave.setFillColor(sf::Color(255, 200, 0, static_cast<sf::Uint8>(160 * (1 - progress))));
    window.draw(wave);
}

void VisualWrapper::drawHurtEffect(float x, float y, float progress) {
    float radius = 5.0f + progress * 15.0f;

    sf::CircleShape c(radius);
    c.setOrigin(radius, radius);
    c.setPosition(x, y);

    sf::Color col(255, 255, 0, static_cast<sf::Uint8>(220 * (1 - progress)));
    c.setFillColor(col);

    window.draw(c);
    
    float radius2 = 3.0f + progress * 10.0f;
    sf::CircleShape c2(radius2);
    c2.setOrigin(radius2, radius2);
    c2.setPosition(x, y);
    c2.setFillColor(sf::Color(255, 150, 0, static_cast<sf::Uint8>(180 * (1 - progress))));
    
    window.draw(c2);
}

void VisualWrapper::drawEscapeEffect(float x, float y, float progress) {
    // Улучшенный эффект уклонения: быстрое мерцание + след в случайном направлении
    
    // Мерцание персонажа (afterimage effect)
    if (progress < 0.5f) {
        float pulseAlpha = std::sin(progress * 3.14159f * 10.0f);
        if (pulseAlpha > 0) {
            sf::CircleShape flash(15);
            flash.setOrigin(15, 15);
            flash.setPosition(x, y);
            
            sf::Uint8 alpha = static_cast<sf::Uint8>(200 * pulseAlpha * (1.0f - progress * 2));
            flash.setFillColor(sf::Color(100, 255, 100, alpha));
            window.draw(flash);
        }
    }
    
    // Динамический след уклонения (используем hash от координат для "случайного" направления)
    int directionSeed = static_cast<int>(x * 1000 + y) % 8;
    float angle = directionSeed * 3.14159f / 4.0f; // 8 направлений
    
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    
    // Рисуем след из частиц в направлении уклонения
    for (int i = 0; i < 6; ++i) {
        float offset = i * 0.15f;
        if (progress < offset) continue;
        
        float adjProgress = (progress - offset) / (1.0f - offset);
        
        // Расстояние след растёт со временем
        float distance = (i + progress * 3) * 8.0f;
        float trail_x = x - cos_a * distance;
        float trail_y = y - sin_a * distance;
        
        float size = 8.0f * (1.0f - adjProgress);
        sf::CircleShape trail(size);
        trail.setOrigin(size, size);
        trail.setPosition(trail_x, trail_y);
        
        sf::Uint8 alpha = static_cast<sf::Uint8>(180 * (1.0f - adjProgress));
        
        // Градиент от зелёного к жёлтому
        int green = 255;
        int red = 100 + static_cast<int>(155 * adjProgress);
        trail.setFillColor(sf::Color(red, green, 100, alpha));
        
        window.draw(trail);
        
        // Дополнительные искры
        if (i % 2 == 0) {
            sf::CircleShape spark(2);
            spark.setOrigin(2, 2);
            spark.setPosition(trail_x + (i % 3 - 1) * 4, trail_y + (i % 3 - 1) * 4);
            spark.setFillColor(sf::Color(255, 255, 255, alpha / 2));
            window.draw(spark);
        }
    }
    
    // Конечный "дым" уклонения
    if (progress > 0.3f) {
        float smokeProgress = (progress - 0.3f) / 0.7f;
        float smokeRadius = 5.0f + smokeProgress * 20.0f;
        
        sf::CircleShape smoke(smokeRadius);
        smoke.setOrigin(smokeRadius, smokeRadius);
        smoke.setPosition(x, y);
        
        sf::Uint8 smokeAlpha = static_cast<sf::Uint8>(100 * (1.0f - smokeProgress));
        smoke.setFillColor(sf::Color(150, 255, 150, smokeAlpha));
        window.draw(smoke);
    }
}

void VisualWrapper::drawHealEffect(float x, float y, float progress) {
    float pulse = std::sin(progress * 3.14159f * 4.0f) * 0.5f + 0.5f;
    float radius = 20.0f + pulse * 10.0f;
    
    sf::CircleShape glow(radius);
    glow.setOrigin(radius, radius);
    glow.setPosition(x, y);
    
    sf::Uint8 alpha = static_cast<sf::Uint8>(150 * (1.0f - progress) * pulse);
    glow.setFillColor(sf::Color(100, 200, 255, alpha));
    
    window.draw(glow);
    
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
            case EffectType::Kill:
                drawKillEffect(screen_x, screen_y, progress);
                break;
            case EffectType::Hurt:
                drawHurtEffect(screen_x, screen_y, progress);
                break;
            case EffectType::Escape:
                drawEscapeEffect(screen_x, screen_y, progress);
                break;
            case EffectType::Heal:
                drawHealEffect(screen_x, screen_y, progress);
                break;
            default:
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
    window.clear(sf::Color(50, 50, 100));
    
    sf::Sprite background(backgroundTexture);
    window.draw(background);
    
    const float scaleX = static_cast<float>(window.getSize().x) / MAP_X;
    const float scaleY = static_cast<float>(window.getSize().y) / MAP_Y;
    
    int aliveCount = 0;
    int deadCount = 0;
    
    {
        std::lock_guard<std::mutex> lock(npcsMutex);
        if (npcs != nullptr) {
            for (const auto& npc : *npcs) {
                if (!npc) continue;
                if (npc->is_alive()) aliveCount++;
                else deadCount++;
            }
            
            // Сначала рисуем трупы (на заднем плане)
            for (const auto& npc : *npcs) {
                if (!npc || npc->is_alive()) continue;
                
                auto [visual_x, visual_y] = npc->get_visual_position(300.0f);
                
                float screen_x = visual_x * scaleX;
                float screen_y = visual_y * scaleY;
                
                // Крест для трупа
                sf::RectangleShape hbar(sf::Vector2f(16, 3));
                sf::RectangleShape vbar(sf::Vector2f(3, 16));
                
                hbar.setOrigin(8, 1.5f);
                vbar.setOrigin(1.5f, 8);
                
                hbar.setPosition(screen_x, screen_y);
                vbar.setPosition(screen_x, screen_y);
                
                sf::Color corpseColor(100, 0, 0, 200);
                hbar.setFillColor(corpseColor);
                vbar.setFillColor(corpseColor);
                
                window.draw(hbar);
                window.draw(vbar);
                
                sf::Text nameText;
                nameText.setFont(font);
                nameText.setString(npc->name.substr(0, 8));
                nameText.setCharacterSize(10);
                nameText.setFillColor(sf::Color(150, 150, 150, 150));
                nameText.setPosition(screen_x, screen_y - 25);
                window.draw(nameText);
            }
            
            // Потом рисуем живых NPC с пиксель-арт спрайтами
            for (const auto& npc : *npcs) {
                if (!npc || !npc->is_alive()) continue;
                
                auto [visual_x, visual_y] = npc->get_visual_position(300.0f);
                
                float screen_x = visual_x * scaleX;
                float screen_y = visual_y * scaleY;
                
                // Выбираем текстуру на основе типа NPC
                sf::Sprite npcSprite;
                switch (npc->type) {
                    case NPCType::Bear:
                        npcSprite.setTexture(bearTexture);
                        break;
                    case NPCType::Dragon:
                        npcSprite.setTexture(dragonTexture);
                        break;
                    case NPCType::Druid:
                        npcSprite.setTexture(druidTexture);
                        break;
                    case NPCType::Orc:
                        npcSprite.setTexture(orcTexture);
                        break;
                    case NPCType::Squirrel:
                        npcSprite.setTexture(squirrelTexture);
                        break;
                    default:
                        npcSprite.setTexture(orcTexture);
                }
                
                npcSprite.setOrigin(16, 16); // Центр спрайта 32x32
                npcSprite.setPosition(screen_x, screen_y);
                
                window.draw(npcSprite);
                
                // Полоска здоровья
                int currentHP = npc->get_current_health();
                int maxHP = npc->get_max_health();
                drawHealthBar(screen_x, screen_y, currentHP, maxHP);
                
                // Имя NPC
                sf::Text nameText;
                nameText.setFont(font);
                nameText.setString(npc->name.substr(0, 10));
                nameText.setCharacterSize(10);
                nameText.setFillColor(sf::Color::White);
                nameText.setPosition(screen_x - 20, screen_y + 18);
                window.draw(nameText);
            }
        }
    }
    
    auto visualObs = std::static_pointer_cast<VisualObserver>(VisualObserver::get());
    
    auto effects = visualObs->getActiveEffects();
    renderEffects(effects, scaleX, scaleY);
    
    auto particles = visualObs->getActiveParticles();
    renderParticles(particles, scaleX, scaleY);
    
    if (!lastInteractionMessage.empty()) {
        interactionBox.setSize(sf::Vector2f(
            static_cast<float>(lastInteractionMessage.length() * 8 + 20), 
            40.0f
        ));
        window.draw(interactionBox);
        
        interactionText.setString(lastInteractionMessage);
        window.draw(interactionText);
    }
    
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