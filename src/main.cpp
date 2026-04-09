#include <print>

#include "app.hpp"

int main(int argc, char** argv) {
    AppConfig config;

    for (int i = 1; i < argc; ++i) {
        if (strcmp("--sensitivity", argv[i]) == 0) {
            if (i+1 >= argc) {
                std::println("Use: --sensitivity <sensitivity>");
                return 1;
            }
            config.sensitivity = std::stof(argv[i+1]);
            ++i;
        }
    }

    App app(config);
    app.Run();

    return 0;
}