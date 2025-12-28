#include "../include/visual_wrapper.h"
#include "../include/game_utils.h"
#include <iostream>
#include <cmath>
#include <mutex>

VisualObserver::VisualObserver() {
    // Initialize member variables
}

void VisualObserver::on_interaction(const std::shared_ptr<NPC>& actor,
                                  const std::shared_ptr<NPC>& target,
                                  InteractionOutcome outcome) {
    if (!actor || !target) return;
    
    std::string outcomeStr;
    switch (outcome) {
        case InteractionOutcome::TargetKilled:
            outcomeStr = "killed";
            break;
        case InteractionOutcome::TargetEscaped:
            outcomeStr = "escaped";
            break;
        case InteractionOutcome::TargetHealed:
            outcomeStr = "healed";
            break;
        case InteractionOutcome::NoInteraction:
            outcomeStr = "no interaction";
            break;
    }
    
    lastInteractionMessage = actor->name + " " + outcomeStr + " " + target->name;
    
    std::cout << "Interaction: " << lastInteractionMessage << std::endl;
}

std::string VisualObserver::getLastInteractionMessage() const {
    return lastInteractionMessage;
}

VisualWrapper::VisualWrapper(int width, int height) 
    : window(sf::VideoMode(width, height), "PixelRPG - Visual Wrapper"),
      npcs(nullptr),
      messageDisplayTime(sf::Time::Zero) {
    // Initialize member variables
    window.setFramerateLimit(60);
}

bool VisualWrapper::initialize() {
    // Load font (try to load a font file, or use default if not available)
    if (!font.loadFromFile("arial.ttf")) {
        // If font loading fails, SFML will use a default font
        std::cout << "Warning: Could not load arial.ttf, using default font." << std::endl;
    }
    
    createDefaultTextures();
    
    // Set up the interaction text display
    interactionText.setFont(font);
    interactionText.setCharacterSize(16);
    interactionText.setFillColor(sf::Color::White);
    interactionText.setPosition(10, 10);
    
    // Set up the interaction message box
    interactionBox.setSize(sf::Vector2f(400, 50));
    interactionBox.setPosition(10, 10);
    interactionBox.setFillColor(sf::Color(0, 0, 0, 180)); // Semi-transparent black
    interactionBox.setOutlineColor(sf::Color::White);
    interactionBox.setOutlineThickness(1);
    
    return true;
}

void VisualWrapper::createDefaultTextures() {
    // Create simple colored rectangles as textures for different NPC types
    const int size = 32;
    
    // Create a simple texture for Orc (red)
    sf::Image orcImg;
    orcImg.create(size, size, sf::Color::Red);
    orcTexture.loadFromImage(orcImg);
    
    // Create a simple texture for Squirrel (brown)
    sf::Image squirrelImg;
    squirrelImg.create(size, size, sf::Color(139, 69, 19)); // Brown
    squirrelTexture.loadFromImage(squirrelImg);
    
    // Create a simple texture for Bear (dark brown)
    sf::Image bearImg;
    bearImg.create(size, size, sf::Color(101, 67, 33)); // Dark brown
    bearTexture.loadFromImage(bearImg);
    
    // Create a simple texture for Druid (green)
    sf::Image druidImg;
    druidImg.create(size, size, sf::Color::Green);
    druidTexture.loadFromImage(druidImg);
    
    // Create a simple background texture
    sf::Image bgImg;
    bgImg.create(800, 600, sf::Color(50, 50, 100)); // Dark blue background
    backgroundTexture.loadFromImage(bgImg);
}

sf::Color VisualWrapper::getColorForNPC(NPCType type) const {
    switch (type) {
        case NPCType::Orc: return sf::Color::Red;
        case NPCType::Squirrel: return sf::Color(139, 69, 19); // Brown
        case NPCType::Bear: return sf::Color(101, 67, 33); // Dark brown
        case NPCType::Druid: return sf::Color::Green;
        default: return sf::Color::White;
    }
}

void VisualWrapper::setNPCs(std::vector<std::shared_ptr<NPC>>& npcs_list) {
    npcs = &npcs_list;
}

void VisualWrapper::run() {
    while (window.isOpen()) {
        handleEvents();
        render();
        
        // Clear the interaction message after some time
        if (!lastInteractionMessage.empty()) {
            if (clock.getElapsedTime() > sf::seconds(3)) { // Clear after 3 seconds
                lastInteractionMessage.clear();
            }
        }
        
        // Small delay to prevent excessive CPU usage
        sf::sleep(sf::milliseconds(16)); // ~60 FPS
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

void VisualWrapper::render() {
    window.clear();
    
    // Draw background
    sf::Sprite background(backgroundTexture);
    window.draw(background);
    
    // Calculate scale factors to map game coordinates to screen coordinates
    const float scaleX = static_cast<float>(window.getSize().x) / MAP_X;
    const float scaleY = static_cast<float>(window.getSize().y) / MAP_Y;
    
    // Draw NPCs - use a lock to safely access the NPC vector
    {
        std::lock_guard<std::mutex> lock(npcsMutex);
        if (npcs != nullptr) {
            for (const auto& npc : *npcs) {
                if (!npc || !npc->is_alive()) continue;
                
                sf::CircleShape npcShape(8); // Use a circle for simplicity
                npcShape.setFillColor(getColorForNPC(npc->type));
                npcShape.setOutlineColor(sf::Color::Black);
                npcShape.setOutlineThickness(1);
                
                // Position based on NPC coordinates
                float x = static_cast<float>(npc->x) * scaleX;
                float y = static_cast<float>(npc->y) * scaleY;
                
                npcShape.setPosition(x - 8, y - 8); // Center the shape at the position
                
                window.draw(npcShape);
                
                // Draw NPC name
                sf::Text nameText;
                nameText.setFont(font);
                nameText.setString(npc->name.substr(0, 8)); // Limit name length
                nameText.setCharacterSize(10);
                nameText.setFillColor(sf::Color::White);
                nameText.setPosition(x, y - 20);
                window.draw(nameText);
            }
        }
    }
    
    // Draw interaction message if there is one
    if (!lastInteractionMessage.empty()) {
        interactionBox.setSize(sf::Vector2f(
            static_cast<float>(lastInteractionMessage.length() * 8 + 20), 
            40.0f
        ));
        window.draw(interactionBox);
        
        interactionText.setString(lastInteractionMessage);
        window.draw(interactionText);
    }
    
    window.display();
}

void VisualWrapper::setInteractionMessage(const std::string& message) {
    lastInteractionMessage = message;
    clock.restart(); // Reset the timer for the message
}