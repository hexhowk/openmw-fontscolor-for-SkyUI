#include "technique.hpp"

#include <array>
#include <string>

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/CullStack>

#include <SDL_opengl_glext.h>

#include <components/vfs/manager.hpp>

#include <components/misc/stringops.hpp>

#include <components/sceneutil/util.hpp>

#include <components/resource/imagemanager.hpp>

#include <components/debug/debuglog.hpp>

#include "parse_constants.hpp"

namespace
{
    struct ProxyTextureData
    {
        osg::Texture::WrapMode wrap_s = osg::Texture::CLAMP_TO_EDGE;
        osg::Texture::WrapMode wrap_t = osg::Texture::CLAMP_TO_EDGE;
        osg::Texture::WrapMode wrap_r = osg::Texture::CLAMP_TO_EDGE;
        osg::Texture::FilterMode min_filter = osg::Texture::LINEAR_MIPMAP_LINEAR;
        osg::Texture::FilterMode mag_filter =osg::Texture::LINEAR;
    };
}

namespace fx
{
    Technique::Technique(const VFS::Manager& vfs, Resource::ImageManager& imageManager, const std::string& name, int width, int height, bool ubo)
        : mName(name)
        , mFileName((std::filesystem::path(Technique::sSubdir) / (mName + Technique::sExt)).string())
        , mLastModificationTime(std::filesystem::file_time_type())
        , mWidth(width)
        , mHeight(height)
        , mVFS(vfs)
        , mImageManager(imageManager)
        , mUBO(ubo)
    {
        clear();
    }

    void Technique::clear()
    {
        mTextures.clear();
        mStatus = Status::Uncompiled;
        mDirty = false;
        mValid = false;
        mHDR = false;
        mEnabled = true;
        mPassMap.clear();
        mPasses.clear();
        mPassKeys.clear();
        mDefinedUniforms.clear();
        mRenderTargets.clear();
        mMainTemplate = nullptr;
        mLastAppliedType = Pass::Type::None;
        mFlags = 0;
        mShared.clear();
        mAuthor = {};
        mDescription = {};
        mVersion = {};
        mGLSLExtensions.clear();
        mGLSLVersion = mUBO ? "330" : "120" ;
        mGLSLProfile.clear();
    }

    std::string Technique::getBlockWithLineDirective()
    {
        auto block = mLexer->getLastJumpBlock();
        std::string content = std::string(block.content);

        content = "\n#line " + std::to_string(block.line + 1) + "\n"  + std::string(block.content) + "\n";
        return content;
    }

    Technique::UniformMap::iterator Technique::findUniform(const std::string& name)
    {
        return std::find_if(mDefinedUniforms.begin(), mDefinedUniforms.end(), [&name](const auto& uniform)
        {
            return uniform->mName == name;
        });
    }

    bool Technique::compile()
    {
        clear();

        if (!mVFS.exists(mFileName))
        {
            Log(Debug::Error) << "Could not load technique, file does not exist '" << mFileName << "'";

            mStatus = Status::File_Not_exists;
            return false;
        }

        try
        {
            std::string source(std::istreambuf_iterator<char>(*mVFS.get(getFileName())), {});

            parse(source);

            if (mPassKeys.empty())
                error("no pass list found, ensure you define one in a 'technique' block");

            int swaps = 0;

            for (auto name : mPassKeys)
            {
                auto it = mPassMap.find(name);

                if (it == mPassMap.end())
                    error(Misc::StringUtils::format("pass '%s' was found in the pass list, but there was no matching 'fragment', 'vertex' or 'compute' block", std::string(name)));

                if (mLastAppliedType != Pass::Type::None && mLastAppliedType != it->second->mType)
                {
                    swaps++;
                    if (swaps == 2)
                        Log(Debug::Warning) << "compute and pixel shaders are being swapped multiple times in shader chain, this can lead to serious performance drain.";
                }
                else
                    mLastAppliedType = it->second->mType;

                it->second->compile(*this, mShared);

                if (!it->second->mTarget.empty())
                {
                    auto rtIt = mRenderTargets.find(it->second->mTarget);
                    if (rtIt == mRenderTargets.end())
                        error(Misc::StringUtils::format("target '%s' not defined", std::string(it->second->mTarget)));
                }

                mPasses.emplace_back(it->second);
            }

            if (mPasses.empty())
                error("invalid pass list, no passes defined for technique");

            mValid = true;
        }
        catch(const Lexer::LexerException& e)
        {
            clear();
            mStatus = Status::Parse_Error;

            mLastError = "Failed parsing technique '" + getName() + "' "  + e.what();;
            Log(Debug::Error) << mLastError;
        }

        return mValid;
    }

