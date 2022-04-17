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
    /*
     * ShaderManager manages the shader.yaml file which is auto-generated next to the settings.cfg if it does not exist.
     * This YAML file is simply a mapping of technique name to a list of uniforms and their values.
     *
     * config:
     *   TECHNIQUE:
     *    MY_FLOAT: 10.34
     *    MY_VEC2: [0.23, 0.34]
     *   TECHNIQUE2:
     *    MY_VEC3: [0.22, 0.33, 0.20]
    */
    class ShaderManager
    {
    public:

        enum class Mode
        {
            Normal,
            Debug
        };

        ShaderManager() = default;
        ShaderManager(ShaderManager const&) = delete;
        void operator=(ShaderManager const&) = delete;

        static ShaderManager& get()
        {
            static ShaderManager instance;
            return instance;
        }

        Mode getMode()
        {
            return mMode;
        }

        void setMode(Mode mode)
        {
            mMode = mode;
        }

        const YAML::Node& getRoot()
        {
            return mData;
        }

        template <class T>
        void setValue(const std::string& tname, const std::string& uname, const T& value)
        {
            if (mData.IsNull())
            {
                Log(Debug::Warning) << "Failed setting " << tname << ", " << uname << " : shader settings failed to load";
                return;
            }

            mData["config"][tname][uname] = value;
        }

        template <class T>
        std::optional<T> getValue(const std::string& tname, const std::string& uname)
        {
            try
            {
                auto value = mData["config"][tname][uname];

                if (!value)
                    return std::nullopt;

                return value.as<T>();
            }
            catch(const YAML::BadConversion& e)
            {
                Log(Debug::Warning) << "Failed retrieving " << tname << ", " << uname << " : mismatched types in config file.";
            }

            return std::nullopt;
        }

        void load(const std::string& path)
        {
            mData = YAML::Null;
            mPath = std::filesystem::path(path);

            Log(Debug::Info) << "Loading shader settings file: " << mPath;

            if (!std::filesystem::exists(mPath))
                std::ofstream fout(mPath);

            try
            {
                mData = YAML::LoadFile(mPath.string());
                mData.SetStyle(YAML::EmitterStyle::Block);

                if (!mData["config"])
                    mData["config"] = YAML::Node();
            }
            catch(const YAML::Exception& e)
            {
                Log(Debug::Error) << "Shader settings failed to load, " <<  e.msg;
            }
        }

        void save()
        {
            Log(Debug::Info) << "Saving shader settings file: " << mPath;

            YAML::Emitter out;
            out.SetMapFormat(YAML::Block);
            out << mData;

            std::ofstream fout(mPath.string());
            fout << out.c_str();

            if (!fout)
                Log(Debug::Error) << "Failed saving shader settings file: " << mPath;
        }

    private:
        std::filesystem::path mPath;
        YAML::Node mData;
        Mode mMode = Mode::Normal;
    };
}

#endif
