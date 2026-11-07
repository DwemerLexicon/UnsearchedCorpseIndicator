#include "IconMenu.h"

#include <algorithm>
#include <format>

#include "Conditions.h"
#include "Corpse.h"
#include "PCH.h"
#include "Settings.h"
#include "State.h"

namespace UCI {
    namespace {
        constexpr float kHeightOffset = 20.0f;
        constexpr float kNearDistance = 200.0f;
        constexpr float kFarDistance = 1000.0f;
        constexpr float kLargestScale = 100.0f;
        constexpr float kSmallestScale = 35.0f;
        constexpr float kHudHeight = 720.0f;  // HUD is laid out for 720p

        // same camera matrix and viewport the game uses for quest markers
        bool WorldToScreen(const RE::NiPoint3& a_pos, float& a_x, float& a_y) {
            static const auto worldToCam = reinterpret_cast<float (*)[4]>(RELOCATION_ID(519579, 406126).address());
            static const auto viewPort = reinterpret_cast<RE::NiRect<float>*>(RELOCATION_ID(519618, 406160).address());

            float depth = 0.0f;
            return RE::NiCamera::WorldPtToScreenPt3(worldToCam, *viewPort, a_pos, a_x, a_y, depth, 1e-5f) &&
                   depth >= 0.0f && a_x >= 0.0f && a_x <= 1.0f && a_y >= 0.0f && a_y <= 1.0f;
        }

        RE::NiPoint3 IconPosition(RE::TESObjectREFR* a_ref) {
            RE::NiPoint3 pos;
            if (a_ref->IsActor()) {
                pos = a_ref->GetLookingAtLocation();
            } else {
                pos = a_ref->GetPosition();
                if (const auto base = a_ref->GetBaseObject()) {
                    pos.z += static_cast<float>(base->boundData.boundMax.z) * a_ref->GetScale();
                }
            }
            pos.z += kHeightOffset;
            return pos;
        }

        float Scale(float a_cameraDistance) {
            if (Settings::fixedSize) {
                return kLargestScale * Settings::scale;
            }
            const float t = std::clamp((a_cameraDistance - kNearDistance) / (kFarDistance - kNearDistance), 0.0f, 1.0f);
            return (kLargestScale + t * (kSmallestScale - kLargestScale)) * Settings::scale;
        }

        float Alpha(float a_playerDistance) {
            const float start = Settings::fadeStartDistance;
            const float end = Settings::fadeMaxDistance;
            const float fade = end > start ? std::clamp((end - a_playerDistance) / (end - start), 0.0f, 1.0f)
                                           : (a_playerDistance <= end ? 1.0f : 0.0f);
            return fade * Settings::opacity;
        }

        bool ScreenAllowsIcons(RE::UI* a_ui) {
            return !a_ui->GameIsPaused() && a_ui->IsShowingMenus() && !a_ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) &&
                   !a_ui->IsMenuOpen(RE::CraftingMenu::MENU_NAME) && !a_ui->IsMenuOpen(RE::RaceSexMenu::MENU_NAME);
        }

        void Tint(RE::GFxValue& a_icon, const std::array<float, 3>& a_rgb) {
            RE::GRenderer::Cxform cxform;
            cxform.matrix[RE::GRenderer::Cxform::kR][RE::GRenderer::Cxform::kMult] = a_rgb[0];
            cxform.matrix[RE::GRenderer::Cxform::kG][RE::GRenderer::Cxform::kMult] = a_rgb[1];
            cxform.matrix[RE::GRenderer::Cxform::kB][RE::GRenderer::Cxform::kMult] = a_rgb[2];
            a_icon.SetCxform(cxform);
        }

