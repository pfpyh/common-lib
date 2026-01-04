#include <fstream>

#include "common/lifecycle/Application.h"

class TestApplication : public ::common::lifecycle::Application
{
public :
    auto bootup() -> std::shared_ptr<std::future<void>>
    {
        std::ofstream file("bootup.txt");
        if (file.is_open()) {
            file << "done!";
            file.close();
        }
        return nullptr;
    }

    auto shutdown() -> std::shared_ptr<std::future<void>>
    {
        std::ofstream file("shutdown.txt");
        if (file.is_open()) {
            file << "done!";
            file.close();
        }
        return nullptr;
    }
};

auto main() -> int32_t
{
    TestApplication app;
    app.run();
}