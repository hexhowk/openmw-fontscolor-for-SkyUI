#ifndef COMPONENTS_STD140_UBO_H
#define COMPONENTS_STD140_UBO_H

#include <osg/Vec2f>
#include <osg/Vec4f>
#include <osg/Matrixf>

#include <cstdint>
#include <tuple>
#include <cstring>
#include <string>
#include <string_view>

namespace std140
{
    using std140_mat4 = osg::Matrixf;
    using std140_vec4 = osg::Vec4f;
    using std140_vec2 = osg::Vec2f;
    using std140_float = float;
    using std140_int = std::int32_t;
    using std140_bool = std::int32_t;

    template <class T>
    struct Field {
        using Value = T;
        T mValue;
    };

    template <class ... CArgs>
    class UBO
    {
    public:
        std::tuple<CArgs...> mData;

        using value_type = std::array<char, (sizeof(CArgs) + ...)>;

        template <class T>
        typename T::Value& get()
        {
            return std::get<T>(mData).mValue;
        }

        template <class T>
        const typename T::Value& get() const
        {
            return std::get<T>(mData).mValue;
        }

        std::string getDefinition(const std::string& name)
        {
            std::string structDefinition = "struct " + name + " {\n";
            std::apply([&] (const auto& ... v) { structDefinition += (makeStructField(v) + ...); }, mData);
            return structDefinition + "};";
        }

        template <class T>
        constexpr auto copy(T* dest) {
            std::array<char, (sizeof(CArgs) + ...)> buffer;
            char* dst = dest;
            const auto copy = [&] (const auto& v) {
                static_assert(std::is_standard_layout_v<std::decay_t<decltype(v)>>);
                std::memcpy(dst, &v, sizeof(v));
                dst += sizeof(v) /* + alignment*/;
            };
            std::apply([&] (const auto& ... v) { (copy(v) , ...); }, mData);
            return buffer;
        }

    private:
        template <class T>
        static constexpr std::string_view getTypeName()
        {
            if constexpr (std::is_same_v<T, std140_mat4>) {
                return "mat4";
            } else if constexpr (std::is_same_v<T, std140_vec4>) {
                return "vec4";
            } else if constexpr (std::is_same_v<T, std140_vec2>) {
                return "vec2";
            } else if constexpr (std::is_same_v<T, std140_float>) {
                return "float";
            } else if constexpr (std::is_same_v<T, std140_int>) {
                return "int";
            } else if constexpr (std::is_same_v<T, std140_bool>) {
                return "bool";
            } else {
                static_assert(std::is_void_v<T> && !std::is_void_v<T>, "Unsupported field type");
            }
        }

        template <class T>
        std::string makeStructField(const T& v)
        {
            return "    " + std::string(getTypeName<typename T::Value>()) + " " + std::string(v.sName) + ";\n";
        }
    };
}

#endif