        void Hide(RE::GFxValue& a_icon) {
            RE::GFxValue::DisplayInfo info;
            info.SetVisible(false);
            a_icon.SetDisplayInfo(info);
        }
    }

    void IconMenu::Register() { RE::UI::GetSingleton()->Register(MENU_NAME, Create); }

    void IconMenu::Show() {
        RE::UIMessageQueue::GetSingleton()->AddMessage(MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, nullptr);
    }

    IconMenu::IconMenu() {
        depthPriority = 0;
        menuFlags.set(Flag::kAlwaysOpen, Flag::kRequiresUpdate, Flag::kAllowSaving);
        inputContext = Context::kNone;

        const auto scaleform = RE::BSScaleformManager::GetSingleton();
        if (!scaleform->LoadMovieEx(this, "UnsearchedCorpsesIndicator/indicator",
                                    RE::GFxMovieView::ScaleModeType::kNoScale, [](RE::GFxMovieDef*) {})) {
            SKSE::log::error("Couldn't load Interface/UnsearchedCorpsesIndicator/indicator.swf");
            return;
        }
        uiMovie->SetViewAlignment(RE::GFxMovieView::AlignType::kTopLeft);
        uiMovie->SetMouseCursorCount(0);

        RE::GFxValue root;
        if (!uiMovie->GetVariable(&root, "_root")) {
            return;
        }

        _icons.reserve(Settings::maxIcons);
        for (std::uint32_t i = 0; i < Settings::maxIcons; ++i) {
            const auto name = std::format("icon{}", i);
            const RE::GFxValue args[3]{"Loot", name.c_str(), static_cast<double>(i + 1)};
            RE::GFxValue icon;
            if (!root.Invoke("attachMovie", &icon, args, 3) || !icon.IsDisplayObject()) {
                SKSE::log::error("Couldn't create the icon from indicator.swf");
                break;
            }
            if (Settings::color) {
                Tint(icon, *Settings::color);
            }
            Hide(icon);
            _icons.push_back(icon);
        }
    }

    void IconMenu::AdvanceMovie(float a_interval, std::uint32_t a_currentTime) {
        Update();
        IMenu::AdvanceMovie(a_interval, a_currentTime);
    }

    void IconMenu::Update() {
        std::size_t used = 0;

        const auto ui = RE::UI::GetSingleton();
        const auto player = RE::PlayerCharacter::GetSingleton();
        const auto camera = RE::Main::WorldRootCamera();

        if (uiMovie && !_icons.empty() && player && camera && State::IconsEnabled() && ScreenAllowsIcons(ui)) {
            const auto frame = uiMovie->GetVisibleFrameRect();
            const float pixelScale = (frame.bottom - frame.top) / kHudHeight;
            const auto playerPos = player->GetPosition();
            const auto cameraPos = camera->world.translate;

            RE::ProcessLists::GetSingleton()->ForEachHighActor([&](RE::Actor* a_actor) {
                if (used >= _icons.size()) {
                    return RE::BSContainer::ForEachResult::kStop;
                }
                if (!a_actor || !a_actor->IsDead()) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }

                RE::NiPointer<RE::TESObjectREFR> target;
                if (Corpse::HasAshPile(a_actor)) {
                    target = a_actor->extraList.GetAshPileRef().get();
                } else if (!a_actor->IsDisabled()) {
                    target = RE::NiPointer<RE::TESObjectREFR>(a_actor);
                }
                if (!target || !target->Get3D2() || !Conditions::ShowIcon(target.get())) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }

                const auto pos = IconPosition(target.get());

                const float alpha = Alpha(playerPos.GetDistance(pos));
                float x = 0.0f;
                float y = 0.0f;
                if (alpha <= 0.0f || !WorldToScreen(pos, x, y)) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }

                const float scale = Scale(cameraPos.GetDistance(pos)) * pixelScale;
                RE::GFxValue::DisplayInfo info;
                info.Set(frame.left + x * (frame.right - frame.left),
                         frame.top + (1.0f - y) * (frame.bottom - frame.top), 0.0, scale, scale, alpha, true);
                _icons[used++].SetDisplayInfo(info);
                return RE::BSContainer::ForEachResult::kContinue;
            });
        }

        for (std::size_t i = used; i < _icons.size(); ++i) {
            Hide(_icons[i]);
        }
    }
}
