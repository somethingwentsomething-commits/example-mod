#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
using namespace geode::prelude;

class DragTab : public CCNode {
public:
    float m_w = 130.f;
    float m_h = 170.f;
    float m_bar = 24.f;

    static DragTab* create(std::string const& title, CCPoint pos) {
        auto ret = new DragTab();
        if (ret && ret->setup(title, pos)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool setup(std::string const& title, CCPoint pos) {
        if (!CCNode::init()) return false;
        this->setContentSize({m_w, m_h});
        this->setPosition(pos);

        auto bg = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
        bg->setContentSize({m_w, m_h});
        bg->setAnchorPoint({0, 0});
        bg->setColor({0, 0, 0});
        bg->setOpacity(190);
        this->addChild(bg);

        auto bar = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
        bar->setContentSize({m_w, m_bar});
        bar->setAnchorPoint({0, 0});
        bar->setPosition({0, m_h - m_bar});
        bar->setColor({40, 120, 255});
        this->addChild(bar);

        auto label = CCLabelBMFont::create(title.c_str(), "bigFont.fnt");
        label->setScale(0.45f);
        label->setPosition({m_w / 2, m_h - m_bar / 2});
        this->addChild(label);

        auto empty = CCLabelBMFont::create("(empty)", "bigFont.fnt");
        empty->setScale(0.35f);
        empty->setOpacity(120);
        empty->setPosition({m_w / 2, (m_h - m_bar) / 2});
        this->addChild(empty);

        return true;
    }

    bool hitTitle(CCPoint worldPoint) {
        auto p = this->convertToNodeSpace(worldPoint);
        return p.x >= 0 && p.x <= m_w && p.y >= m_h - m_bar && p.y <= m_h;
    }
};

class ModMenu : public FLAlertLayer {
protected:
    std::vector<DragTab*> m_tabs;
    DragTab* m_dragging = nullptr;
    CCPoint m_offset;
    int m_topZ = 10;
    CCLabelBMFont* m_closeLabel = nullptr;

    bool initMenu() {
        if (!FLAlertLayer::init(150)) return false;
        m_noElasticity = true;

        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_closeLabel = CCLabelBMFont::create("X", "bigFont.fnt");
        m_closeLabel->setPosition({win.width - 25, win.height - 25});
        m_mainLayer->addChild(m_closeLabel, 1000);

        const char* names[3] = {"Player", "Level", "Misc"};
        float startX = (win.width - (3 * 130.f + 2 * 10.f)) / 2;
        for (int i = 0; i < 3; i++) {
            auto tab = DragTab::create(names[i], {startX + i * 140.f, 70.f});
            m_mainLayer->addChild(tab, 10 + i);
            m_tabs.push_back(tab);
        }
        m_topZ = 10 + 3;
        return true;
    }

public:
    static ModMenu* create() {
        auto ret = new ModMenu();
        if (ret && ret->initMenu()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    void onClose() {
        this->setKeypadEnabled(false);
        this->setTouchEnabled(false);
        this->removeFromParentAndCleanup(true);
    }

    void keyBackClicked() override {
        this->onClose();
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        auto loc = touch->getLocation();
        auto local = m_mainLayer->convertToNodeSpace(loc);

        if (ccpDistance(local, m_closeLabel->getPosition()) < 25.f) {
            this->onClose();
            return true;
        }

        DragTab* hit = nullptr;
        for (auto tab : m_tabs) {
            if (tab->hitTitle(loc) && (!hit || tab->getZOrder() > hit->getZOrder())) {
                hit = tab;
            }
        }
        if (hit) {
            m_dragging = hit;
            m_offset = hit->getPosition() - local;
            m_mainLayer->reorderChild(hit, ++m_topZ);
        }
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) return;
        auto local = m_mainLayer->convertToNodeSpace(touch->getLocation());
        m_dragging->setPosition(local + m_offset);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_dragging = nullptr;
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = nullptr;
    }
};

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
        ModMenu::create()->show();
    }
};
