#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/ui/Popup.hpp>
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

// Vanilla GD icons guaranteed to be loaded in memory
static const char* NOVA_TAB_ICONS[NOVA_TABS] = {
    "GJ_infoIcon_001.png",       
    "GJ_profileButton_001.png",  
    "GJ_playBtn2_001.png",       
    "GJ_lock_001.png",           
    "GJ_colorBtn_001.png",       
    "GJ_creatorBtn_001.png",     
    "GJ_optionsBtn_001.png"      
};

// =========================================================
// NOVA MENU (Using Native Geode Popup for flawless physics)
// =========================================================
class NovaMenu : public geode::Popup<> {
protected:
    CCNode* m_page = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_tabButtons;
    std::vector<CCSprite*> m_tabBgs;
    std::vector<CCLabelBMFont*> m_tabLabels;
    std::vector<CCSprite*> m_tabIcons;
    int m_cur = -1;

    bool setup() override {
        m_noElasticity = false; // Enable native RobTop elastic bounce
        g_novaOpen = true;
        
        // Hide default popup title, we will place our own Custom Title
        this->setTitle("");
        
        // --- 1. Background Panels ---
        // Left sidebar panel (dark brown GD inset)
        auto sideBg = CCScale9Sprite::create("GJ_square02.png");
        sideBg->setContentSize({110.f, NOVA_H - 20.f});
        sideBg->setPosition({65.f, NOVA_H / 2.f});
        m_mainLayer->addChild(sideBg);

        // Main content card panel
        auto contentBg = CCScale9Sprite::create("GJ_square02.png");
        contentBg->setContentSize({300.f, NOVA_H - 50.f});
        contentBg->setPosition({280.f, NOVA_H / 2.f - 15.f});
        m_mainLayer->addChild(contentBg);

        // Custom Title text (RobTop Gold Font) centered over the content area
        auto title = CCLabelBMFont::create("NOVA MENU", "goldFont.fnt");
        title->setScale(0.75f);
        title->setPosition({280.f, NOVA_H - 25.f});
        m_mainLayer->addChild(title);

        // --- 2. Side Tabs ---
        auto tabMenu = CCMenu::create();
        tabMenu->setPosition({0.f, 0.f});
        m_mainLayer->addChild(tabMenu);

        float startY = NOVA_H - 35.f;
        float spacingY = (NOVA_H - 70.f) / (NOVA_TABS - 1); 

        for (int i = 0; i < NOVA_TABS; i++) {
            auto container = CCNode::create();
            container->setContentSize({100.f, 30.f});

            // Tab Background (Using standard CCSprite scaled to prevent pink texture bugs)
            auto sprBg = CCSprite::createWithSpriteFrameName("GJ_button_02.png");
            sprBg->setScaleX(100.f / sprBg->getContentSize().width);
            sprBg->setScaleY(30.f / sprBg->getContentSize().height);
            sprBg->setPosition({50.f, 15.f});
            m_tabBgs.push_back(sprBg);
            container->addChild(sprBg);

            // Tab Icon (Auto-scaled to fit nicely)
            auto icon = CCSprite::createWithSpriteFrameName(NOVA_TAB_ICONS[i]);
            float maxDim = std::max(icon->getContentSize().width, icon->getContentSize().height);
            icon->setScale(18.f / maxDim);
            icon->setPosition({16.f, 15.f});
            icon->setColor({200, 200, 200});
            m_tabIcons.push_back(icon);
            container->addChild(icon);

            // Tab Text
            auto lbl = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
            lbl->limitLabelWidth(60.f, 0.45f, 0.1f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({30.f, 16.f});
            lbl->setColor({180, 180, 180});
            m_tabLabels.push_back(lbl);
            container->addChild(lbl);

            auto btn = CCMenuItemSpriteExtra::create(
                container,
                this,
                menu_selector(NovaMenu::onTabClicked)
            );
            btn->setTag(i);
            btn->setPosition({65.f, startY - spacingY * i});
            tabMenu->addChild(btn);
            m_tabButtons.push_back(btn);
        }

        // Load last tab
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

        auto cache = CCSpriteFrameCache::sharedSpriteFrameCache();
        for (int k = 0; k < NOVA_TABS; k++) {
            bool on = (k == i);
            
            // Swap between green (active) and gray (inactive) vanilla frames
            auto frameName = on ? "GJ_button_01.png" : "GJ_button_02.png";
            m_tabBgs[k]->setDisplayFrame(cache->spriteFrameByName(frameName));

            m_tabLabels[k]->setColor(on ? ccColor3B{255, 255, 255} : ccColor3B{180, 180, 180});
            m_tabIcons[k]->setColor(on ? ccColor3B{255, 255, 255} : ccColor3B{200, 200, 200});
        }

        buildPage(i);
        Mod::get()->setSavedValue<int>("nova-last-tab", i);
    }

    void buildPage(int i) {
        if (m_page) m_page->removeFromParent();
        m_page = CCNode::create();
        m_mainLayer->addChild(m_page);

        // Section Title
        auto title = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "goldFont.fnt");
        title->setScale(0.6f);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({145.f, NOVA_H - 52.f});
        m_page->addChild(title);

        // Content placeholder
        auto empty = CCLabelBMFont::create("Nothing Enabled", "bigFont.fnt");
        empty->setScale(0.5f);
        empty->setOpacity(120);
        empty->setPosition({280.f, NOVA_H / 2.f});
        m_page->addChild(empty);

        std::string hintText = std::string(NOVA_TAB_NAMES[i]) + " options will appear here";
        auto hint = CCLabelBMFont::create(hintText.c_str(), "chatFont.fnt");
        hint->setScale(0.7f);
        hint->setColor({200, 200, 200});
        hint->setOpacity(160);
        hint->setPosition({280.f, NOVA_H / 2.f - 24.f});
        m_page->addChild(hint);

        // Page transition animation
        m_page->setScale(0.96f);
        m_page->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.15f, 1.f)));
    }

