#include "luabindings.hpp"

#include "../mwbase/environment.hpp"
#include "../mwrender/postprocessor.hpp"

#include "luamanagerimp.hpp"

namespace
{
    class ToggleShaderAction final : public MWLua::LuaManager::Action
    {
    public:
        ToggleShaderAction(LuaUtil::LuaState* state, std::shared_ptr<fx::Technique> shader, bool enable, std::optional<int> pos = std::nullopt)
            : MWLua::LuaManager::Action(state), mShader(std::move(shader)), mEnable(enable), mPos(pos) {}

        void apply(MWLua::WorldView&) const override
        {
            MWRender::PostProcessor* processor = MWBase::Environment::get().getWorld()->getPostProcessor();

            if (mEnable)
                processor->enableTechnique(mShader, mPos);
            else
                processor->disableTechnique(mShader);
        }

        std::string toString() const override
        {
            return std::string("ToggleShaderAction shader=") + (mShader ? mShader->getName() : "nil");
        }

    private:
        std::shared_ptr<fx::Technique> mShader;
        bool mEnable;
        std::optional<int> mPos;
    };

    template <class T>
    class SetUniformShaderAction final : public MWLua::LuaManager::Action
    {
    public:
        SetUniformShaderAction(LuaUtil::LuaState* state, std::shared_ptr<fx::Technique> shader, const std::string& name, const T& value)
            : MWLua::LuaManager::Action(state), mShader(std::move(shader)), mName(name), mValue(value) {}

        void apply(MWLua::WorldView&) const override
        {
            MWBase::Environment::get().getWorld()->getPostProcessor()->setUniform(mShader, mName, mValue);
        }

        std::string toString() const override
        {
            return  std::string("SetUniformShaderAction shader=") + (mShader ? mShader->getName() : "nil") +
                    std::string("uniform=") + (mShader ? mName : "nil");
        }

    private:
        std::shared_ptr<fx::Technique> mShader;
        std::string mName;
        T mValue;
    };

    class LoadShaderAction final : public MWLua::LuaManager::Action
    {
    public:
        LoadShaderAction(LuaUtil::LuaState* state, std::shared_ptr<fx::Technique> shader)
            : MWLua::LuaManager::Action(state), mShader(std::move(shader)) {}

        void apply(MWLua::WorldView&) const override
        {
            MWRender::PostProcessor* processor = MWBase::Environment::get().getWorld()->getPostProcessor();

            processor->addTemplate(std::move(mShader));
        }

        std::string toString() const override
        {
            return std::string("LoadShaderAction shader=") + (mShader ? mShader->getName() : "nil");
        }

    private:
        std::shared_ptr<fx::Technique> mShader;
    };
}

namespace MWLua
{
    struct Shader
    {
        std::shared_ptr<fx::Technique> mShader;

        Shader(std::shared_ptr<fx::Technique> shader) : mShader(std::move(shader)) {}

        std::string toString() const
        {
            if (!mShader)
                return "Shader(nil)";

            return Misc::StringUtils::format("Shader(%s, %s)", mShader->getName(), mShader->getFileName());
        }

        bool mQueuedAction = false;
    };

    sol::table initShaderPackage(const Context& context)
    {
        sol::table api(context.mLua->sol(), sol::create);

        sol::usertype<Shader> shader = context.mLua->sol().new_usertype<Shader>("Shader");
        shader[sol::meta_function::to_string] = [](const Shader& shader) { return shader.toString(); };

        shader["enable"] = [context](Shader& shader, sol::optional<int> optPos)
        {
            std::optional<int> pos = std::nullopt;
            if (optPos)
                pos = optPos.value();

            if (shader.mShader && shader.mShader->isValid())
                shader.mQueuedAction = true;
            context.mLuaManager->addAction(std::make_unique<ToggleShaderAction>(context.mLua, shader.mShader, true, pos));
        };

        shader["disable"] = [context](Shader& shader)
        {
            shader.mQueuedAction = false;
            context.mLuaManager->addAction(std::make_unique<ToggleShaderAction>(context.mLua, shader.mShader, false));
        };

        shader["isEnabled"] = [](const Shader& shader)
        {
            return shader.mQueuedAction;
        };

        shader["setUniform"] = sol::overload(
            [context] (const Shader& shader, const std::string& name, bool value) { context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<bool>>(context.mLua, shader.mShader, name, value)); },
            [context] (const Shader& shader, const std::string& name, float value) { context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<float>>(context.mLua, shader.mShader, name, value)); },
            [context] (const Shader& shader, const std::string& name, int value) { context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<int>>(context.mLua, shader.mShader, name, value)); },
            [context] (const Shader& shader, const std::string& name, const osg::Vec2f& value) { context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<osg::Vec2f>>(context.mLua, shader.mShader, name, value)); },
            [context] (const Shader& shader, const std::string& name, const osg::Vec3f& value) { context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<osg::Vec3f>>(context.mLua, shader.mShader, name, value)); },
            [context] (const Shader& shader, const std::string& name, const osg::Vec4f& value) { context.mLuaManager->addAction(std::make_unique<SetUniformShaderAction<osg::Vec4f>>(context.mLua, shader.mShader, name, value)); }
        );

        api["load"] = [context](const std::string& name)
        {
            auto processor = MWBase::Environment::get().getWorld()->getPostProcessor();
            auto technique = processor->loadTechnique(name, false);

            context.mLuaManager->addAction(std::make_unique<LoadShaderAction>(context.mLua, technique));
            return Shader(technique);
        };

        return LuaUtil::makeReadOnly(api);
    }

}
