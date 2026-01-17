#include "include/npc.h"
#include "include/game_utils.h"
#include "include/visual_wrapper.h"

#include <memory>
#include <array>
#include <thread>
#include <atomic>
#include <future>
#include <chrono>
#include <unordered_map>

using namespace std::chrono_literals;

bool hasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == flag) return true;
    }
    return false;
}

int main(int argc, char** argv) {
    bool headless = hasFlag(argc, argv, "--headless");

    // auto consoleObs = ConsoleObserver::get();
    auto fileObs = FileObserver::get("log.txt");

    // ---- NPCs ----
    std::vector<std::shared_ptr<NPC>> npcs;
    constexpr int NPC_COUNT = 50;
    constexpr int MAX_DRAGONS = 1;

    int dragonCount = 0;
    for (int i = 0; i < NPC_COUNT; ++i) {
        NPCType t = random_type();
        while (t == NPCType::Dragon && dragonCount >= MAX_DRAGONS)
            t = random_type();
        if (t == NPCType::Dragon) dragonCount++;

        std::string name;
        switch(t) {
            case NPCType::Bear: name = "Bear_" + std::to_string(i+1); break;
            case NPCType::Dragon: name = "Dragon_" + std::to_string(i+1); break;
            case NPCType::Druid: name = "Druid_" + std::to_string(i+1); break;
            case NPCType::Orc: name = "Orc_" + std::to_string(i+1); break;
            case NPCType::Squirrel: name = "Squirrel_" + std::to_string(i+1); break;
        }

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

#ifndef PIXELRPG_HEADLESS
    auto visualObserver = VisualObserver::get();
    if (!headless) {
        for (auto& npc : npcs)
            npc->subscribe(visualObserver);
    }
#endif

    print_all(npcs);

    std::atomic<bool> running{true};
    std::atomic<bool> paused{false};
    std::promise<void> stop_promise;
    std::future<void> stop_future = stop_promise.get_future();

    // ---- Interaction thread ----
    std::thread interaction_thread(std::ref(InteractionManager::instance()));

    // ---- Move + detect thread ----
    std::thread move_thread([&]() {
        while (running) {
            if (paused) {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            // Двигаем NPC
            for (auto& npc : npcs) {
                if (!npc->is_alive()) continue;
                int d = npc->get_move_distance();
                npc->move(
                    std::rand() % (2*d+1) - d,
                    std::rand() % (2*d+1) - d,
                    MAP_X, MAP_Y
                );
            }

            // Grid и взаимодействия
            std::unordered_map<std::pair<int,int>, std::vector<std::shared_ptr<NPC>>, PairHash> grid;
            for (auto& npc : npcs) {
                if (!npc->is_alive()) continue;
                grid[npc->grid_cell].push_back(npc);
            }

            for (const auto& [cell, cell_npcs] : grid) {
                // Внутри клетки
                for (size_t i=0; i<cell_npcs.size(); ++i)
                    for (size_t j=i+1; j<cell_npcs.size(); ++j) {
                        int maxDist = std::max(cell_npcs[i]->get_interaction_distance(),
                                               cell_npcs[j]->get_interaction_distance());
                        if (cell_npcs[i]->is_close(cell_npcs[j], maxDist))
                            InteractionManager::instance().push({cell_npcs[i], cell_npcs[j]});
                    }

                // Соседние клетки (4 направления)
                std::array<std::pair<int,int>,4> neighbors = {{{1,0},{1,1},{0,1},{-1,1}}};
                for (auto offset : neighbors) {
                    auto neigh_cell = std::pair<int,int>{cell.first+offset.first, cell.second+offset.second};
                    auto it = grid.find(neigh_cell);
                    if (it != grid.end()) {
                        for (auto& npc1 : cell_npcs)
                            for (auto& npc2 : it->second) {
                                int maxDist = std::max(npc1->get_interaction_distance(), npc2->get_interaction_distance());
                                if (npc1->is_close(npc2, maxDist))
                                    InteractionManager::instance().push({npc1, npc2});
                            }
                    }
                }
            }

            std::this_thread::sleep_for(500ms);
        }
    });

#ifndef PIXELRPG_HEADLESS
    // ---- Visual wrapper в main thread ----
    VisualWrapper visualWrapper(800, 600);

    if (!visualWrapper.initialize()) {
        std::cerr << "Failed to initialize VisualWrapper\n";
        running = false;
    } else { 
        visualWrapper.setNPCs(npcs);
        visualWrapper.setPausedPtr(&paused);
        visualWrapper.setRunningPtr(&running);
        visualWrapper.setEffectsCVPtr(
            InteractionManager::instance().getEffectsCV(),
            InteractionManager::instance().getCVMtx()
        );
        visualWrapper.run(); // блокирует main thread, безопасно для SFML
    }
#endif

    // ---- Game shutdown ----
    running = false;
    stop_promise.set_value();

    if (stop_future.wait_for(5s) == std::future_status::timeout)
        std::cerr << "Threads timeout on shutdown" << std::endl;

    move_thread.join();
    InteractionManager::instance().stop();
    interaction_thread.join();

    print_survivors(npcs);
    return 0;
}
