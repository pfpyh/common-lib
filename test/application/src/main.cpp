#include "common/lifecycle/Application.h"

class TestApplication : public ::common::Application
{
public :
    auto bootup() -> ::common::Future
    {
        return nullptr;
    }

    auto shutdown() -> ::common::Future
    {
        return nullptr;
    }
};

auto main() -> int32_t
{
    TestApplication app;
    app.run();
}