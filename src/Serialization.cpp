#include "PCH.h"
#include "Serialization.h"
#include "State.h"

namespace UCI::Serialization {
    namespace {
        constexpr std::uint32_t FourCC(char a, char b, char c, char d) {
            return static_cast<std::uint32_t>(a) |
                (static_cast<std::uint32_t>(b) << 8) |
                (static_cast<std::uint32_t>(c) << 16) |
                (static_cast<std::uint32_t>(d) << 24);
        }

        constexpr std::uint32_t kUniqueID = FourCC('U', 'C', 'I', 'S');
        constexpr std::uint32_t kSearchedRecord = FourCC('S', 'R', 'C', 'H');
        constexpr std::uint32_t kKilledByPlayerRecord = FourCC('P', 'K', 'I', 'L');  // added in 2.0.0
        constexpr std::uint32_t kRecordVersion = 1;

        // Layout: count, then that many form IDs. Not pruned against LookupByID(): corpses in
        // unloaded cells aren't in memory but still need their marks; deleted references are
        // already dropped by the form delete event.
        void WriteFormIDs(SKSE::SerializationInterface* a_intfc, std::uint32_t a_type, const std::vector<RE::FormID>& a_formIDs) {
            if (!a_intfc->OpenRecord(a_type, kRecordVersion)) return;
            if (!a_intfc->WriteRecordData(static_cast<std::uint32_t>(a_formIDs.size()))) return;
            for (const auto formID : a_formIDs) {
                if (!a_intfc->WriteRecordData(formID)) return;
            }
        }

        std::vector<RE::FormID> ReadFormIDs(SKSE::SerializationInterface* a_intfc) {
            std::vector<RE::FormID> formIDs;

            std::uint32_t count = 0;
            if (a_intfc->ReadRecordData(count) != sizeof(count)) return formIDs;

            for (std::uint32_t i = 0; i < count; ++i) {
                RE::FormID oldFormID = 0;
                if (a_intfc->ReadRecordData(oldFormID) != sizeof(oldFormID)) break;

                RE::FormID resolvedFormID = 0;
                if (a_intfc->ResolveFormID(oldFormID, resolvedFormID)) {
                    formIDs.push_back(resolvedFormID);
                }
            }
            return formIDs;
        }

        void Save(SKSE::SerializationInterface* a_intfc) {
            if (!a_intfc) return;
            WriteFormIDs(a_intfc, kSearchedRecord, State::GetSearched());
            WriteFormIDs(a_intfc, kKilledByPlayerRecord, State::GetKilledByPlayer());
        }

        void Load(SKSE::SerializationInterface* a_intfc) {
            State::Reset();
            if (!a_intfc) return;

            std::vector<RE::FormID> searched;
            std::vector<RE::FormID> killedByPlayer;

            std::uint32_t type = 0;
            std::uint32_t version = 0;
            std::uint32_t length = 0;
            while (a_intfc->GetNextRecordInfo(type, version, length)) {
                if (version != kRecordVersion) continue;

                if (type == kSearchedRecord) {
                    searched = ReadFormIDs(a_intfc);
                } else if (type == kKilledByPlayerRecord) {
                    killedByPlayer = ReadFormIDs(a_intfc);
                }
            }

            State::Restore(std::move(searched), std::move(killedByPlayer));
        }

        void Revert(SKSE::SerializationInterface*) {
            State::Reset();
        }
    }

    void Install() {
        const auto* intfc = SKSE::GetSerializationInterface();
        if (!intfc) return;

        intfc->SetUniqueID(kUniqueID);
        intfc->SetSaveCallback(Save);
        intfc->SetLoadCallback(Load);
        intfc->SetRevertCallback(Revert);
    }
}
