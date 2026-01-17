#include "include/npc.h"
#include "include/game_utils.h"
#include "include/visual_wrapper.h"

#include <memory>
#include <array>
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <iostream>

using namespace std::chrono_literals;

static bool hasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == flag) return true;
    }
    return false;
}

int main(int argc, char** argv) {
    const bool headless = hasFlag(argc, argv, "--headless");

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
        switch (t) {
            case NPCType::Bear:     name = "Bear_" + std::to_string(i + 1);     break;
            case NPCType::Dragon:   name = "Dragon_" + std::to_string(i + 1);   break;
            case NPCType::Druid:    name = "Druid_" + std::to_string(i + 1);    break;
            case NPCType::Orc:      name = "Orc_" + std::to_string(i + 1);      break;
            case NPCType::Squirrel: name = "Squirrel_" + std::to_string(i + 1); break;
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

#ifndef PIXELRPG_HEADLESS
    // IMPORTANT (macOS): VisualWrapper / SFML window MUST be created on the main thread.
    std::unique_ptr<VisualWrapper> visualWrapper;
    if (!headless) {
        visualWrapper = std::make_unique<VisualWrapper>(800, 600);
        if (!visualWrapper->initialize()) {
            std::cerr << "Failed to initialize visual wrapper\n";
            return 1;
        }
        visualWrapper->setNPCs(npcs);
        visualWrapper->setPausedPtr(&paused);
        visualWrapper->setRunningPtr(&running);
        visualWrapper->setEffectsCVPtr(
            InteractionManager::instance().getEffectsCV(),
            InteractionManager::instance().getCVMtx()
        );
    }
#endif

    // ---- Interaction thread ----
    std::thread interaction_thread(std::ref(InteractionManager::instance()));

    // ---- Move + detect thread ----
    std::thread move_thread([&]() {
        while (running) {
            if (paused) {
                std::this_thread::sleep_for(100ms);
                continue;
            }

            // Move NPCs
            for (auto& npc : npcs) {
                if (!npc->is_alive()) continue;
                int d = npc->get_move_distance();
                npc->move(
                    std::rand() % (2 * d + 1) - d,
                    std::rand() % (2 * d + 1) - d,
                    MAP_X, MAP_Y
                );
            }

            // Grid and interactions
            std::unordered_map<std::pair<int, int>, std::vector<std::shared_ptr<NPC>>, PairHash> grid;
            for (auto& npc : npcs) {
                if (!npc->is_alive()) continue;
                grid[npc->grid_cell].push_back(npc);
            }

            for (const auto& [cell, cell_npcs] : grid) {
                // Inside the same cell
                for (size_t i = 0; i < cell_npcs.size(); ++i)
                    for (size_t j = i + 1; j < cell_npcs.size(); ++j) {
                        int maxDist = std::max(cell_npcs[i]->get_interaction_distance(),
                                               cell_npcs[j]->get_interaction_distance());
                        if (cell_npcs[i]->is_close(cell_npcs[j], maxDist))
                            InteractionManager::instance().push({cell_npcs[i], cell_npcs[j]});
                    }

                // Neighbor cells (4 directions)
                std::array<std::pair<int, int>, 4> neighbors = {{{1, 0}, {1, 1}, {0, 1}, {-1, 1}}};
                for (auto offset : neighbors) {
                    auto neigh_cell = std::pair<int, int>{cell.first + offset.first, cell.second + offset.second};
                    auto it = grid.find(neigh_cell);
                    if (it != grid.end()) {
                        for (auto& npc1 : cell_npcs)
                            for (auto& npc2 : it->second) {
                                int maxDist = std::max(npc1->get_interaction_distance(),
                                                       npc2->get_interaction_distance());
                                if (npc1->is_close(npc2, maxDist))
                                    InteractionManager::instance().push({npc1, npc2});
                            }
                    }
                }
            }

            std::this_thread::sleep_for(500ms);
        }
    });

    std::thread timer_thread([&]() {
        auto start = std::chrono::steady_clock::now();
        const auto duration = 30s;

        while (running) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= duration) {
                std::cout << "[DEBUG] Timer expired (30s). Shutting down...\n";
                running = false;
                break;
            }
            std::this_thread::sleep_for(100ms);
        }
    });

#ifndef PIXELRPG_HEADLESS
    // ---- Visual loop on MAIN thread (macOS requirement) ----
    if (!headless && visualWrapper) {
        visualWrapper->run();   // blocks until window closed
        running = false;        // ensure workers stop after closing window
    }
#endif

    // ---- Shutdown ----
    running = false;

    timer_thread.join();
    move_thread.join();

    InteractionManager::instance().stop();
    interaction_thread.join();

    print_survivors(npcs);
    return 0;
}
