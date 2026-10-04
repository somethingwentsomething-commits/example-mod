#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace geode::prelude;

static bool g_novaOpen = false;
static CCNode* g_novaBubble = nullptr;

static const int NOVA_TABS = 7;
static const float NOVA_W = 470.f;
static const float NOVA_H = 290.f;
static const char* NOVA_TAB_NAMES[NOVA_TABS] = {
    "Overall", "Player", "Level", "Bypass", "Visual", "Creator", "Settings"
};

// Enhanced rounded rectangle with smooth draw polygon & optional custom opacity
static CCDrawNode* novaRect(float w, float h, float r, int red, int green, int blue, float alpha = 1.0f) {
    auto node = CCDrawNode::create();
    float hw = w / 2.f;
    float hh = h / 2.f;
    r = std::min(r, std::min(hw, hh) - 0.05f);
    int seg = r > 12.f ? 10 : 6;
    float cxs[4] = { hw - r, -hw + r, -hw + r, hw - r };
    float cys[4] = { hh - r, hh - r, -hh + r, -hh + r };
    std::vector<CCPoint> pts;
    for (int c = 0; c < 4; c++) {
        for (int i = 0; i <= seg; i++) {
            float a = (90.f * c + 90.f * i / seg) * 3.14159265f / 180.f;
            pts.push_back(CCPoint(cxs[c] + r * std::cos(a), cys[c] + r * std::sin(a)));
        }
    }
    node->drawPolygon(
        pts.data(), static_cast<unsigned int>(pts.size()),
        ccc4f(red / 255.f, green / 255.f, blue / 255.f, alpha),
        0.f, ccc4f(0.f, 0.f, 0.f, 0.f)
    );
    return node;
}

class NovaMenu : public FLAlertLayer {
protected:
    CCNode* m_root = nullptr;
    CCNode* m_page = nullptr;
    CCLayerColor* m_dimOverlay = nullptr;
    std::vector<CCNode*> m_tabNodes;
    std::vector<CCNode*> m_tabBgs;
    std::vector<CCNode*> m_tabBars;
    std::vector<CCLabelBMFont*> m_tabLabels;
    int m_cur = -1;
    bool m_closing = false;

