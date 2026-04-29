#include "coah_engine/coah_engine.hpp"

int main() {
    CoahEngine engine;
    try { engine.run(); }
    catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}