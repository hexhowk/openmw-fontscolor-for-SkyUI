#ifndef MWLUA_TYPES_H
#define MWLUA_TYPES_H

#include <sol/sol.hpp>

#include <components/esm/defs.hpp>
#include <components/esm/luascripts.hpp>

#include "../context.hpp"

namespace MWLua
{
    std::string_view getLuaObjectTypeName(ESM::RecNameInts type, std::string_view fallback = "Unknown");
    std::string_view getLuaObjectTypeName(const MWWorld::Ptr& ptr);
    const MWWorld::Ptr& verifyType(ESM::RecNameInts type, const MWWorld::Ptr& ptr);

    sol::table getTypeToPackageTable(lua_State* L);
    sol::table getPackageToTypeTable(lua_State* L);

    sol::table initTypesPackage(const Context& context);

    // used in initTypesPackage
    void addActivatorBindings(sol::table activator, const Context& context);
    void addBookBindings(sol::table book, const Context& context);
    void addContainerBindings(sol::table container, const Context& context);
    void addDoorBindings(sol::table door, const Context& context);
    void addActorBindings(sol::table actor, const Context& context);
    void addWeaponBindings(sol::table weapon, const Context& context);
    void addNpcBindings(sol::table npc, const Context& context);
    void addCreatureBindings(sol::table creature, const Context& context);
    void addLockpickBindings(sol::table lockpick, const Context& context);
    void addProbeBindings(sol::table probe, const Context& context);
    void addApparatusBindings(sol::table apparatus, const Context& context);
    void addRepairBindings(sol::table repair, const Context& context);
    void addMiscellaneousBindings(sol::table miscellaneous, const Context& context);
    void addPotionBindings(sol::table potion, const Context& context);
}

#endif // MWLUA_TYPES_H