    std::string Technique::getName() const
    {
        return mName;
    }

    std::string Technique::getFileName() const
    {
        return mFileName;
    }

    void Technique::setLastModificationTime(std::filesystem::file_time_type timeStamp, bool dirty)
    {
        if (dirty && mLastModificationTime != timeStamp)
            mDirty = true;

        mLastModificationTime = timeStamp;
    }

    bool Technique::isDirty() const
    {
        return mDirty;
    }

    void Technique::setDirty(bool dirty)
    {
        mDirty = dirty;
    }

    bool Technique::isValid() const
    {
        return mValid;
    }

    [[noreturn]] void Technique::error(const std::string& msg)
    {
        mLexer->error(msg);
    }

    template<>
    void Technique::parseBlockImp<Lexer::Shared>()
    {
        if (!mLexer->jump())
            error(Misc::StringUtils::format("unterminated 'shared' block, expected closing brackets"));

        if (!mShared.empty())
            error("repeated 'shared' block, only one allowed per technique file");

        mShared = getBlockWithLineDirective();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Technique>()
    {
        if (!mPassKeys.empty())
            error("exactly one 'technique' block can appear per file");

        while (!isNext<Lexer::Close_bracket>() && !isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>();

            auto key = std::get<Lexer::Literal>(mToken).value;

            expect<Lexer::Equal>();

            if (key == "passes")
                mPassKeys = parseLiteralList<Lexer::Comma>();
            else if (key == "version")
                mVersion = parseString();
            else if (key == "description")
                mDescription = parseString();
            else if (key == "author")
                mAuthor = parseString();
            else if (key == "glsl_version")
                mGLSLVersion = std::to_string(parseInteger());
            else if (key == "flags")
                mFlags = parseFlags();
            else if (key == "hdr")
                mHDR = parseBool();
            else if (key == "glsl_profile")
            {
                expect<Lexer::String>();
                mGLSLProfile = std::string(std::get<Lexer::String>(mToken).value);
            }
            else if (key == "glsl_extensions")
            {
                expect<Lexer::Open_Parenthesis>();
                for (const auto& ext : parseLiteralList<Lexer::Comma>())
                    mGLSLExtensions.emplace(ext);
                expect<Lexer::Close_Parenthesis>();
            }
            else
                error(Misc::StringUtils::format("unexpected key '%s'", std::string{key}));

            expect<Lexer::SemiColon>();
        }

        if (mPassKeys.empty())
            error("pass list in 'technique' block cannot be empty.");
    }

    template<>
    void Technique::parseBlockImp<Lexer::Main_Pass>()
    {
        if (mMainTemplate)
            error("duplicate 'main_pass' block");

        if (mName != "main")
            error("'main_pass' block can only be defined in the 'main.omwfx' technique file");

        mMainTemplate = new osg::Texture2D;

        mMainTemplate->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mMainTemplate->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        while (!isNext<Lexer::Close_bracket>() && !isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>();

            auto key = std::get<Lexer::Literal>(mToken).value;

            expect<Lexer::Equal>();

            if (key == "wrap_s")
                mMainTemplate->setWrap(osg::Texture::WRAP_S, parseWrapMode());
            else if (key == "wrap_t")
                mMainTemplate->setWrap(osg::Texture::WRAP_T, parseWrapMode());
            // Skip depth attachments for main scene, as some engine settings rely on specific depth formats.
            // Allowing this to be overriden will cause confusion.
            else if (key == "internal_format")
                mMainTemplate->setInternalFormat(parseInternalFormat());
            else if (key == "source_type")
                mMainTemplate->setSourceType(parseSourceType());
            else if (key == "source_format")
                mMainTemplate->setSourceFormat(parseSourceFormat());
            else
                error(Misc::StringUtils::format("unexpected key '%s'", std::string(key)));

            expect<Lexer::SemiColon>();
        }
    }

    template<>
    void Technique::parseBlockImp<Lexer::Render_Target>()
    {
        if (mRenderTargets.count(mBlockName))
            error(Misc::StringUtils::format("redeclaration of render target '%s'", std::string(mBlockName)));

        fx::Types::RenderTarget rt;
        rt.mTarget->setTextureSize(mWidth, mHeight);
        rt.mTarget->setSourceFormat(GL_RGB);
        rt.mTarget->setInternalFormat(GL_RGB);
        rt.mTarget->setSourceType(GL_UNSIGNED_BYTE);

        while (!isNext<Lexer::Close_bracket>() && !isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>();

            auto key = std::get<Lexer::Literal>(mToken).value;

            expect<Lexer::Equal>();

            if (key == "min_filter")
                rt.mTarget->setFilter(osg::Texture2D::MIN_FILTER, parseFilterMode());
            else if (key == "mag_filter")
                rt.mTarget->setFilter(osg::Texture2D::MAG_FILTER, parseFilterMode());
            else if (key == "wrap_s")
                rt.mTarget->setWrap(osg::Texture2D::WRAP_S, parseWrapMode());
            else if (key == "wrap_t")
                rt.mTarget->setWrap(osg::Texture2D::WRAP_T, parseWrapMode());
            else if (key == "width_ratio")
                rt.mSize.mWidthRatio = parseFloat();
            else if (key == "height_ratio")
                rt.mSize.mHeightRatio = parseFloat();
            else if (key == "width")
                rt.mSize.mWidth = parseInteger();
            else if (key == "height")
                rt.mSize.mHeight = parseInteger();
            else if (key == "internal_format")
                rt.mTarget->setInternalFormat(parseInternalFormat());
            else if (key == "source_type")
                rt.mTarget->setSourceType(parseSourceType());
            else if (key == "source_format")
                rt.mTarget->setSourceFormat(parseSourceFormat());
            else if (key == "mipmaps")
                rt.mMipMap = parseBool();
            else
                error(Misc::StringUtils::format("unexpected key '%s'", std::string(key)));

            expect<Lexer::SemiColon>();
        }

        mRenderTargets.emplace(mBlockName, std::move(rt));
    }

    template<>
    void Technique::parseBlockImp<Lexer::Vertex>()
    {
        if (!mLexer->jump())
            error(Misc::StringUtils::format("unterminated 'vertex' block, expected closing brackets"));

        auto& pass = mPassMap[mBlockName];

        if (!pass)
            pass = std::make_shared<fx::Pass>();

        pass->mName = mBlockName;

        if (pass->mCompute)
            error(Misc::StringUtils::format("'compute' block already defined. Usage is ambiguous."));
        else if (!pass->mVertex)
            pass->mVertex = new osg::Shader(osg::Shader::VERTEX, getBlockWithLineDirective());
        else
            error(Misc::StringUtils::format("duplicate vertex shader for block '%s'", std::string(mBlockName)));

        pass->mType = Pass::Type::Pixel;
    }

    template<>
    void Technique::parseBlockImp<Lexer::Fragment>()
    {
        if (!mLexer->jump())
            error(Misc::StringUtils::format("unterminated 'fragment' block, expected closing brackets"));

        auto& pass = mPassMap[mBlockName];

        if (!pass)
            pass = std::make_shared<fx::Pass>();

        pass->mUBO = mUBO;
        pass->mName = mBlockName;

        if (pass->mCompute)
            error(Misc::StringUtils::format("'compute' block already defined. Usage is ambiguous."));
        else if (!pass->mFragment)
            pass->mFragment = new osg::Shader(osg::Shader::FRAGMENT, getBlockWithLineDirective());
        else
            error(Misc::StringUtils::format("duplicate vertex shader for block '%s'", std::string(mBlockName)));

        pass->mType = Pass::Type::Pixel;
    }

    template<>
    void Technique::parseBlockImp<Lexer::Compute>()
    {
        if (!mLexer->jump())
            error(Misc::StringUtils::format("unterminated 'compute' block, expected closing brackets"));

        auto& pass = mPassMap[mBlockName];

        if (!pass)
            pass = std::make_shared<fx::Pass>();

        pass->mName = mBlockName;

        if (pass->mFragment)
            error(Misc::StringUtils::format("'fragment' block already defined. Usage is ambiguous."));
        else if (pass->mVertex)
            error(Misc::StringUtils::format("'vertex' block already defined. Usage is ambiguous."));
        else if (!pass->mFragment)
            pass->mCompute = new osg::Shader(osg::Shader::COMPUTE, getBlockWithLineDirective());
        else
            error(Misc::StringUtils::format("duplicate vertex shader for block '%s'", std::string(mBlockName)));

        pass->mType = Pass::Type::Compute;
    }

    template <class T>
    void Technique::parseSampler()
    {
        if (findUniform(std::string(mBlockName)) != mDefinedUniforms.end())
            error(Misc::StringUtils::format("redeclaration of uniform '%s'", std::string(mBlockName)));

        ProxyTextureData proxy;
        osg::ref_ptr<osg::Texture> sampler;

        constexpr bool is1D = std::is_same_v<Lexer::Sampler_1D, T>;
        constexpr bool is3D = std::is_same_v<Lexer::Sampler_3D, T>;

        Types::SamplerType type;

        while (!isNext<Lexer::Close_bracket>() && !isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>();

            auto key = asLiteral();

            expect<Lexer::Equal>();

            if (!is1D && key == "min_filter")
                proxy.min_filter = parseFilterMode();
            else if (!is1D && key == "mag_filter")
                proxy.mag_filter = parseFilterMode();
            else if (key == "wrap_s")
                proxy.wrap_s = parseWrapMode();
            else if (key == "wrap_t")
                proxy.wrap_t = parseWrapMode();
            else if (is3D && key == "wrap_r")
                proxy.wrap_r = parseWrapMode();
            else if (key == "source")
            {
                expect<Lexer::String>();
                auto image = mImageManager.getImage(std::string{std::get<Lexer::String>(mToken).value});
                if constexpr (is1D)
                {
                    type = Types::SamplerType::Texture_1D;
                    sampler = new osg::Texture1D(image);
                }
                else if constexpr (is3D)
                {
                    type = Types::SamplerType::Texture_3D;
                    sampler = new osg::Texture3D(image);
                }
                else
                {
                    type = Types::SamplerType::Texture_2D;
                    sampler = new osg::Texture2D(image);
                }
            }
            else
                error(Misc::StringUtils::format("unexpected key '%s'", std::string{key}));

            expect<Lexer::SemiColon>();
        }
        if (!sampler)
            error(Misc::StringUtils::format("%s '%s' requires a filename", std::string(T::repr), std::string{mBlockName}));

        if (!is1D)
        {
            sampler->setFilter(osg::Texture::MIN_FILTER, proxy.min_filter);
            sampler->setFilter(osg::Texture::MAG_FILTER, proxy.mag_filter);
        }
        if (is3D)
            sampler->setWrap(osg::Texture::WRAP_R, proxy.wrap_r);
        sampler->setWrap(osg::Texture::WRAP_S, proxy.wrap_s);
        sampler->setWrap(osg::Texture::WRAP_T, proxy.wrap_t);
        sampler->setName(std::string{mBlockName});

        mTextures.emplace_back(sampler);

        std::shared_ptr<Types::UniformBase> uniform = std::make_shared<Types::UniformBase>();
        uniform->mSamplerType = type;
        uniform->mName = std::string(mBlockName);
        mDefinedUniforms.emplace_back(std::move(uniform));
    }

    template <class SrcT, class T>
    void Technique::parseUniform()
    {
        if (findUniform(std::string(mBlockName)) != mDefinedUniforms.end())
            error(Misc::StringUtils::format("redeclaration of uniform '%s'", std::string(mBlockName)));

        std::shared_ptr<Types::UniformBase> uniform = std::make_shared<Types::UniformBase>();
        Types::Uniform<SrcT> data;

        while (!isNext<Lexer::Close_bracket>() && !isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>();

            auto key = asLiteral();

            expect<Lexer::Equal>("error parsing config for uniform block");

            constexpr bool isVec = std::is_same_v<osg::Vec2f, SrcT> || std::is_same_v<osg::Vec3f, SrcT> || std::is_same_v<osg::Vec4f, SrcT>;
            constexpr bool isFloat = std::is_same_v<float, SrcT>;
            constexpr bool isInt = std::is_same_v<int, SrcT>;

            std::optional<double> step;

            if constexpr (isInt)
                step = 1.0;

            if (key == "default")
            {
                if constexpr (isVec)
                    data.mDefault = parseVec<SrcT, T>();
                else if constexpr (isFloat)
                    data.mDefault = parseFloat();
                else if constexpr (isInt)
                    data.mDefault = parseInteger();
                else
                    data.mDefault = parseBool();
            }
            else if (key == "min")
            {
                if constexpr (isVec)
                    data.mMin = parseVec<SrcT, T>();
                else if constexpr (isFloat)
                    data.mMin = parseFloat();
                else if constexpr (isInt)
                    data.mMin = parseInteger();
                else
                    data.mMin = parseBool();
            }
            else if (key == "max")
            {
                if constexpr (isVec)
                    data.mMax = parseVec<SrcT, T>();
                else if constexpr (isFloat)
                    data.mMax = parseFloat();
                else if constexpr (isInt)
                    data.mMax = parseInteger();
                else
                    data.mMax = parseBool();
            }
            else if (key == "step")
                step = parseFloat();
            else if (key == "static")
                uniform->mStatic = parseBool();
            else if (key == "description")
            {
                expect<Lexer::String>();
                uniform->mDescription = std::get<Lexer::String>(mToken).value;
            }
            else if (key == "header")
            {
                expect<Lexer::String>();
                uniform->mHeader = std::get<Lexer::String>(mToken).value;
            }
            else
                error(Misc::StringUtils::format("unexpected key '%s'", std::string{key}));

            if (step)
                uniform->mStep = step.value();

            expect<Lexer::SemiColon>();
        }

        uniform->mName = std::string(mBlockName);
        uniform->mData = data;
        uniform->mTechniqueName = mName;

        if (auto cached = Settings::ShaderManager::getValue<SrcT>(mName, uniform->mName))
            uniform->setValue<SrcT>(cached.value());

        mDefinedUniforms.emplace_back(std::move(uniform));
    }

    template<>
    void Technique::parseBlockImp<Lexer::Sampler_1D>()
    {
        parseSampler<Lexer::Sampler_1D>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Sampler_2D>()
    {
        parseSampler<Lexer::Sampler_2D>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Sampler_3D>()
    {
        parseSampler<Lexer::Sampler_3D>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Uniform_Bool>()
    {
        parseUniform<bool, bool>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Uniform_Float>()
    {
        parseUniform<float, float>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Uniform_Int>()
    {
        parseUniform<int, int>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Uniform_Vec2>()
    {
        parseUniform<osg::Vec2f, Lexer::Vec2>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Uniform_Vec3>()
    {
        parseUniform<osg::Vec3f, Lexer::Vec3>();
    }

    template<>
    void Technique::parseBlockImp<Lexer::Uniform_Vec4>()
    {
        parseUniform<osg::Vec4f, Lexer::Vec4>();
    }

    template<class T>
    void Technique::expect(const std::string& err)
    {
        mToken = mLexer->next();
        if (!std::holds_alternative<T>(mToken))
        {
            if (err.empty())
                error(Misc::StringUtils::format("Expected %s", std::string(T::repr)));
            else
                error(Misc::StringUtils::format("%s. Expected %s", err, std::string(T::repr)));
        }
    }

    template<class T, class T2>
    void Technique::expect(const std::string& err)
    {
        mToken = mLexer->next();
        if (!std::holds_alternative<T>(mToken) && !std::holds_alternative<T2>(mToken))
        {
            if (err.empty())
                error(Misc::StringUtils::format("%s. Expected %s or %s", err, std::string(T::repr), std::string(T2::repr)));
            else
                error(Misc::StringUtils::format("Expected %s or %s", std::string(T::repr), std::string(T2::repr)));
        }
    }

    template <class T>
    bool Technique::isNext()
    {
        return std::holds_alternative<T>(mLexer->peek());
    }

    void Technique::parse(std::string& buffer)
    {
        mBuffer = std::move(buffer);
        Misc::StringUtils::replaceAll(mBuffer, "\r\n", "\n");
        mLexer = std::make_unique<Lexer::Lexer>(mBuffer);

        for (auto t = mLexer->next(); !std::holds_alternative<Lexer::Eof>(t); t = mLexer->next())
        {
            std::visit([=](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<Lexer::Shared, T>)
                    parseBlock<Lexer::Shared>(false);
                else if constexpr (std::is_same_v<Lexer::Technique, T>)
                    parseBlock<Lexer::Technique>(false);
                else if constexpr (std::is_same_v<Lexer::Main_Pass, T>)
                    parseBlock<Lexer::Main_Pass>(false);
                else if constexpr (std::is_same_v<Lexer::Render_Target, T>)
                    parseBlock<Lexer::Render_Target>();
                else if constexpr (std::is_same_v<Lexer::Vertex, T>)
                    parseBlock<Lexer::Vertex>();
                else if constexpr (std::is_same_v<Lexer::Fragment, T>)
                    parseBlock<Lexer::Fragment>();
                else if constexpr (std::is_same_v<Lexer::Compute, T>)
                    parseBlock<Lexer::Compute>();
                else if constexpr (std::is_same_v<Lexer::Sampler_1D, T>)
                    parseBlock<Lexer::Sampler_1D>();
                else if constexpr (std::is_same_v<Lexer::Sampler_2D, T>)
                    parseBlock<Lexer::Sampler_2D>();
                else if constexpr (std::is_same_v<Lexer::Sampler_3D, T>)
                    parseBlock<Lexer::Sampler_3D>();
                else if constexpr (std::is_same_v<Lexer::Uniform_Bool, T>)
                    parseBlock<Lexer::Uniform_Bool>();
                else if constexpr (std::is_same_v<Lexer::Uniform_Float, T>)
                    parseBlock<Lexer::Uniform_Float>();
                else if constexpr (std::is_same_v<Lexer::Uniform_Int, T>)
                    parseBlock<Lexer::Uniform_Int>();
                else if constexpr (std::is_same_v<Lexer::Uniform_Vec2, T>)
                    parseBlock<Lexer::Uniform_Vec2>();
                else if constexpr (std::is_same_v<Lexer::Uniform_Vec3, T>)
                    parseBlock<Lexer::Uniform_Vec3>();
                else if constexpr (std::is_same_v<Lexer::Uniform_Vec4, T>)
                    parseBlock<Lexer::Uniform_Vec4>();
                else
                    error("invalid top level block");
            }
            , t);
        }
    }

    template <class T>
    void Technique::parseBlock(bool named)
    {
        mBlockName = T::repr;

        if (named)
        {
            expect<Lexer::Literal>("name is required for preceeding block decleration");

            mBlockName = std::get<Lexer::Literal>(mToken).value;

            if (isNext<Lexer::Open_Parenthesis>())
                parseBlockHeader();
        }

        expect<Lexer::Open_bracket>();

        parseBlockImp<T>();

        expect<Lexer::Close_bracket>();
    }

    template <class TDelimeter>
    std::vector<std::string_view> Technique::parseLiteralList()
    {
        std::vector<std::string_view> data;

        while (!isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>();

            data.emplace_back(std::get<Lexer::Literal>(mToken).value);

            if (!isNext<TDelimeter>())
                break;

            mLexer->next();
        }

        return data;
    }

    void Technique::parseBlockHeader()
    {
        expect<Lexer::Open_Parenthesis>();

        if (isNext<Lexer::Close_Parenthesis>())
        {
            mLexer->next();
            return;
        }

        auto& pass = mPassMap[mBlockName];

        if (!pass)
            pass = std::make_shared<fx::Pass>();

        bool clear = true;
        osg::Vec4f clearColor = {1,1,1,1};

        while (!isNext<Lexer::Eof>())
        {
            expect<Lexer::Literal>("invalid key in block header");

            std::string_view key = std::get<Lexer::Literal>(mToken).value;

            expect<Lexer::Equal>();

            if (key == "target")
            {
                expect<Lexer::Literal>();
                pass->mTarget = std::get<Lexer::Literal>(mToken).value;
            }
            else if (key == "blend")
            {
                expect<Lexer::Open_Parenthesis>();
                osg::BlendEquation::Equation blendEq = parseBlendEquation();
                expect<Lexer::Comma>();
                osg::BlendFunc::BlendFuncMode blendSrc = parseBlendFuncMode();
                expect<Lexer::Comma>();
                osg::BlendFunc::BlendFuncMode blendDest = parseBlendFuncMode();
                expect<Lexer::Close_Parenthesis>();

                pass->mBlendSource = blendSrc;
                pass->mBlendDest = blendDest;
                if (blendEq != osg::BlendEquation::FUNC_ADD)
                    pass->mBlendEq = blendEq;
            }
            else if (key == "clear")
                clear = parseBool();
            else if (key == "clear_color")
                clearColor = parseVec<osg::Vec4f, Lexer::Vec4>();
            else
                error(Misc::StringUtils::format("unrecognized key '%s' in block header", std::string(key)));

            mToken = mLexer->next();

            if (std::holds_alternative<Lexer::Comma>(mToken))
            {
                if (std::holds_alternative<Lexer::Close_Parenthesis>(mLexer->peek()))
                    error(Misc::StringUtils::format("leading comma in '%s' is not allowed", std::string(mBlockName)));
                else
                    continue;
            }

            if (std::holds_alternative<Lexer::Close_Parenthesis>(mToken))
                return;
        }

        if (clear)
            pass->mClearColor = clearColor;

        error("malformed block header");
    }

    std::string_view Technique::asLiteral() const
    {
        return std::get<Lexer::Literal>(mToken).value;
    }

    FlagsType Technique::parseFlags()
    {
        auto parseBit = [this] (std::string_view term) {
            for (const auto& [identifer, bit]: constants::TechniqueFlag)
            {
                if (Misc::StringUtils::ciEqual(term, identifer))
                    return bit;
            }
            error(Misc::StringUtils::format("unrecognized flag '%s'", std::string(term)));
        };

        FlagsType flag = 0;
        for (const auto& bit : parseLiteralList<Lexer::Comma>())
            flag |= parseBit(bit);

        return flag;
    }

    osg::Texture::FilterMode Technique::parseFilterMode()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::FilterMode)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized filter mode '%s'", std::string{asLiteral()}));
    }

    osg::Texture::WrapMode Technique::parseWrapMode()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::WrapMode)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized wrap mode '%s'", std::string{asLiteral()}));
    }

    int Technique::parseInternalFormat()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::InternalFormat)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized internal format '%s'", std::string{asLiteral()}));
    }

    int Technique::parseSourceType()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::SourceType)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized source type '%s'", std::string{asLiteral()}));
    }

    int Technique::parseSourceFormat()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::SourceFormat)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized source format '%s'", std::string{asLiteral()}));
    }

    osg::BlendEquation::Equation Technique::parseBlendEquation()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::BlendEquation)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized blend equation '%s'", std::string{asLiteral()}));
    }

    osg::BlendFunc::BlendFuncMode Technique::parseBlendFuncMode()
    {
        expect<Lexer::Literal>();

        for (const auto& [identifer, mode]: constants::BlendFunc)
        {
            if (asLiteral() == identifer)
                return mode;
        }

        error(Misc::StringUtils::format("unrecognized blend function '%s'", std::string{asLiteral()}));
    }

    bool Technique::parseBool()
    {
        mToken = mLexer->next();

        if (std::holds_alternative<Lexer::True>(mToken))
            return true;
        if (std::holds_alternative<Lexer::False>(mToken))
            return false;

        error("expected 'true' or 'false' as boolean value");
    }

    std::string_view Technique::parseString()
    {
        expect<Lexer::String>();

        return std::get<Lexer::String>(mToken).value;
    }

    float Technique::parseFloat()
    {
        mToken = mLexer->next();

        if (std::holds_alternative<Lexer::Float>(mToken))
            return std::get<Lexer::Float>(mToken).value;
        if (std::holds_alternative<Lexer::Integer>(mToken))
            return static_cast<float>(std::get<Lexer::Integer>(mToken).value);

        error("expected float value");
    }

    int Technique::parseInteger()
    {
        expect<Lexer::Integer>();

        return std::get<Lexer::Integer>(mToken).value;
    }

    template <class OSGVec, class T>
    OSGVec Technique::parseVec()
    {
        expect<T>();
        expect<Lexer::Open_Parenthesis>();

        OSGVec value;

        for (int i = 0; i < OSGVec::num_components; ++i)
        {
            value[i] = parseFloat();

            if (i < OSGVec::num_components - 1)
                expect<Lexer::Comma>();
        }

        expect<Lexer::Close_Parenthesis>("check definition of the vector");

        return value;
    }
}