    bool initMenu() {
        if (!FLAlertLayer::init(180)) return false;
        m_noElasticity = true;
        g_novaOpen = true;

        // Dim background layer with fade-in
        m_dimOverlay = CCLayerColor::create(ccc4(0, 0, 0, 0));
        this->addChild(m_dimOverlay, -1);
        m_dimOverlay->runAction(CCFadeTo::create(0.2f, 160));

        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();
        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f, win.height / 2.f});
        m_mainLayer->addChild(m_root);

        // Cyberpunk style background layers & side panel
        m_root->addChild(novaRect(NOVA_W + 6.f, NOVA_H + 6.f, 16.f, 0, 210, 255, 0.25f)); // Outer glow border
        m_root->addChild(novaRect(NOVA_W + 2.f, NOVA_H + 2.f, 14.f, 30, 42, 70));         // Inner stroke
        m_root->addChild(novaRect(NOVA_W, NOVA_H, 12.f, 13, 16, 26));                     // Main body
        
        auto side = novaRect(118.f, NOVA_H - 16.f, 9.f, 20, 24, 38);
        side->setPosition({-166.f, 0.f});
        m_root->addChild(side);

        // Tab list setup for 7 tabs
        float startY = 96.f;
        float spacingY = 32.f;

        for (int i = 0; i < NOVA_TABS; i++) {
            auto node = CCNode::create();
            node->setPosition({-166.f, startY - spacingY * i});
            m_root->addChild(node);

            auto bg = novaRect(108.f, 26.f, 6.f, 0, 162, 255, 0.22f);
            bg->setVisible(false);
            node->addChild(bg);

            auto bar = novaRect(3.5f, 16.f, 1.5f, 0, 230, 255);
            bar->setPosition({-48.f, 0.f});
            bar->setVisible(false);
            node->addChild(bar);

            auto lbl = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
            lbl->limitLabelWidth(82.f, 0.42f, 0.1f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({-38.f, 0.f});
            lbl->setColor({140, 150, 175});
            node->addChild(lbl);

            m_tabNodes.push_back(node);
            m_tabBgs.push_back(bg);
            m_tabBars.push_back(bar);
            m_tabLabels.push_back(lbl);
        }

        // Close button (Top-Right)
        auto closeBg = novaRect(26.f, 26.f, 8.f, 230, 45, 75);
        closeBg->setPosition({212.f, 120.f});
        m_root->addChild(closeBg);
        auto closeX = CCLabelBMFont::create("X", "bigFont.fnt");
        closeX->setScale(0.42f);
        closeX->setPosition({212.f, 120.f});
        m_root->addChild(closeX);

        // Header brand branding
        auto brand = CCLabelBMFont::create("NOVA", "bigFont.fnt");
        brand->setScale(0.48f);
        brand->setColor({0, 230, 255});
        brand->setAnchorPoint({0.f, 0.5f});
        brand->setPosition({-212.f, 120.f});
        m_root->addChild(brand);

        // Footer
        auto foot = CCLabelBMFont::create("Nova Menu v2.0", "bigFont.fnt");
        foot->setScale(0.24f);
        foot->setOpacity(100);
        foot->setAnchorPoint({1.f, 0.5f});
        foot->setPosition({220.f, -132.f});
        m_root->addChild(foot);

        // Open Scale Pop Animation
        m_root->setScale(0.5f);
        m_root->runAction(CCEaseBackOut::create(CCScaleTo::create(0.25f, 1.f)));

        int last = Mod::get()->getSavedValue<int>("nova-last-tab", 0);
        selectTab(std::max(0, std::min(NOVA_TABS - 1, last)));
        return true;
    }

    void selectTab(int i) {
        if (i == m_cur) return;
        m_cur = i;
        for (int k = 0; k < NOVA_TABS; k++) {
            bool on = (k == i);
            m_tabBgs[k]->setVisible(on);
            m_tabBars[k]->setVisible(on);
            m_tabLabels[k]->setColor(on ? ccColor3B{255, 255, 255} : ccColor3B{140, 150, 175});
        }
        auto node = m_tabNodes[i];
        node->stopAllActions();
        node->setScale(0.9f);
        node->runAction(CCEaseBackOut::create(CCScaleTo::create(0.2f, 1.f)));
        buildPage(i);
        Mod::get()->setSavedValue<int>("nova-last-tab", i);
    }

    void buildPage(int i) {
        if (m_page) m_page->removeFromParent();
        m_page = CCNode::create();
        m_root->addChild(m_page);

        // Title text
        auto title = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
        title->setScale(0.6f);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({-92.f, 120.f});
        m_page->addChild(title);

        // Title accent divider line
        auto line = novaRect(310.f, 2.5f, 1.f, 0, 200, 255, 0.8f);
        line->setPosition({62.f, 102.f});
        m_page->addChild(line);

        // Inner page card container
        auto card = novaRect(324.f, 218.f, 10.f, 20, 25, 40);
        card->setPosition({62.f, -16.f});
        m_page->addChild(card);

        // Placeholder content text
        auto empty = CCLabelBMFont::create("No Cheats Active", "bigFont.fnt");
        empty->setScale(0.48f);
        empty->setOpacity(140);
        empty->setPosition({62.f, -4.f});
        m_page->addChild(empty);

        std::string hintText = std::string(NOVA_TAB_NAMES[i]) + " options will appear here";
        auto hint = CCLabelBMFont::create(hintText.c_str(), "bigFont.fnt");
        hint->setScale(0.28f);
        hint->setOpacity(90);
        hint->setPosition({62.f, -28.f});
        m_page->addChild(hint);

        // Page switch animation
        m_page->setScale(0.96f);
        m_page->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.18f, 1.f)));
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

        if (m_dimOverlay) {
            m_dimOverlay->runAction(CCFadeTo::create(0.18f, 0));
        }

        auto finishCall = CCCallFunc::create(this, callfunc_selector(NovaMenu::finishClose));
        auto scaleAnim = CCEaseBackIn::create(CCScaleTo::create(0.18f, 0.4f));
        m_root->runAction(CCSequence::create(scaleAnim, finishCall, nullptr));
    }

    void keyBackClicked() override {
        this->onClose(nullptr);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (m_closing) return true;
        auto p = m_root->convertToNodeSpace(t->getLocation());

        // Click outside panel to close
        if (std::fabs(p.x) > NOVA_W / 2.f || std::fabs(p.y) > NOVA_H / 2.f) {
            this->onClose(nullptr);
            return true;
        }

        // Close button check
        if (std::fabs(p.x - 212.f) < 18.f && std::fabs(p.y - 120.f) < 18.f) {
            this->onClose(nullptr);
            return true;
        }

        // Sidebar tab click check
        if (std::fabs(p.x + 166.f) < 56.f) {
            float startY = 96.f;
            float spacingY = 32.f;
            for (int i = 0; i < NOVA_TABS; i++) {
                if (std::fabs(p.y - (startY - spacingY * i)) < 15.f) {
                    this->selectTab(i);
                    break;
                }
            }
        }
        return true;
    }

    void ccTouchMoved(CCTouch*, CCEvent*) override {}
    void ccTouchEnded(CCTouch*, CCEvent*) override {}
    void ccTouchCancelled(CCTouch*, CCEvent*) override {}
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

        // Multi-layered Orb Bubble
        m_orb = CCNode::create();
        
        // Outer aura glow & border
        m_orb->addChild(novaRect(52.f, 52.f, 26.f, 0, 220, 255, 0.25f));
        m_orb->addChild(novaRect(46.f, 46.f, 23.f, 15, 20, 32, 0.95f));
        m_orb->addChild(novaRect(38.f, 38.f, 19.f, 0, 150, 255, 0.85f));
        
        // Native GD Icon Symbol inside the bubble
        auto iconSpr = CCSprite::createWithSpriteFrameName("GJ_demonIcon_001.png");
        if (!iconSpr) iconSpr = CCSprite::createWithSpriteFrameName("GJ_starIcon_001.png");
        if (iconSpr) {
            float maxDim = std::max(iconSpr->getContentSize().width, iconSpr->getContentSize().height);
            if (maxDim > 0.f) {
                iconSpr->setScale(22.f / maxDim);
            }
            m_orb->addChild(iconSpr);
        }

        float x = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-x", 28.0));
        float y = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-y", win.height / 2.f + 20.f));
        x = std::max(26.f, std::min(win.width - 26.f, x));
        y = std::max(26.f, std::min(win.height - 26.f, y));
        m_orb->setPosition({x, y});
        this->addChild(m_orb);

        // Breathing idle animation
        auto pulse = CCRepeatForever::create(
            CCSequence::create(
                CCEaseInOut::create(CCScaleTo::create(1.2f, 1.05f), 2.0f),
                CCEaseInOut::create(CCScaleTo::create(1.2f, 0.96f), 2.0f),
                nullptr
            )
        );
        m_orb->runAction(pulse);

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
        if (!m_dragging) {
            if (auto menu = NovaMenu::create()) menu->show();
            return;
        }
        
        // Exact Free Floating: Saves wherever you drop it (No Edge-Snapping)
        auto pos = m_orb->getPosition();
        Mod::get()->setSavedValue<double>("nova-bubble-x", pos.x);
        Mod::get()->setSavedValue<double>("nova-bubble-y", pos.y);
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {}
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
