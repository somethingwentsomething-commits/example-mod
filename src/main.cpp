#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <string>
#include <vector>

using namespace geode::prelude;

static bool g_novaOpen = false;
static CCNode* g_novaBubble = nullptr;

static const int NOVA_TABS = 7;
static const float NOVA_W = 440.f;
static const float NOVA_H = 280.f;

static const char* NOVA_TAB_NAMES[NOVA_TABS] = {
    "Overall", "Player", "Level", "Bypass", "Visual", "Creator", "Settings"
};

// Standard native Geometry Dash icons that are always loaded in memory
static const char* NOVA_TAB_ICONS[NOVA_TABS] = {
    "GJ_infoIcon_001.png",       // Overall
    "GJ_profileButton_001.png",  // Player
    "GJ_playBtn2_001.png",       // Level
    "GJ_lock_001.png",           // Bypass
    "GJ_colorBtn_001.png",       // Visual
    "GJ_creatorBtn_001.png",     // Creator
    "GJ_optionsBtn_001.png"      // Settings
};

class NovaMenu : public FLAlertLayer {
protected:
    CCNode* m_root = nullptr;
    CCNode* m_page = nullptr;
    CCMenu* m_tabMenu = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_tabButtons;
    std::vector<CCScale9Sprite*> m_tabBgs;
    std::vector<CCLabelBMFont*> m_tabLabels;
    std::vector<CCSprite*> m_tabIcons;
    int m_cur = -1;
    bool m_closing = false;

    bool initMenu() {
        if (!FLAlertLayer::init(180)) return false;
        m_noElasticity = true;
        g_novaOpen = true;

        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();
        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f, win.height / 2.f});
        m_mainLayer->addChild(m_root);

        // --- RobTop Native Main Window ---
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({NOVA_W, NOVA_H});
        m_root->addChild(bg);

        // Left sidebar panel (dark brown GD inset)
        auto sideBg = CCScale9Sprite::create("GJ_square02.png");
        sideBg->setContentSize({118.f, NOVA_H - 30.f});
        sideBg->setPosition({-150.f, -4.f});
        m_root->addChild(sideBg);

        // Main content card panel
        auto contentBg = CCScale9Sprite::create("GJ_square02.png");
        contentBg->setContentSize({286.f, NOVA_H - 60.f});
        contentBg->setPosition({60.f, -18.f});
        m_root->addChild(contentBg);

        // Title text (RobTop Gold Font)
        auto title = CCLabelBMFont::create("NOVA MENU", "goldFont.fnt");
        title->setScale(0.75f);
        title->setPosition({0.f, NOVA_H / 2.f - 22.f});
        m_root->addChild(title);

        // --- RobTop Native Close Button ---
        auto closeBtnSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(
            closeBtnSprite,
            this,
            menu_selector(NovaMenu::onClose)
        );

        auto closeMenu = CCMenu::create();
        closeMenu->setPosition({-NOVA_W / 2.f + 14.f, NOVA_H / 2.f - 14.f});
        closeMenu->addChild(closeBtn);
        m_root->addChild(closeMenu);

        // --- RobTop Side Tabs with Icons ---
        m_tabMenu = CCMenu::create();
        m_tabMenu->setPosition({0.f, 0.f});
        m_root->addChild(m_tabMenu);

        float startY = 96.f;
        float spacingY = 32.f;

        for (int i = 0; i < NOVA_TABS; i++) {
            auto container = CCNode::create();
            container->setContentSize({110.f, 28.f});

            // Tab Background (Scale9 to stretch nicely without distorting corners)
            auto sprBg = CCScale9Sprite::createWithSpriteFrameName("GJ_button_02.png");
            sprBg->setContentSize({110.f, 28.f});
            sprBg->setPosition({55.f, 14.f});
            m_tabBgs.push_back(sprBg);
            container->addChild(sprBg);

            // Tab Icon
            auto icon = CCSprite::createWithSpriteFrameName(NOVA_TAB_ICONS[i]);
            // Keep large icons small, but let small icons stay relatively unchanged
            float iconScale = 0.45f;
            if (i == 1 || i == 5 || i == 6) iconScale = 0.4f; 
            icon->setScale(iconScale);
            icon->setPosition({20.f, 14.f});
            icon->setColor({200, 200, 200});
            m_tabIcons.push_back(icon);
            container->addChild(icon);

            // Tab Text
            auto lbl = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
            lbl->limitLabelWidth(65.f, 0.45f, 0.1f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({38.f, 15.f});
            lbl->setColor({180, 180, 180});
            m_tabLabels.push_back(lbl);
            container->addChild(lbl);

            auto btn = CCMenuItemSpriteExtra::create(
                container,
                this,
                menu_selector(NovaMenu::onTabClicked)
            );
            btn->setTag(i);
            btn->setPosition({-150.f, startY - spacingY * i});
            m_tabMenu->addChild(btn);
            m_tabButtons.push_back(btn);
        }

        // Open animation (Elastic bounce)
        m_root->setScale(0.4f);
        m_root->runAction(CCEaseBackOut::create(CCScaleTo::create(0.25f, 1.f)));

        int last = Mod::get()->getSavedValue<int>("nova-last-tab", 0);
        selectTab(std::max(0, std::min(NOVA_TABS - 1, last)));
        return true;
    }

    void onTabClicked(CCObject* sender) {
        if (auto btn = typeinfo_cast<CCMenuItemSpriteExtra*>(sender)) {
            selectTab(btn->getTag());
        }
    }

    void selectTab(int i) {
        if (i == m_cur) return;
        m_cur = i;

        for (int k = 0; k < NOVA_TABS; k++) {
            bool on = (k == i);
            
            // Switch sprite texture to Green (GJ_button_01) when active, Blue/Gray (GJ_button_02) when inactive
            auto frameName = on ? "GJ_button_01.png" : "GJ_button_02.png";
            auto cache = CCSpriteFrameCache::sharedSpriteFrameCache();
            m_tabBgs[k]->setSpriteFrame(cache->spriteFrameByName(frameName));
            m_tabBgs[k]->setContentSize({110.f, 28.f}); // re-apply size after changing frame

            m_tabLabels[k]->setColor(on ? ccColor3B{255, 255, 255} : ccColor3B{180, 180, 180});
            m_tabIcons[k]->setColor(on ? ccColor3B{255, 255, 255} : ccColor3B{200, 200, 200});
        }

        buildPage(i);
        Mod::get()->setSavedValue<int>("nova-last-tab", i);
    }

    void buildPage(int i) {
        if (m_page) m_page->removeFromParent();
        m_page = CCNode::create();
        m_root->addChild(m_page);

        // Section Title
        auto title = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "goldFont.fnt");
        title->setScale(0.6f);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({-70.f, 96.f});
        m_page->addChild(title);

        // Content
        auto empty = CCLabelBMFont::create("Nothing Enabled", "bigFont.fnt");
        empty->setScale(0.5f);
        empty->setOpacity(120);
        empty->setPosition({60.f, -10.f});
        m_page->addChild(empty);

        std::string hintText = std::string(NOVA_TAB_NAMES[i]) + " options will appear here";
        auto hint = CCLabelBMFont::create(hintText.c_str(), "chatFont.fnt");
        hint->setScale(0.7f);
        hint->setColor({200, 200, 200});
        hint->setOpacity(160);
        hint->setPosition({60.f, -34.f});
        m_page->addChild(hint);

        m_page->setScale(0.96f);
        m_page->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.15f, 1.f)));
    }

