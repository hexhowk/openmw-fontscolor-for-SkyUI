#include <components/settings/shadermanager.hpp>

#include <fstream>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace Settings;

    struct ShaderSettingsTest : Test
    {
        template <typename F>
        void withSettingsFile( const std::string& content, F&& f)
        {
            const auto path = std::string(UnitTest::GetInstance()->current_test_info()->name()) + ".yaml";

            {
                std::ofstream stream;
                stream.open(path);
                stream << content;
                stream.close();
            }

            f(path);
        }
    };

    TEST_F(ShaderSettingsTest, load_empty_file)
    {
        const std::string content;

        withSettingsFile(content, [this] (const auto& path) {
            ShaderManager::get().load(path);

            EXPECT_EQ(ShaderManager::get().getRoot(), YAML::Node());
        });
    }
}