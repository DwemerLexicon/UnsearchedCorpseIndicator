#include "PCH.h"
#include "Conditions.h"
#include "Events.h"
#include "Plugin.h"
#include "Serialization.h"
#include "Settings.h"

SKSE_PLUGIN_VERSION = [] {
    SKSE::PluginVersionData v;
    v.PluginVersion(UCI::Plugin::VERSION);
    v.PluginName(UCI::Plugin::NAME);
    v.AuthorName(UCI::Plugin::AUTHOR);
    v.UsesAddressLibrary();
    v.UsesUpdatedStructs();
    return v;
}();

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    UCI::Settings::Load();
    UCI::Events::Register();
    UCI::Serialization::Install();
    UCI::Conditions::Register();

    return true;
}
