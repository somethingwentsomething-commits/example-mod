
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

using namespace geode::prelude;

static bool g_novaOpen = false;
static CCNode* g_novaBubble = nullptr;

static constexpr int TAB_COUNT = 7;
static constexpr float MENU_W = 550.f;
static constexpr float MENU_H = 365.f;

static const char* TAB_NAMES[TAB_COUNT] = {
    "Overall", "Player", "Level", "Bypass",
    "Visual", "Creator", "Settings"
};

// Palette inspired by the reference mockup.
static const ccColor3B BG       = {15, 19, 35};
static const ccColor3B SIDEBAR  = {13, 17, 31};
static const ccColor3B CARD     = {19, 24, 43};
static const ccColor3B TEXT     = {235, 239, 255};
static const ccColor3B MUTED    = {137, 151, 194};
static const ccColor3B ACCENT   = {100, 111, 255};
static const ccColor3B OUTLINE  = {57, 69, 132};

// Rounded polygon drawing helper.
static CCDrawNode* panel(
    float w, float h, float radius,
    ccColor3B color, float opacity = 1.f
) {
    auto draw = CCDrawNode::create();

    float hw = w * .5f;
    float hh = h * .5f;
    float r = std::max(0.f, std::min(radius, std::min(hw, hh)));

    std::vector<CCPoint> points;
    constexpr int segments = 8;

    const float cx[] = {hw-r, -hw+r, -hw+r, hw-r};
    const float cy[] = {hh-r, hh-r, -hh+r, -hh+r};

    for (int corner = 0; corner < 4; ++corner) {
        for (int j = 0; j <= segments; ++j) {
            float angle = (90.f * corner + 90.f * j / segments)
                * 3.14159265f / 180.f;

            points.emplace_back(
                cx[corner] + r * std::cos(angle),
                cy[corner] + r * std::sin(angle)
            );
        }
    }

    draw->drawPolygon(
        points.data(),
        static_cast<unsigned int>(points.size()),
        ccc4f(
            color.r / 255.f,
            color.g / 255.f,
            color.b / 255.f,
            opacity
        ),
        0.f,
        ccc4f(0.f, 0.f, 0.f, 0.f)
    );

    return draw;
}

static CCLabelBMFont* label(
    const char* text,
    float scale,
    ccColor3B color
) {
    auto result = CCLabelBMFont::create(text, "bigFont.fnt");
    result->setScale(scale);
    result->setColor(color);
    return result;
}

class NovaMenu : public FLAlertLayer {
protected:
    CCNode* m_root = nullptr;
    CCNode* m_page = nullptr;
    CCLayerColor* m_dim = nullptr;

    std::vector<CCNode*> m_tabNodes;
    std::vector<CCNode*> m_tabBgs;
    std::vector<CCNode*> m_tabBars;
    std::vector<CCLabelBMFont*> m_tabLabels;

    int m_current = -1;
    bool m_closing = false;

