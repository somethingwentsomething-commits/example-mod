#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
using namespace geode::prelude;

static bool g_novaOpen = false;
static CCNode* g_novaBubble = nullptr;

static const int NOVA_TABS = 10;
static const float NOVA_W = 460.f;
static const float NOVA_H = 280.f;
static const char* NOVA_TAB_NAMES[NOVA_TABS] = {
    "Player", "Level", "Visual", "Bypass", "Game",
    "Speed", "Practice", "Icons", "Labels", "Settings"
};

// filled rounded rectangle, centered on (0,0)
static CCDrawNode* novaRect(float w, float h, float r, int red, int green, int blue) {
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
        ccc4f(red / 255.f, green / 255.f, blue / 255.f, 1.f),
        0.f, ccc4f(0.f, 0.f, 0.f, 0.f)
    );
    return node;
}

class NovaMenu : public FLAlertLayer {
protected:
    CCNode* m_root = nullptr;
    CCNode* m_page = nullptr;
    std::vector<CCNode*> m_tabNodes;
    std::vector<CCNode*> m_tabBgs;
    std::vector<CCNode*> m_tabBars;
    std::vector<CCLabelBMFont*> m_tabLabels;
    int m_cur = -1;
    bool m_closing = false;

    bool initMenu() {
        if (!FLAlertLayer::init(150)) return false;
        m_noElasticity = true;
        g_novaOpen = true;
        if (g_novaBubble) g_novaBubble->setVisible(false);

        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();
        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f, win.height / 2.f});
        m_mainLayer->addChild(m_root);

        // panel + border + sidebar
        m_root->addChild(novaRect(NOVA_W + 3.f, NOVA_H + 3.f, 14.f, 36, 78, 150));
        m_root->addChild(novaRect(NOVA_W, NOVA_H, 12.f, 20, 22, 32));
        auto side = novaRect(112.f, NOVA_H - 16.f, 9.f, 13, 14, 21);
        side->setPosition({-166.f, 0.f});
        m_root->addChild(side);

        // tabs
        for (int i = 0; i < NOVA_TABS; i++) {
            auto node = CCNode::create();
            node->setPosition({-166.f, 112.5f - 25.f * i});
            m_root->addChild(node);

            auto bg = novaRect(100.f, 21.f, 5.f, 30, 62, 118);
            bg->setVisible(false);
            node->addChild(bg);

            auto bar = novaRect(3.f, 12.f, 1.5f, 58, 134, 255);
            bar->setPosition({-43.f, 0.f});
            bar->setVisible(false);
            node->addChild(bar);

            auto lbl = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
            lbl->limitLabelWidth(76.f, 0.4f, 0.1f);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({-34.f, 0.f});
            lbl->setColor({140, 146, 170});
            node->addChild(lbl);

            m_tabNodes.push_back(node);
            m_tabBgs.push_back(bg);
            m_tabBars.push_back(bar);
            m_tabLabels.push_back(lbl);
        }

        // close button
        auto closeBg = novaRect(24.f, 24.f, 7.f, 34, 38, 58);
        closeBg->setPosition({205.f, 116.f});
        m_root->addChild(closeBg);
        auto closeX = CCLabelBMFont::create("X", "bigFont.fnt");
        closeX->setScale(0.42f);
        closeX->setPosition({205.f, 116.f});
        m_root->addChild(closeX);

        // footer
        auto foot = CCLabelBMFont::create("Nova Menu  v1.0", "bigFont.fnt");
        foot->setScale(0.26f);
        foot->setOpacity(90);
        foot->setAnchorPoint({1.f, 0.5f});
        foot->setPosition({214.f, -132.f});
        m_root->addChild(foot);

        // open animation
        m_root->setScale(0.86f);
        m_root->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.22f, 1.f)));

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
            m_tabLabels[k]->setColor(on ? ccColor3B{255, 255, 255} : ccColor3B{140, 146, 170});
        }
        auto node = m_tabNodes[i];
        node->stopAllActions();
        node->setScale(0.94f);
        node->runAction(CCEaseExponentialOut::create(CCScaleTo::create(0.2f, 1.f)));
        buildPage(i);
        Mod::get()->setSavedValue<int>("nova-last-tab", i);
    }

    void buildPage(int i) {
        if (m_page) m_page->removeFromParent();
        m_page = CCNode::create();
        m_root->addChild(m_page);

        auto title = CCLabelBMFont::create(NOVA_TAB_NAMES[i], "bigFont.fnt");
        title->setScale(0.62f);
        title->setAnchorPoint({0.f, 0.5f});
        title->setPosition({-94.f, 116.f});
        m_page->addChild(title);

        auto line = novaRect(308.f, 3.f, 1.5f, 58, 134, 255);
        line->setPosition({60.f, 100.f});
        m_page->addChild(line);

        auto card = novaRect(324.f, 216.f, 10.f, 26, 29, 43);
        card->setPosition({60.f, -16.f});
        m_page->addChild(card);

        auto empty = CCLabelBMFont::create("Nothing here yet", "bigFont.fnt");
        empty->setScale(0.5f);
        empty->setOpacity(130);
        empty->setPosition({60.f, -8.f});
        m_page->addChild(empty);

        std::string hintText = std::string(NOVA_TAB_NAMES[i]) + " cheats will show up here";
        auto hint = CCLabelBMFont::create(hintText.c_str(), "bigFont.fnt");
        hint->setScale(0.3f);
        hint->setOpacity(80);
        hint->setPosition({60.f, -30.f});
        m_page->addChild(hint);
    }

