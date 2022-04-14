#ifndef OPENMW_COMPONENTS_SETTINGS_SHADERMANAGER_H
#define OPENMW_COMPONENTS_SETTINGS_SHADERMANAGER_H

#include <unordered_map>
#include <filesystem>
#include <optional>
#include <fstream>

#include <yaml-cpp/yaml.h>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

#include <components/serialization/osgyaml.hpp>
#include <components/debug/debuglog.hpp>

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
            ShaderManager::mData[tname][uname] = value;
        }

        template <class T>
        static std::optional<T> getValue(const std::string& tname, const std::string& uname)
        {
            if (!ShaderManager::mData[tname][uname])
                return std::nullopt;
            return ShaderManager::mData[tname][uname].as<T>();
        }

        void load(const std::string& userConfigPath)
        {
            auto path = ShaderManager::getPath(userConfigPath);
            Log(Debug::Info) << "Loading shader settings file: " << path;

            if (!std::filesystem::exists(path))
            {
                std::ofstream fout(path);
            }

            mData = YAML::LoadFile(path);
        }

        void save(const std::string& userConfigPath)
        {
            auto path = ShaderManager::getPath(userConfigPath);
            Log(Debug::Info) << "Updating shader settings file: " << path;

            std::ofstream fout(path);
            fout << mData;
        }

    private:

        inline static std::filesystem::path getPath(const std::string& userConfigPath)
        {
            return std::filesystem::path(userConfigPath) / "shaders.yaml";
        }

        inline static YAML::Node mData;

        inline static Mode mMode = Mode::Normal;
    };
}

#endif
