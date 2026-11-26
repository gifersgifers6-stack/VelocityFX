#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <cocos2d.h>
#include <algorithm>

using namespace geode::prelude;

namespace {
    bool g_panelOpen = false;

    bool fpsEnabled() {
        return Mod::get()->getSettingValue<bool>("fps-enabled");
    }
    int targetFPS() {
        return std::clamp(Mod::get()->getSettingValue<int>("target-fps"), 30, 360);
    }
    bool trailEnabled() {
        return Mod::get()->getSettingValue<bool>("motion-blur-enabled");
    }
    float trailIntensity() {
        return std::clamp(Mod::get()->getSettingValue<float>("blur-intensity"), 0.1f, 1.0f);
    }

    void applyFPSRequest() {
        // This requests an animation interval from Cocos. It cannot exceed
        // the display refresh rate or force the device/GD to sustain that FPS.
        if (fpsEnabled()) {
            cocos2d::CCApplication::sharedApplication()->setAnimationInterval(
                1.0 / static_cast<double>(targetFPS())
            );
        }
    }

    class VelocityPanel : public cocos2d::CCNode {
        cocos2d::CCLayerColor* m_panel = nullptr;
        cocos2d::CCLabelBMFont* m_status = nullptr;

        bool init() {
            if (!CCNode::init()) return false;
            auto win = CCDirector::sharedDirector()->getWinSize();

            auto tabLabel = CCLabelBMFont::create("VFX", "bigFont.fnt");
            tabLabel->setScale(0.48f);
            auto tab = CCMenuItemLabel::create(tabLabel, this, menu_selector(VelocityPanel::togglePanel));
            auto tabMenu = CCMenu::create();
            tabMenu->addChild(tab);
            tabMenu->setPosition({25.f, win.height * 0.5f});
            this->addChild(tabMenu, 100);

            m_panel = CCLayerColor::create({13, 20, 34, 238}, 190.f, 170.f);
            m_panel->setAnchorPoint({0.f, 0.5f});
            m_panel->setPosition({45.f, win.height * 0.5f});
            m_panel->setVisible(false);
            this->addChild(m_panel, 99);

            auto title = CCLabelBMFont::create("VELOCITYFX", "bigFont.fnt");
            title->setScale(0.48f);
            title->setColor({110, 190, 255});
            title->setPosition({95.f, 148.f});
            m_panel->addChild(title);

            m_status = CCLabelBMFont::create("", "bigFont.fnt");
            m_status->setScale(0.34f);
            m_status->setPosition({95.f, 125.f});
            m_panel->addChild(m_status);

            auto menu = CCMenu::create();
            menu->setPosition({0.f, 0.f});
            m_panel->addChild(menu);

            addButton(menu, "FPS: TOGGLE", {95.f, 96.f}, menu_selector(VelocityPanel::toggleFPS));
            addButton(menu, "FPS: 60 / 90 / 120 / 144 / 240", {95.f, 70.f}, menu_selector(VelocityPanel::cycleFPS));
            addButton(menu, "TRAIL: TOGGLE", {95.f, 43.f}, menu_selector(VelocityPanel::toggleTrail));
            addButton(menu, "TRAIL INTENSITY", {95.f, 19.f}, menu_selector(VelocityPanel::cycleIntensity));

            refresh();
            return true;
        }

        void addButton(CCMenu* menu, char const* text, CCPoint pos, SEL_MenuHandler callback) {
            auto label = CCLabelBMFont::create(text, "bigFont.fnt");
            label->setScale(0.30f);
            auto item = CCMenuItemLabel::create(label, this, callback);
            item->setPosition(pos);
            menu->addChild(item);
        }

        void refresh() {
            auto fpsText = fpsEnabled() ? fmt::format("{} FPS request", targetFPS()) : "FPS request OFF";
            auto trailText = trailEnabled() ? "Trail ON" : "Trail OFF";
            m_status->setString(fmt::format("{}  |  {}", fpsText, trailText).c_str());
        }

        void togglePanel(CCObject*) {
            g_panelOpen = !g_panelOpen;
            m_panel->setVisible(g_panelOpen);
        }
        void toggleFPS(CCObject*) {
            Mod::get()->setSettingValue("fps-enabled", !fpsEnabled());
            applyFPSRequest();
            refresh();
        }
        void cycleFPS(CCObject*) {
            int current = targetFPS();
            int next = current == 60 ? 90 : current == 90 ? 120 : current == 120 ? 144 : current == 144 ? 240 : 60;
            Mod::get()->setSettingValue("target-fps", next);
            applyFPSRequest();
            refresh();
        }
        void toggleTrail(CCObject*) {
            Mod::get()->setSettingValue("motion-blur-enabled", !trailEnabled());
            refresh();
        }
        void cycleIntensity(CCObject*) {
            float value = trailIntensity() + 0.2f;
            if (value > 1.01f) value = 0.2f;
            Mod::get()->setSettingValue("blur-intensity", value);
            refresh();
        }

    public:
        static VelocityPanel* create() {
            auto ret = new VelocityPanel();
            if (ret && ret->init()) {
                ret->autorelease();
                return ret;
            }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }
    };
}

// Attach the floating control panel to gameplay.
class $modify(VelocityFXPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (auto panel = VelocityPanel::create()) this->addChild(panel, 9999);
        return true;
    }
};

// A lightweight afterimage trail. This is not a full-screen shader blur.
// It copies the player's current sprite frame at short intervals and fades it.
class $modify(VelocityFXPlayer, PlayerObject) {
    struct Fields {
        float trailTimer = 0.f;
    };

    void update(float dt) {
        PlayerObject::update(dt);

        if (!trailEnabled() || !this->getParent() || this->m_isDead) return;

        m_fields->trailTimer += dt;
        if (m_fields->trailTimer < 0.045f) return;
        m_fields->trailTimer = 0.f;

        auto texture = this->getTexture();
        if (!texture) return;

        auto ghost = CCSprite::createWithTexture(texture, this->getTextureRect());
        if (!ghost) return;

        ghost->setPosition(this->getPosition());
        ghost->setAnchorPoint(this->getAnchorPoint());
        ghost->setScaleX(this->getScaleX());
        ghost->setScaleY(this->getScaleY());
        ghost->setRotation(this->getRotation());
        ghost->setFlipX(this->isFlipX());
        ghost->setFlipY(this->isFlipY());
        ghost->setColor({95, 175, 255});
        ghost->setOpacity(static_cast<GLubyte>(90.f * trailIntensity()));

        // Add behind the player in the same parent coordinate space.
        this->getParent()->addChild(ghost, this->getZOrder() - 1);
        ghost->runAction(CCSequence::create(
            CCFadeOut::create(0.18f),
            CCRemoveSelf::create(),
            nullptr
        ));
    }
};

$on_mod(Loaded) {
    applyFPSRequest();
    log::info("VelocityFX v0.2.0 loaded. Android64 target.");
}
