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
    struct Mat4
    {
        using Value = osg::Matrixf;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "mat4";
    };

    struct Vec4
    {
        using Value = osg::Vec4f;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "vec4";
    };

    struct Vec3
    {
        using Value = osg::Vec3f;
        Value mValue;
        static constexpr size_t sAlign = 4 * sizeof(osg::Vec3f::value_type);
        static constexpr std::string_view sTypeName = "vec2";
    };

    struct Vec2
    {
        using Value = osg::Vec2f;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "vec2";
    };

    struct Float
    {
        using Value = float;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "float";
    };

    struct Int
    {
        using Value = std::int32_t;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "int";
    };

    struct UInt
    {
        using Value = std::uint32_t;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "uint";
    };

    struct Bool
    {
        using Value = std::int32_t;
        Value mValue;
        static constexpr size_t sAlign = sizeof(Value);
        static constexpr std::string_view sTypeName = "bool";
    };

    template <class... CArgs>
    class UBO
    {
    private:

        template<typename T, typename... Args>
        struct contains : std::bool_constant<(std::is_base_of_v<Args, T> || ...)> { };

        static_assert((contains<CArgs, Mat4, Vec4, Vec3, Vec2, Float, Int, UInt, Bool>() && ...));

        static constexpr size_t roundUpRemainder(size_t x, size_t multiple)
        {
            size_t remainder = x % multiple;
            if (remainder == 0)
                return 0;
            return multiple - remainder;
        }

    public:

        static constexpr size_t getGPUSize()
        {
            std::size_t size = 0;
            ((size += (sizeof(typename CArgs::Value) + UBO::roundUpRemainder(size, CArgs::sAlign))) , ...);
            return size;
        }

        using BufferType = std::array<char, UBO::getGPUSize()>;

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

        std::string getDefinition(const std::string& name) const
        {
            std::string structDefinition = "struct " + name + " {\n";
            std::apply([&] (const auto& ... v) { structDefinition += (makeStructField(v) + ...); }, mData);
            return structDefinition + "};";
        }

        void copyTo(BufferType& buffer) const
        {
            char* dst = buffer.data();
            size_t byteOffset = 0;

            const auto copy = [&] (const auto& v) {
                static_assert(std::is_standard_layout_v<std::decay_t<decltype(v.mValue)>>);
                using T = std::decay_t<decltype(v)>;

                size_t alignmentDelta = roundUpRemainder(byteOffset, T::sAlign);
                dst += alignmentDelta;
                std::memcpy(dst, &v.mValue, sizeof(v.mValue));

                byteOffset += sizeof(T::Value) + alignmentDelta;
                dst += sizeof(T::Value);
            };

            std::apply([&] (const auto& ... v) { (copy(v) , ...); }, mData);
        }

    private:
        template <class T>
        std::string makeStructField(const T& v) const
        {
            return "    " + std::string(T::sTypeName) + " " + std::string(v.sName) + ";\n";
        }

        std::tuple<CArgs...> mData;
    };
}

#endif
