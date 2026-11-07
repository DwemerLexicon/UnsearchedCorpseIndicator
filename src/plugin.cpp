#include "PCH.h"
#include "Events.h"
#include "IconMenu.h"
#include "Plugin.h"
#include "Serialization.h"
#include "Settings.h"

// also does SKSEPlugin_Query for 1.5.97
SKSEPluginInfo(
    .Version = UCI::Plugin::VERSION,
    .Name = UCI::Plugin::NAME,
    .Author = UCI::Plugin::AUTHOR,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary)

namespace {
    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            UCI::Events::Register();
            UCI::IconMenu::Register();
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
        SKSE::log::error("Couldn't register for SKSE messages");
        return false;
    }

    return true;
}
