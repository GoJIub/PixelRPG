#pragma once
#include "npc.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class VisualObserver : public IInteractionObserver {
private:
    // Store the last interaction message
    std::string lastInteractionMessage;
    
public:
    VisualObserver();
    ~VisualObserver() = default;
    
    // IInteractionObserver implementation
    void on_interaction(const std::shared_ptr<NPC>& actor,
                       const std::shared_ptr<NPC>& target,
                       InteractionOutcome outcome) override;
    
    // Get the last interaction message
    std::string getLastInteractionMessage() const;
};

class VisualWrapper {
private:
    sf::RenderWindow window;
    sf::Font font;
    sf::Clock clock;
    
    // NPC sprites and textures
    sf::Texture orcTexture;
    sf::Texture squirrelTexture;
    sf::Texture bearTexture;
    sf::Texture druidTexture;
    sf::Texture backgroundTexture;
    
    // Text for displaying interaction messages
    sf::Text interactionText;
    sf::RectangleShape interactionBox;
    
    // Store NPCs for rendering
    std::vector<std::shared_ptr<NPC>>* npcs;
    mutable std::mutex npcsMutex;
    
    // Interaction message variables
    std::string lastInteractionMessage;
    sf::Time messageDisplayTime;
    
    // Create default textures if SFML assets aren't available
    void createDefaultTextures();
    
    // Get color based on NPC type
    sf::Color getColorForNPC(NPCType type) const;

public:
    VisualWrapper(int width = 800, int height = 600);
    ~VisualWrapper() = default;
    
    // Initialize the visual wrapper
    bool initialize();
    
    // Set the NPCs to be displayed
    void setNPCs(std::vector<std::shared_ptr<NPC>>& npcs_list);
    
    // Set the interaction message
    void setInteractionMessage(const std::string& message);
    
    // Main render loop
    void run();
    
    // Handle events
    void handleEvents();
    
    // Render all elements
    void render();
};