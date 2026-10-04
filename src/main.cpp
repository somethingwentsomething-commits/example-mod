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

// Standard native GD icons
static const char* NOVA_TAB_ICONS[NOVA_TABS] = {
    "GJ_infoIcon_001.png",       
    "GJ_profileButton_001.png",  
    "GJ_playBtn2_001.png",       
    "GJ_lock_001.png",           
    "GJ_colorBtn_001.png",       
    "GJ_creatorBtn_001.png",     
    "GJ_optionsBtn_001.png"      
};

class NovaMenu : public FLAlertLayer {
protected:
    CCNode* m_root = nullptr;
    CCNode* m_page = nullptr;
    CCMenu* m_tabMenu = nullptr;
    std::vector<CCMenuItemSpriteExtra*> m_tabButtons;
    std::vector<CCSprite*> m_tabBgs;
    std::vector<CCLabelBMFont*> m_tabLabels;
    std::vector<CCSprite*> m_tabIcons;
    int m_cur = -1;
    bool m_closing = false;

    bool initMenu() {
        if (!FLAlertLayer::init(150)) return false;
        m_noElasticity = true;
        g_novaOpen = true;

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f, win.height / 2.f});
        m_mainLayer->addChild(m_root);

        // --- 1. Main Background (RobTop Brown Box) ---
        auto bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({NOVA_W, NOVA_H});
        m_root->addChild(bg);

        // --- 2. Left Sidebar Panel (RobTop Dark Inset) ---
        auto sideBg = CCScale9Sprite::create("GJ_square02.png");
        sideBg->setContentSize({114.f, NOVA_H - 50.f});
        sideBg->setPosition({-148.f, -12.f});
        m_root->addChild(sideBg);

        // --- 3. Content Panel (RobTop Dark Inset) ---
        auto contentBg = CCScale9Sprite::create("GJ_square02.png");
        contentBg->setContentSize({274.f, NOVA_H - 50.f});
        contentBg->setPosition({62.f, -12.f});
        m_root->addChild(contentBg);

        // --- 4. Title Header ---
        auto title = CCLabelBMFont::create("NOVA MENU", "goldFont.fnt");
        title->setScale(0.75f);
        title->setPosition({0.f, NOVA_H / 2.f - 22.f});
        m_root->addChild(title);

        // --- 5. Close Button (Top Left Corner) ---
        auto closeBtnSprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
        auto closeBtn = CCMenuItemSpriteExtra::create(
            closeBtnSprite,
            this,
            menu_selector(NovaMenu::onClose)
        );

        auto closeMenu = CCMenu::create();
        closeMenu->setPosition({-NOVA_W / 2.f + 3.f, NOVA_H / 2.f - 3.f});
        closeMenu->addChild(closeBtn);
        m_root->addChild(closeMenu);

        // --- 6. Sidebar Tabs ---
        m_tabMenu = CCMenu::create();
        m_tabMenu->setPosition({0.f, 0.f});
        m_root->addChild(m_tabMenu);

        float startY = 78.f;
        float spacingY = 30.f;

        for (int i = 0; i < NOVA_TABS; i++) {
            auto container = CCNode::create();
            container->setContentSize({104.f, 26.f});

            // Standard CCSprite for button background (Fixes pink texture issue)
            auto sprBg = CCSprite::createWithSpriteFrameName("GJ_button_02.png");
            sprBg->setScaleX(104.f / sprBg->getContentSize().width);
            sprBg->setScaleY(26.f / sprBg->getContentSize().height);
            sprBg->setPosition({52.f, 13.f});
            m_tabBgs.push_back(sprBg);
            container->addChild(sprBg);

            // Icon
            auto icon = CCSprite::createWithSpriteFrameName(NOVA_TAB_ICONS[i]);
            float maxDim = std::max(icon->getContentSize().width, icon->getContentSize().height);
            if (maxDim > 0.f) {
                icon->setScale(16.f / maxDim);
            }
            icon->setPosition({15.f, 13.f});
            icon->setColor({200, 200, 200});
            m_tabIcons.push_back(icon);
            container->addChild(icon);

            // Text Label
            auto lbl = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
            lbl->limitLabelWidth(64.f, 0.42f, 0.1f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({28.f, 14.f});
            lbl->setColor({180, 180, 180});
            m_tabLabels.push_back(lbl);
            container->addChild(lbl);

            auto btn = CCMenuItemSpriteExtra::create(
                container,
                this,
                menu_selector(NovaMenu::onTabClicked)
            );
            btn->setTag(i);
            btn->setPosition({-148.f, startY - spacingY * i});
            m_tabMenu->addChild(btn);
            m_tabButtons.push_back(btn);
        }

        // Elastic open animation
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

        auto cache = CCSpriteFrameCache::sharedSpriteFrameCache();
        for (int k = 0; k < NOVA_TABS; k++) {
            bool on = (k == i);
            auto frameName = on ? "GJ_button_01.png" : "GJ_button_02.png";
            if (auto frame = cache->spriteFrameByName(frameName)) {
                m_tabBgs[k]->setDisplayFrame(frame);
                m_tabBgs[k]->setScaleX(104.f / frame->getRect().size.width);
                m_tabBgs[k]->setScaleY(26.f / frame->getRect().size.height);
            }

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
        title->setPosition({-62.f, 88.f});
        m_page->addChild(title);

        // Placeholder content
        auto empty = CCLabelBMFont::create("Nothing Enabled", "bigFont.fnt");
        empty->setScale(0.5f);
        empty->setOpacity(120);
        empty->setPosition({62.f, -8.f});
        m_page->addChild(empty);

        std::string hintText = std::string(NOVA_TAB_NAMES[i]) + " options will appear here";
        auto hint = CCLabelBMFont::create(hintText.c_str(), "chatFont.fnt");
        hint->setScale(0.7f);
        hint->setColor({200, 200, 200});
        hint->setOpacity(160);
        hint->setPosition({62.f, -32.f});
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

        // Native GD Green Floating Button
        m_orb = CCNode::create();
        
        auto btnSpr = CCSprite::createWithSpriteFrameName("GJ_button_01.png"); 
        btnSpr->setScale(0.95f);
        m_orb->addChild(btnSpr);

        auto lbl = CCLabelBMFont::create("N", "goldFont.fnt");
        lbl->setScale(0.85f);
        lbl->setPosition({1.f, 2.f}); 
        m_orb->addChild(lbl);

        float x = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-x", 30.0));
        float y = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-y", win.height / 2.f + 20.f));
        
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
        
        if (ccpDistance(p, m_orb->getPosition()) > 28.f) return false;

        m_dragging = false;
        m_startTouch = p;
        m_grab = m_orb->getPosition() - p;
        
        m_orb->setScale(0.9f);
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = this->convertToNodeSpace(t->getLocation());
        if (!m_dragging && ccpDistance(p, m_startTouch) > 8.f) m_dragging = true;
        if (!m_dragging) return;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto np = p + m_grab;
        
        np.x = std::max(26.f, std::min(win.width - 26.f, np.x));
        np.y = std::max(26.f, std::min(win.height - 26.f, np.y));
        m_orb->setPosition(np);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_orb->setScale(1.f);
        
        if (!m_dragging) {
            if (auto menu = NovaMenu::create()) {
                menu->show();
            }
            return;
        }
        
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
            if (auto notificationNode = director->getNotificationNode()) {
                notificationNode->addChild(NovaBubble::create(), 999);
            }
        }
        return true;
    }
};
