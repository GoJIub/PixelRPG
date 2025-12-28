#include "include/npc.h"
#include "include/game_utils.h"
#include "include/visual_wrapper.h"

#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;


int main() {
    auto consoleObs = ConsoleObserver::get();
    auto fileObs = FileObserver::get("log.txt");

    // ---- NPCs ----
    std::vector<std::shared_ptr<NPC>> npcs;
    constexpr int NPC_COUNT = 50;

    for (int i = 0; i < NPC_COUNT; ++i) {
        NPCType t = random_type();
        std::string name;
        if (t == NPCType::Orc)           name = "Orc_" + std::to_string(i+1);
        else if (t == NPCType::Bear)     name = "Bear_" + std::to_string(i+1);
        else if (t == NPCType::Squirrel) name = "Squirrel_" + std::to_string(i+1);
        else                             name = "Druid_" + std::to_string(i+1);

        auto npc = createNPC(
            t,
            name,
            random_coord(0, MAP_X),
            random_coord(0, MAP_Y)
        );

        // npc->subscribe(consoleObs);
        npc->subscribe(fileObs);
        npcs.push_back(npc);
    }

    print_all(npcs);

    // Create visual observer using singleton
    auto visualObserver = VisualObserver::get();
    
    // Subscribe all NPCs to visual observer
    for (auto& npc : npcs) {
        npc->subscribe(visualObserver);
    }
        
    std::atomic<bool> running{true};
    
    // ---- Interaction thread ----
    std::thread Interaction_thread(std::ref(InteractionManager::instance()));
    
    // ---- Move + detect ----
    std::thread move_thread([&]() {
        while (running) {
            for (auto& npc : npcs) {
                if (!npc->is_alive()) continue;
    
                int d = npc->get_move_distance();
                npc->move(
                    std::rand() % (2 * d + 1) - d,
                    std::rand() % (2 * d + 1) - d,
                    MAP_X,
                    MAP_Y
                );
            }
    
            for (size_t i = 0; i < npcs.size(); ++i)
                for (size_t j = i + 1; j < npcs.size(); ++j)
                    InteractionManager::instance().push({npcs[i], npcs[j]});
    
            std::this_thread::sleep_for(500ms); // Плавная интерполяция сделает движение гладким
        }
    });
    
    // ---- Visual wrapper thread ----
    // ВАЖНО: создаём и инициализируем визуальный враппер ВНУТРИ потока
    std::thread visual_thread([&]() {
        // Создаём окно в этом потоке
        VisualWrapper visualWrapper(800, 600);
        if (!visualWrapper.initialize()) {
            std::cerr << "Failed to initialize visual wrapper" << std::endl;
            return;
        }
        
        std::cout << "Visual wrapper initialized in visual thread" << std::endl;
        
        // Установить NPCs
        visualWrapper.setNPCs(npcs);
        
        // Запустить цикл отрисовки
        while (running && visualWrapper.isWindowOpen()) {
            // Обновить сообщение о взаимодействии из singleton
            auto visualObs = std::static_pointer_cast<VisualObserver>(VisualObserver::get());
            std::string lastMsg = visualObs->getLastInteractionMessage();
            if (!lastMsg.empty()) {
                visualWrapper.setInteractionMessage(lastMsg);
            }
            
            visualWrapper.handleEvents();
            visualWrapper.render();
            
            std::this_thread::sleep_for(16ms); // 60 FPS для плавной интерполяции
        }
        
        running = false; // Если окно закрыли, остановить игру
    });

    // ---- Game duration ----
    std::this_thread::sleep_for(30s);
    running = false;

    // ---- Finish ----
    move_thread.join();
    
    // Дождаться завершения визуального потока
    visual_thread.join();
    
    InteractionManager::instance().stop();
    Interaction_thread.join();

    print_survivors(npcs);
    return 0;
}