#include <fstream>

#include "common/lifecycle/Application.h"

class TestApplication : public ::common::Application
{
public :
    auto bootup() -> ::common::Future
    {
        std::ofstream file("bootup.txt");
        if (file.is_open()) {
            file << "done!";
            file.close();
        }
        return nullptr;
    }

    auto shutdown() -> ::common::Future
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