    bool initMenu() {
        if (!FLAlertLayer::init(180))
            return false;

        m_noElasticity = true;
        g_novaOpen = true;

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_dim = CCLayerColor::create(ccc4(0, 0, 0, 0));
        this->addChild(m_dim, -1);
        m_dim->runAction(CCFadeTo::create(.16f, 125));

        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);

        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f, win.height / 2.f});
        m_mainLayer->addChild(m_root);

        // Main panel and thin outline.
        m_root->addChild(panel(
            MENU_W + 2.f, MENU_H + 2.f, 16.f, OUTLINE
        ));
        m_root->addChild(panel(
            MENU_W, MENU_H, 15.f, BG
        ));

        // Header divider.
        auto headerLine = panel(
            MENU_W - 28.f, 1.f, .5f, OUTLINE, .85f
        );
        headerLine->setPosition({0.f, MENU_H / 2.f - 54.f});
        m_root->addChild(headerLine);

        // Sidebar.
        constexpr float sidebarW = 158.f;
        auto side = panel(
            sidebarW, MENU_H - 56.f, 0.f, SIDEBAR
        );
        side->setPosition({
            -MENU_W / 2.f + sidebarW / 2.f,
            -26.f
        });
        m_root->addChild(side);

        auto verticalLine = panel(
            1.f, MENU_H - 56.f, .5f, OUTLINE, .6f
        );
        verticalLine->setPosition({
            -MENU_W / 2.f + sidebarW,
            -26.f
        });
        m_root->addChild(verticalLine);

        // Brand.
        auto brand = label("NOVA", .65f, TEXT);
        brand->setAnchorPoint({0.f, .5f});
        brand->setPosition({-MENU_W / 2.f + 40.f,
                            MENU_H / 2.f - 27.f});
        m_root->addChild(brand);

        // Small accent mark.
        auto mark = label("*", .85f, ACCENT);
        mark->setPosition({-MENU_W / 2.f + 22.f,
                           MENU_H / 2.f - 27.f});
        m_root->addChild(mark);

        // Close button.
        auto closeBg = panel(27.f, 27.f, 13.f, CARD);
        closeBg->setPosition({
            MENU_W / 2.f - 25.f,
            MENU_H / 2.f - 27.f
        });
        m_root->addChild(closeBg);

        auto close = label("X", .38f, MUTED);
        close->setPosition(closeBg->getPosition());
        m_root->addChild(close);

        // Sidebar tabs.
        const float startY = 104.f;
        const float gap = 42.f;
        const float tabX = -MENU_W / 2.f + 82.f;

        for (int i = 0; i < TAB_COUNT; ++i) {
            auto node = CCNode::create();
            node->setPosition({
                tabX,
                startY - gap * i
            });
            m_root->addChild(node);

            auto bg = panel(142.f, 34.f, 9.f, ACCENT, .35f);
            bg->setVisible(false);
            node->addChild(bg);

            auto bar = panel(3.f, 19.f, 1.f, ACCENT);
            bar->setPosition({-68.f, 0.f});
            bar->setVisible(false);
            node->addChild(bar);

            auto tabLabel = label(TAB_NAMES[i], .42f, MUTED);
            tabLabel->setAnchorPoint({0.f, .5f});
            tabLabel->setPosition({-48.f, 0.f});
            tabLabel->limitLabelWidth(94.f, .42f, .1f);
            node->addChild(tabLabel);

            m_tabNodes.push_back(node);
            m_tabBgs.push_back(bg);
            m_tabBars.push_back(bar);
            m_tabLabels.push_back(tabLabel);
        }

        // Sidebar footer.
        auto footerLine = panel(118.f, 1.f, .5f, OUTLINE, .65f);
        footerLine->setPosition({
            -MENU_W / 2.f + 82.f,
            -MENU_H / 2.f + 34.f
        });
        m_root->addChild(footerLine);

        auto footer = label("Nova Menu v2.0", .27f, MUTED);
        footer->setAnchorPoint({0.f, .5f});
        footer->setPosition({
            -MENU_W / 2.f + 13.f,
            -MENU_H / 2.f + 17.f
        });
        m_root->addChild(footer);

        // Keep the open animation subtle.
        m_root->setScale(.97f);
        m_root->runAction(CCEaseExponentialOut::create(
            CCScaleTo::create(.16f, 1.f)
        ));

        int last = Mod::get()->getSavedValue<int>(
            "nova-last-tab", 0
        );
        selectTab(std::clamp(last, 0, TAB_COUNT - 1));

        return true;
    }

    void selectTab(int index) {
        if (index < 0 || index >= TAB_COUNT || index == m_current)
            return;

        m_current = index;

        for (int i = 0; i < TAB_COUNT; ++i) {
            bool active = (i == index);

            m_tabBgs[i]->setVisible(active);
            m_tabBars[i]->setVisible(active);
            m_tabLabels[i]->setColor(active ? TEXT : MUTED);
        }

        buildPage(index);
        Mod::get()->setSavedValue<int>("nova-last-tab", index);
    }

    void buildPage(int index) {
        if (m_page) {
            m_page->removeFromParentAndCleanup(true);
            m_page = nullptr;
        }

        m_page = CCNode::create();
        m_root->addChild(m_page);

        // Content starts to the right of the sidebar.
        const float contentX = 66.f;
        const float contentTop = 112.f;

        auto title = label(TAB_NAMES[index], .65f, TEXT);
        title->setAnchorPoint({0.f, .5f});
        title->setPosition({-76.f, contentTop});
        m_page->addChild(title);

        const char* descriptions[] = {
            "General options and quality of life features.",
            "Player controls and player options.",
            "Level options and gameplay settings.",
            "Compatibility and bypass options.",
            "Visual options and interface settings.",
            "Creator tools and editor options.",
            "Customize your Nova experience."
        };

        auto subtitle = label(descriptions[index], .31f, MUTED);
        subtitle->setAnchorPoint({0.f, .5f});
        subtitle->setPosition({-76.f, contentTop - 24.f});
        subtitle->limitLabelWidth(285.f, .31f, .1f);
        m_page->addChild(subtitle);

        auto divider = panel(292.f, 1.f, .5f, OUTLINE, .8f);
        divider->setPosition({contentX, contentTop - 39.f});
        m_page->addChild(divider);

        // Main content card.
        auto card = panel(300.f, 224.f, 12.f, CARD);
        card->setPosition({contentX, -31.f});
        m_page->addChild(card);

        auto star = label("*", 1.2f, MUTED);
        star->setPosition({contentX, 2.f});
        m_page->addChild(star);

        auto empty = label("No Cheats Active", .52f, MUTED);
        empty->setPosition({contentX, -29.f});
        m_page->addChild(empty);

        std::string hintText =
            std::string(TAB_NAMES[index]) + " options will appear here.";

        auto hint = label(hintText.c_str(), .30f, MUTED);
        hint->setOpacity(210);
        hint->setPosition({contentX, -53.f});
        hint->limitLabelWidth(265.f, .30f, .1f);
        m_page->addChild(hint);

        m_page->setScale(.985f);
        m_page->runAction(CCEaseExponentialOut::create(
            CCScaleTo::create(.12f, 1.f)
        ));
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
        if (m_closing)
            return;

        m_closing = true;
        this->setKeypadEnabled(false);
        this->setTouchEnabled(false);

        if (m_dim)
            m_dim->runAction(CCFadeTo::create(.12f, 0));

        auto finish = CCCallFunc::create(
            this, callfunc_selector(NovaMenu::finishClose)
        );

        m_root->runAction(CCSequence::create(
            CCEaseExponentialIn::create(
                CCScaleTo::create(.12f, .97f)
            ),
            finish,
            nullptr
        ));
    }

    void keyBackClicked() override {
        onClose(nullptr);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (m_closing)
            return true;

        auto p = m_root->convertToNodeSpace(
            touch->getLocation()
        );

        // Outside click.
        if (std::fabs(p.x) > MENU_W / 2.f ||
            std::fabs(p.y) > MENU_H / 2.f) {
            onClose(nullptr);
            return true;
        }

        // Header close button.
        if (std::fabs(p.x - (MENU_W / 2.f - 25.f)) < 17.f &&
            std::fabs(p.y - (MENU_H / 2.f - 27.f)) < 17.f) {
            onClose(nullptr);
            return true;
        }

        // Sidebar tabs.
        const float startY = 104.f;
        const float gap = 42.f;
        const float tabX = -MENU_W / 2.f + 82.f;

        if (std::fabs(p.x - tabX) < 76.f) {
            for (int i = 0; i < TAB_COUNT; ++i) {
                if (std::fabs(p.y - (startY - gap * i)) < 19.f) {
                    selectTab(i);
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
        if (!CCLayer::init())
            return false;

        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-300);
        setTouchEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_orb = CCNode::create();

        // Simple, understated floating button.
        m_orb->addChild(panel(46.f, 46.f, 23.f, ACCENT, .35f));
        m_orb->addChild(panel(40.f, 40.f, 20.f, BG));
        m_orb->addChild(panel(36.f, 36.f, 18.f, OUTLINE, .8f));

        auto icon = CCSprite::createWithSpriteFrameName(
            "GJ_demonIcon_001.png"
        );

        if (!icon) {
            icon = CCSprite::createWithSpriteFrameName(
                "GJ_starIcon_001.png"
            );
        }

        if (icon) {
            float dim = std::max(
                icon->getContentSize().width,
                icon->getContentSize().height
            );

            if (dim > 0.f)
                icon->setScale(22.f / dim);

            m_orb->addChild(icon);
        }

        float x = static_cast<float>(
            Mod::get()->getSavedValue<double>(
                "nova-bubble-x", 28.0
            )
        );

        float y = static_cast<float>(
            Mod::get()->getSavedValue<double>(
                "nova-bubble-y", win.height / 2.f
            )
        );

        x = std::clamp(x, 24.f, win.width - 24.f);
        y = std::clamp(y, 24.f, win.height - 24.f);

        m_orb->setPosition({x, y});
        addChild(m_orb);

        g_novaBubble = this;
        scheduleUpdate();

        return true;
    }

    void update(float dt) override {
        CCLayer::update(dt);

        if (g_novaOpen) {
            setVisible(false);
            return;
        }

        auto play = PlayLayer::get();
        setVisible(!play || play->m_isPaused);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!isVisible() || g_novaOpen)
            return false;

        auto p = convertToNodeSpace(touch->getLocation());

        if (ccpDistance(p, m_orb->getPosition()) > 27.f)
            return false;

        m_dragging = false;
        m_startTouch = p;
        m_grab = m_orb->getPosition() - p;

        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        auto p = convertToNodeSpace(touch->getLocation());

        if (!m_dragging && ccpDistance(p, m_startTouch) > 8.f)
            m_dragging = true;

        if (!m_dragging)
            return;

        auto win = CCDirector::sharedDirector()->getWinSize();
        auto pos = p + m_grab;

        pos.x = std::clamp(pos.x, 24.f, win.width - 24.f);
        pos.y = std::clamp(pos.y, 24.f, win.height - 24.f);

        m_orb->setPosition(pos);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_dragging) {
            if (auto menu = NovaMenu::create())
                menu->show();
            return;
        }

        auto pos = m_orb->getPosition();

        Mod::get()->setSavedValue<double>("nova-bubble-x", pos.x);
        Mod::get()->setSavedValue<double>("nova-bubble-y", pos.y);
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = false;
    }
};

class $modify(NovaMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init())
            return false;

        auto director = CCDirector::sharedDirector();

        if (!director->getNotificationNode())
            director->setNotificationNode(CCNode::create());

        auto notification = director->getNotificationNode();

        if (!notification->getChildByID("nova-floating-bubble")) {
            auto bubble = NovaBubble::create();
            bubble->setID("nova-floating-bubble");
            notification->addChild(bubble, 999);
        }

        return true;
    }
};