public:
    ~NovaMenu() {
        g_novaOpen = false;
        if (g_novaBubble) g_novaBubble->setVisible(true);
    }

    static NovaMenu* create() {
        auto ret = new NovaMenu();
        if (ret && ret->initMenu()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void onClose(CCObject*) {
        if (m_closing) return;
        m_closing = true;
        g_novaOpen = false;
        if (g_novaBubble) g_novaBubble->setVisible(true);
        this->setKeypadEnabled(false);
        this->removeFromParentAndCleanup(true);
    }

    void keyBackClicked() override {
        this->onClose(nullptr);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        auto p = m_root->convertToNodeSpace(t->getLocation());
        if (std::fabs(p.x) > NOVA_W / 2.f || std::fabs(p.y) > NOVA_H / 2.f) {
            this->onClose(nullptr);
            return true;
        }
        if (std::fabs(p.x - 205.f) < 15.f && std::fabs(p.y - 116.f) < 15.f) {
            this->onClose(nullptr);
            return true;
        }
        if (std::fabs(p.x + 166.f) < 52.f) {
            for (int i = 0; i < NOVA_TABS; i++) {
                if (std::fabs(p.y - (112.5f - 25.f * i)) < 12.5f) {
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

        m_orb = CCNode::create();
        m_orb->addChild(novaRect(46.f, 46.f, 23.f, 16, 20, 36));
        m_orb->addChild(novaRect(40.f, 40.f, 20.f, 58, 134, 255));
        auto lbl = CCLabelBMFont::create("N", "bigFont.fnt");
        lbl->setScale(0.55f);
        m_orb->addChild(lbl);

        float x = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-x", 26.0));
        float y = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-y", win.height / 2.f + 20.f));
        x = std::max(24.f, std::min(win.width - 24.f, x));
        y = std::max(24.f, std::min(win.height - 24.f, y));
        m_orb->setPosition({x, y});
        this->addChild(m_orb);

        g_novaBubble = this;
        return true;
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (g_novaOpen) return false;
        auto p = this->convertToNodeSpace(t->getLocation());
        if (ccpDistance(p, m_orb->getPosition()) > 30.f) return false;
        m_dragging = false;
        m_startTouch = p;
        m_grab = m_orb->getPosition() - p;
        m_orb->stopAllActions();
        m_orb->setScale(0.92f);
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = this->convertToNodeSpace(t->getLocation());
        if (!m_dragging && ccpDistance(p, m_startTouch) > 8.f) m_dragging = true;
        if (!m_dragging) return;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto np = p + m_grab;
        np.x = std::max(24.f, std::min(win.width - 24.f, np.x));
        np.y = std::max(24.f, std::min(win.height - 24.f, np.y));
        m_orb->setPosition(np);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_orb->setScale(1.f);
        if (!m_dragging) {
            if (auto menu = NovaMenu::create()) menu->show();
            return;
        }
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto pos = m_orb->getPosition();
        float tx = pos.x < win.width / 2.f ? 26.f : win.width - 26.f;
        m_orb->runAction(CCEaseExponentialOut::create(CCMoveTo::create(0.25f, ccp(tx, pos.y))));
        Mod::get()->setSavedValue<double>("nova-bubble-x", tx);
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
            SceneManager::get()->keepAcrossScenes(NovaBubble::create());
        }
        return true;
    }
};
