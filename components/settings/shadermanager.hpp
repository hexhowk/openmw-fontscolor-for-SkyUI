#ifndef OPENMW_COMPONENTS_SETTINGS_SHADERMANAGER_H
#define OPENMW_COMPONENTS_SETTINGS_SHADERMANAGER_H

#include <unordered_map>
#include <filesystem>
#include <optional>

#include <osg/io_utils>
#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/settings/categories.hpp>
#include <components/settings/parser.hpp>

namespace Settings
{
    class ShaderManager
    {
    public:

        enum class Mode
        {
            Normal,
            Debug
        };

        static Mode getMode()
        {
            return mMode;
        }

        static void setMode(Mode mode)
        {
            mMode = mode;
        }

        template <class T>
        static void setValue(const std::string& tname, const std::string& uname, const T& value)
        {
            std::string svalue = mStringFactory.clear() << value;
            ShaderManager::mSettings[{tname, uname}] = svalue;
        }

        template <class T>
        static std::optional<T> getValue(const std::string& tname, const std::string& uname)
        {
            auto it = mSettings.find({tname, uname});

            if (it == mSettings.end())
                return std::nullopt;

            std::istringstream ss(it->second);

            T value;
            ss >> value;

            if (ss.fail())
                return std::nullopt;

            return value;
        }

        void load(const std::string& userConfigPath)
        {
            SettingsFileParser parser;
            parser.loadSettingsFile(ShaderManager::getPath(userConfigPath).string(), ShaderManager::mSettings);
        }

        void save(const std::string& userConfigPath)
        {
            SettingsFileParser parser;
            parser.saveSettingsFile(ShaderManager::getPath(userConfigPath).string(), ShaderManager::mSettings, true, false);
        }

    private:

        inline static std::filesystem::path getPath(const std::string& userConfigPath)
        {
            return std::filesystem::path(userConfigPath) / "shader_settings.cfg";
        }

        inline static Mode mMode = Mode::Normal;

        inline static CategorySettingValueMap mSettings;

        inline static osg::MakeString mStringFactory;
    };
}

#endif
