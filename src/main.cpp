
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
using namespace geode::prelude;

class $modify(MyMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Mods"), this,
            menu_selector(MyMenu::onOpen));
        auto menu = this->getChildByID("bottom-menu");
        menu->addChild(btn);
        menu->updateLayout();
        return true;
    }
    void onOpen(CCObject*) {
        FLAlertLayer::create("My Menu", "Hello!", "OK")->show();
    }
};
