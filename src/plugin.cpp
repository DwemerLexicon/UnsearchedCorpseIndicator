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

namespace {
    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
        case SKSE::MessagingInterface::kPostLoad:
            // Every plugin is loaded now, so SIF can be found whatever order SKSE loaded us in.
            UCI::Conditions::Register();
            break;
        case SKSE::MessagingInterface::kDataLoaded:
            UCI::Events::Register();
            break;
        default:
            break;
        }
    }
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    UCI::Settings::Load();
    UCI::Serialization::Install();

    if (!SKSE::GetMessagingInterface()->RegisterListener(OnSKSEMessage)) {
        REX::ERROR("Couldn't register for SKSE messages");
        return false;
    }

    return true;
}