public:
    static NovaMenu* create() {
        auto ret = new NovaMenu();
        // initAnchored creates the native RobTop background automatically
        if (ret && ret->initAnchored(NOVA_W, NOVA_H)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
    
    void onClose(CCObject* sender) override {
        g_novaOpen = false;
        Popup::onClose(sender); // Handles the closing animation natively
    }
};

// =========================================================
// NOVA BUBBLE (Free-Floating & Persistent)
// =========================================================
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

        // Native RobTop Floating Green Button
        m_orb = CCNode::create();
        
        auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_button_01.png"); 
        btnSpr->setScale(1.1f);
        m_orb->addChild(btnSpr);

        auto lbl = CCLabelBMFont::create("N", "goldFont.fnt");
        lbl->setScale(0.95f);
        lbl->setPosition({1.f, 2.f}); 
        m_orb->addChild(lbl);

        float x = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-x", 40.0));
        float y = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-y", win.height / 2.f + 20.f));
        
        // Clamps strictly inside the screen borders so it doesn't get lost
        x = std::max(30.f, std::min(win.width - 30.f, x));
        y = std::max(30.f, std::min(win.height - 30.f, y));
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
        
        if (ccpDistance(p, m_orb->getPosition()) > 30.f) return false;

        m_dragging = false;
        m_startTouch = p;
        m_grab = m_orb->getPosition() - p;
        m_orb->setScale(0.95f); // Click feedback
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = this->convertToNodeSpace(t->getLocation());
        if (!m_dragging && ccpDistance(p, m_startTouch) > 8.f) m_dragging = true;
        if (!m_dragging) return;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto np = p + m_grab;
        
        // Exact Free-Floating (No Snapping) - Clamped to screen bounds
        np.x = std::max(30.f, std::min(win.width - 30.f, np.x));
        np.y = std::max(30.f, std::min(win.height - 30.f, np.y));
        m_orb->setPosition(np);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_orb->setScale(1.f);
        
        if (!m_dragging) {
            if (auto menu = NovaMenu::create()) menu->show();
            return;
        }
        
        // Saves its exact position precisely where you dropped it
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