public:
    static NovaMenu* create() {
        auto ret = new NovaMenu();
        if (ret && ret->initMenu()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void finishClose() {
        g_novaOpen = false;
        this->removeFromParentAndCleanup(true);
    }

    void onClose(CCObject*) {
        if (m_closing) return;
        m_closing = true;
        this->setKeypadEnabled(false);

        auto finishCall = CCCallFunc::create(this, callfunc_selector(NovaMenu::finishClose));
        auto scaleAnim = CCEaseBackIn::create(CCScaleTo::create(0.18f, 0.3f));
        m_root->runAction(CCSequence::create(scaleAnim, finishCall, nullptr));
    }

    void keyBackClicked() override {
        this->onClose(nullptr);
    }
};

class NovaBubble : public CCLayer {
protected:
    CCNode* m_orb = nullptr;
    bool m_dragging = false;
    CCPoint m_startTouch;
    CCPoint m_grab;

public:
    static NovaBubble* create() {
        auto ret = new NovaBubble();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) return false;
        this->setTouchMode(kCCTouchesOneByOne);
        this->setTouchPriority(-300);
        this->setTouchEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();

        // Native RobTop Floating Button
        m_orb = CCNode::create();
        
        auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_button_01.png"); // Green button
        btnSpr->setScale(0.95f);
        m_orb->addChild(btnSpr);

        auto lbl = CCLabelBMFont::create("N", "goldFont.fnt");
        lbl->setScale(0.85f);
        lbl->setPosition({1.f, 2.f}); // slightly offset center for gold font alignment
        m_orb->addChild(lbl);

        float x = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-x", 30.0));
        float y = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-y", win.height / 2.f + 20.f));
        
        // Safety bounds check for initial load
        x = std::max(26.f, std::min(win.width - 26.f, x));
        y = std::max(26.f, std::min(win.height - 26.f, y));
        m_orb->setPosition({x, y});
        this->addChild(m_orb);

        g_novaBubble = this;
        this->scheduleUpdate();
        return true;
    }

    void update(float dt) override {
        CCLayer::update(dt);

        if (g_novaOpen) {
            this->setVisible(false);
            return;
        }

        auto playLayer = PlayLayer::get();
        if (playLayer) {
            this->setVisible(playLayer->m_isPaused);
        } else {
            this->setVisible(true);
        }
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!this->isVisible() || g_novaOpen) return false;
        auto p = this->convertToNodeSpace(t->getLocation());
        
        // Check if user tapped inside the button
        if (ccpDistance(p, m_orb->getPosition()) > 28.f) return false;

        m_dragging = false;
        m_startTouch = p;
        m_grab = m_orb->getPosition() - p;
        
        // Darken sprite slightly on click
        m_orb->setScale(0.9f);
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = this->convertToNodeSpace(t->getLocation());
        if (!m_dragging && ccpDistance(p, m_startTouch) > 8.f) m_dragging = true;
        if (!m_dragging) return;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto np = p + m_grab;
        
        // Clamp inside screen bounds, no wall-snapping!
        np.x = std::max(26.f, std::min(win.width - 26.f, np.x));
        np.y = std::max(26.f, std::min(win.height - 26.f, np.y));
        m_orb->setPosition(np);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_orb->setScale(1.f);
        
        if (!m_dragging) {
            if (auto menu = NovaMenu::create()) menu->show();
            return;
        }
        
        // Save EXACT position, completely freely floating.
        auto pos = m_orb->getPosition();
        Mod::get()->setSavedValue<double>("nova-bubble-x", pos.x);
        Mod::get()->setSavedValue<double>("nova-bubble-y", pos.y);
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_orb->setScale(1.f);
    }
};

class $modify(NovaMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        static bool created = false;
        if (!created) {
            created = true;
            auto director = CCDirector::sharedDirector();
            if (!director->getNotificationNode()) {
                director->setNotificationNode(CCNode::create());
            }
            director->getNotificationNode()->addChild(NovaBubble::create(), 999);
        }
        return true;
    }
};
