#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace geode::prelude;

static bool g_open = false;

static constexpr int   TAB_COUNT = 7;
static constexpr float MENU_W = 450.f;
static constexpr float MENU_H = 280.f;
static constexpr float TAB_X = -158.f;
static constexpr float TAB_W = 104.f;
static constexpr float TAB_H = 26.f;
static constexpr float TAB_Y0 = 70.f;
static constexpr float TAB_GAP = 30.f;
static constexpr float COL_X = 65.f;
static constexpr float COL_W = 290.f;
static constexpr float ROW_W = 266.f;
static constexpr float ROW_H = 38.f;
static constexpr float ROW_Y0 = 50.f;
static constexpr float ROW_GAP = 46.f;
static constexpr float INPUT_OFFSET = ROW_W / 2.f - 52.f;

static const ccColor3B C_BLACK  = {0, 0, 0};
static const ccColor3B C_BG     = {24, 26, 41};
static const ccColor3B C_SIDE   = {18, 20, 33};
static const ccColor3B C_CARD   = {30, 33, 53};
static const ccColor3B C_ROW    = {42, 46, 74};
static const ccColor3B C_LINE   = {52, 58, 92};
static const ccColor3B C_OFF    = {66, 72, 108};
static const ccColor3B C_TEXT   = {240, 242, 255};
static const ccColor3B C_MUTED  = {140, 148, 186};
static const ccColor3B C_ACCENT = {112, 126, 255};

static const char* TAB_NAMES[TAB_COUNT] = {
    "Overall", "Player", "Level", "Bypass", "Visual", "Creator", "Settings"
};

static const char* TAB_SUBS[TAB_COUNT] = {
    "Everyday options for the whole game.",
    "Movement and player options.",
    "Level and practice tools.",
    "Restrictions and limits.",
    "Make the game look how you like.",
    "Editor helpers.",
    "Tune your Nova menu."
};

// On/off cheats. tab = index into TAB_NAMES. Saved as "nova-<key>".
struct Cheat { int tab; const char* key; const char* name; const char* desc; };
static constexpr int CHEAT_COUNT = 2;
static const Cheat CHEATS[CHEAT_COUNT] = {
    {1, "noclip",  "Noclip",        "Survive hazards without dying"},
    {1, "respawn", "Respawn Delay", "Wait before respawning"},
};

// Typed number boxes. Saved as "nova-<key>".
struct InputCfg { int tab; const char* key; const char* name; const char* desc; double def; };
static constexpr int INPUT_COUNT = 1;
static const InputCfg INPUTS[INPUT_COUNT] = {
    {1, "respawn-delay", "Delay (seconds)", "0 = instant respawn", 1.0},
};

static CCDrawNode* shape(float w, float h, float radius, ccColor3B color, float alpha = 1.f) {
    auto draw = CCDrawNode::create();
    float hw = w * .5f;
    float hh = h * .5f;
    float r = std::max(0.f, std::min(radius, std::min(hw, hh) - 0.05f));
    int seg = r > 12.f ? 10 : 7;
    const float cx[4] = {hw - r, -hw + r, -hw + r, hw - r};
    const float cy[4] = {hh - r, hh - r, -hh + r, -hh + r};
    std::vector<CCPoint> pts;
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= seg; ++i) {
            float a = (90.f * c + 90.f * i / seg) * 3.14159265f / 180.f;
            pts.push_back(CCPoint(cx[c] + r * std::cos(a), cy[c] + r * std::sin(a)));
        }
    }
    draw->drawPolygon(
        pts.data(), static_cast<unsigned int>(pts.size()),
        ccc4f(color.r / 255.f, color.g / 255.f, color.b / 255.f, alpha),
        0.f, ccc4f(0.f, 0.f, 0.f, 0.f)
    );
    return draw;
}

static CCLabelBMFont* big(const char* t, float s, ccColor3B c) {
    auto l = CCLabelBMFont::create(t, "bigFont.fnt");
    l->setScale(s);
    l->setColor(c);
    return l;
}

static CCLabelBMFont* chat(const char* t, float s, ccColor3B c) {
    auto l = CCLabelBMFont::create(t, "chatFont.fnt");
    l->setScale(s);
    l->setColor(c);
    return l;
}

class NovaMenu : public FLAlertLayer {
protected:
    struct ToggleRow {
        std::string key;
        CCNode* onTrack;
        CCNode* knob;
        float y;
        bool on;
    };

    CCNode* m_root = nullptr;
    CCNode* m_page = nullptr;
    CCNode* m_pill = nullptr;
    CCLayerColor* m_dim = nullptr;
    TextInput* m_input = nullptr;
    float m_inputY = 0.f;
    std::vector<CCLabelBMFont*> m_tabLabels;
    std::vector<ToggleRow> m_rows;
    int m_current = -1;
    bool m_closing = false;

    bool initMenu() {
        if (!FLAlertLayer::init(0)) return false;
        m_noElasticity = true;
        g_open = true;

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_dim = CCLayerColor::create(ccc4(0, 0, 0, 0));
        this->addChild(m_dim, -1);
        m_dim->runAction(CCFadeTo::create(.2f, 150));

        m_mainLayer = CCLayer::create();
        this->addChild(m_mainLayer);
        this->setTouchEnabled(true);
        this->setKeypadEnabled(true);

        m_root = CCNode::create();
        m_root->setPosition({win.width / 2.f, win.height / 2.f});
        m_mainLayer->addChild(m_root);

        const float pads[3] = {18.f, 11.f, 5.f};
        for (float p : pads) {
            auto sh = shape(MENU_W + p, MENU_H + p, 22.f, C_BLACK, .12f);
            sh->setPosition({0.f, -5.f});
            m_root->addChild(sh);
        }
        m_root->addChild(shape(MENU_W + 2.f, MENU_H + 2.f, 17.f, C_LINE));
        m_root->addChild(shape(MENU_W, MENU_H, 16.f, C_BG));

        auto side = shape(118.f, 256.f, 13.f, C_SIDE);
        side->setPosition({TAB_X, 0.f});
        m_root->addChild(side);

        auto brand = big("NOVA", .55f, C_ACCENT);
        brand->setPosition({TAB_X, 108.f});
        m_root->addChild(brand);

        auto brandLine = shape(88.f, 1.5f, .7f, C_LINE, .8f);
        brandLine->setPosition({TAB_X, 91.f});
        m_root->addChild(brandLine);

        m_pill = CCNode::create();
        m_pill->addChild(shape(TAB_W, TAB_H, 9.f, C_ACCENT, .30f));
        auto bar = shape(3.f, 14.f, 1.5f, C_ACCENT);
        bar->setPosition({-TAB_W / 2.f + 7.f, 0.f});
        m_pill->addChild(bar);
        m_pill->setPosition({TAB_X, TAB_Y0});
        m_root->addChild(m_pill);

        for (int i = 0; i < TAB_COUNT; ++i) {
            auto l = big(TAB_NAMES[i], .4f, C_MUTED);
            l->setAnchorPoint({0.f, .5f});
            l->limitLabelWidth(70.f, .4f, .1f);
            l->setPosition({TAB_X - TAB_W / 2.f + 18.f, TAB_Y0 - TAB_GAP * i});
            m_root->addChild(l);
            m_tabLabels.push_back(l);
        }

        auto closeBg = shape(24.f, 24.f, 12.f, C_CARD);
        closeBg->setPosition({200.f, 116.f});
        m_root->addChild(closeBg);
        auto closeX = big("X", .34f, C_MUTED);
        closeX->setPosition({200.f, 116.f});
        m_root->addChild(closeX);

        auto ver = chat("Nova v2.4", .5f, C_MUTED);
        ver->setAnchorPoint({1.f, .5f});
        ver->setOpacity(150);
        ver->setPosition({210.f, -127.f});
        m_root->addChild(ver);

        m_root->setScale(.94f);
        m_root->runAction(CCEaseBackOut::create(CCScaleTo::create(.24f, 1.f)));

        int last = Mod::get()->getSavedValue<int>("nova-last-tab", 0);
        selectTab(std::clamp(last, 0, TAB_COUNT - 1));
        return true;
    }

    void selectTab(int index) {
        if (index < 0 || index >= TAB_COUNT || index == m_current) return;
        bool first = m_current < 0;
        m_current = index;

        float y = TAB_Y0 - TAB_GAP * index;
        m_pill->stopAllActions();
        if (first) {
            m_pill->setPosition({TAB_X, y});
        } else {
            m_pill->runAction(CCEaseExponentialOut::create(
                CCMoveTo::create(.22f, ccp(TAB_X, y))));
        }

        for (int i = 0; i < TAB_COUNT; ++i) {
            m_tabLabels[i]->setColor(i == index ? C_TEXT : C_MUTED);
        }

        buildPage(index, !first);
        Mod::get()->setSavedValue<int>("nova-last-tab", index);
    }

    void buildPage(int index, bool animate) {
        if (m_input) {
            m_input->defocus();
            m_input = nullptr;
        }
        if (m_page) m_page->removeFromParentAndCleanup(true);
        m_rows.clear();
        m_page = CCNode::create();
        m_root->addChild(m_page);

        auto title = big(TAB_NAMES[index], .6f, C_TEXT);
        title->setAnchorPoint({0.f, .5f});
        title->setPosition({-80.f, 112.f});
        m_page->addChild(title);

        auto sub = chat(TAB_SUBS[index], .55f, C_MUTED);
        sub->setAnchorPoint({0.f, .5f});
        sub->setPosition({-80.f, 94.f});
        m_page->addChild(sub);

        auto div = shape(COL_W, 1.5f, .7f, C_LINE, .7f);
        div->setPosition({COL_X, 84.f});
        m_page->addChild(div);

        auto card = shape(COL_W, 184.f, 13.f, C_CARD);
        card->setPosition({COL_X, -16.f});
        m_page->addChild(card);

        int count = 0;

        for (int c = 0; c < CHEAT_COUNT; ++c) {
            const Cheat& cheat = CHEATS[c];
            if (cheat.tab != index) continue;

            float y = ROW_Y0 - ROW_GAP * count;
            ++count;

            auto bg = shape(ROW_W, ROW_H, 11.f, C_ROW);
            bg->setPosition({COL_X, y});
            m_page->addChild(bg);

            auto name = big(cheat.name, .42f, C_TEXT);
            name->setAnchorPoint({0.f, .5f});
            name->limitLabelWidth(150.f, .42f, .1f);
            name->setPosition({COL_X - ROW_W / 2.f + 14.f, y + 7.f});
            m_page->addChild(name);

            auto desc = chat(cheat.desc, .5f, C_MUTED);
            desc->setAnchorPoint({0.f, .5f});
            desc->limitLabelWidth(170.f, .5f, .1f);
            desc->setPosition({COL_X - ROW_W / 2.f + 14.f, y - 8.f});
            m_page->addChild(desc);

            bool on = Mod::get()->getSavedValue<bool>(
                std::string("nova-") + cheat.key, false);

            auto tg = CCNode::create();
            tg->setPosition({COL_X + ROW_W / 2.f - 32.f, y});
            auto offTrack = shape(38.f, 20.f, 10.f, C_OFF);
            auto onTrack = shape(38.f, 20.f, 10.f, C_ACCENT);
            onTrack->setVisible(on);
            auto knob = shape(14.f, 14.f, 7.f, C_TEXT);
            knob->setPosition({on ? 9.f : -9.f, 0.f});
            tg->addChild(offTrack);
            tg->addChild(onTrack);
            tg->addChild(knob);
            m_page->addChild(tg);

            m_rows.push_back({std::string(cheat.key), onTrack, knob, y, on});
        }

        for (int n = 0; n < INPUT_COUNT; ++n) {
            const InputCfg& cfg = INPUTS[n];
            if (cfg.tab != index) continue;

            float y = ROW_Y0 - ROW_GAP * count;
            ++count;

            auto bg = shape(ROW_W, ROW_H, 11.f, C_ROW);
            bg->setPosition({COL_X, y});
            m_page->addChild(bg);

            auto name = big(cfg.name, .42f, C_TEXT);
            name->setAnchorPoint({0.f, .5f});
            name->limitLabelWidth(120.f, .42f, .1f);
            name->setPosition({COL_X - ROW_W / 2.f + 14.f, y + 7.f});
            m_page->addChild(name);

            auto desc = chat(cfg.desc, .5f, C_MUTED);
            desc->setAnchorPoint({0.f, .5f});
            desc->limitLabelWidth(120.f, .5f, .1f);
            desc->setPosition({COL_X - ROW_W / 2.f + 14.f, y - 8.f});
            m_page->addChild(desc);

            double saved = Mod::get()->getSavedValue<double>(
                std::string("nova-") + cfg.key, cfg.def);
            char buf[24];
            std::snprintf(buf, sizeof(buf), "%g", saved);

            auto input = TextInput::create(70.f, "0", "bigFont.fnt");
            input->setFilter("0123456789.");
            input->setMaxCharCount(5);
            input->setString(buf);
            input->setCallback([key = std::string("nova-") + cfg.key](std::string const& s) {
                double v = s.empty() ? 0.0 : std::strtod(s.c_str(), nullptr);
                v = std::clamp(v, 0.0, 10.0);
                Mod::get()->setSavedValue<double>(key, v);
            });
            input->setScale(.85f);
            input->setPosition({COL_X + INPUT_OFFSET, y});
            m_page->addChild(input);

            m_input = input;
            m_inputY = y;
        }

        if (count == 0) {
            auto ringOuter = shape(46.f, 46.f, 23.f, C_ACCENT, .55f);
            ringOuter->setPosition({COL_X, 10.f});
            m_page->addChild(ringOuter);
            auto ringInner = shape(40.f, 40.f, 20.f, C_BG);
            ringInner->setPosition({COL_X, 10.f});
            m_page->addChild(ringInner);

            std::string letter(1, TAB_NAMES[index][0]);
            auto initial = big(letter.c_str(), .6f, C_ACCENT);
            initial->setPosition({COL_X, 10.f});
            m_page->addChild(initial);

            auto empty = big("Nothing here yet", .42f, C_TEXT);
            empty->setPosition({COL_X, -26.f});
            m_page->addChild(empty);

            std::string hintText = std::string(TAB_NAMES[index]) + " options will appear here";
            auto hint = chat(hintText.c_str(), .55f, C_MUTED);
            hint->setPosition({COL_X, -45.f});
            m_page->addChild(hint);
        }

        if (animate) {
            m_page->setPosition({14.f, 0.f});
            m_page->runAction(CCEaseExponentialOut::create(
                CCMoveTo::create(.2f, ccp(0.f, 0.f))));
        }
    }

    void flipRow(ToggleRow& r) {
        r.on = !r.on;
        r.onTrack->setVisible(r.on);
        r.knob->stopAllActions();
        r.knob->runAction(CCEaseExponentialOut::create(
            CCMoveTo::create(.18f, ccp(r.on ? 9.f : -9.f, 0.f))));
        Mod::get()->setSavedValue<bool>("nova-" + r.key, r.on);
    }

public:
    ~NovaMenu() { g_open = false; }

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
        g_open = false;
        this->removeFromParentAndCleanup(true);
    }

    void onClose(CCObject*) {
        if (m_closing) return;
        m_closing = true;
        if (m_input) m_input->defocus();
        this->setKeypadEnabled(false);
        this->setTouchEnabled(false);
        m_dim->runAction(CCFadeTo::create(.14f, 0));
        auto done = CCCallFunc::create(this, callfunc_selector(NovaMenu::finishClose));
        m_root->runAction(CCSequence::create(
            CCEaseExponentialIn::create(CCScaleTo::create(.14f, .94f)),
            done, nullptr));
    }

    void keyBackClicked() override { this->onClose(nullptr); }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (m_closing) return true;
        auto p = m_root->convertToNodeSpace(touch->getLocation());

        if (std::fabs(p.x) > MENU_W / 2.f + 4.f || std::fabs(p.y) > MENU_H / 2.f + 4.f) {
            this->onClose(nullptr);
            return true;
        }
        if (ccpDistance(p, ccp(200.f, 116.f)) < 16.f) {
            this->onClose(nullptr);
            return true;
        }

        // let taps on the typing box reach the text input
        if (m_input &&
            std::fabs(p.x - (COL_X + INPUT_OFFSET)) < 46.f &&
            std::fabs(p.y - m_inputY) < 18.f) {
            return false;
        }
        if (m_input) m_input->defocus();

        if (std::fabs(p.x - TAB_X) < TAB_W / 2.f + 4.f) {
            for (int i = 0; i < TAB_COUNT; ++i) {
                if (std::fabs(p.y - (TAB_Y0 - TAB_GAP * i)) < TAB_GAP / 2.f) {
                    this->selectTab(i);
                    break;
                }
            }
            return true;
        }
        if (std::fabs(p.x - COL_X) < ROW_W / 2.f) {
            for (auto& row : m_rows) {
                if (std::fabs(p.y - row.y) < ROW_H / 2.f) {
                    this->flipRow(row);
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
    CCPoint m_start;
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
        this->setTouchPriority(-600);
        this->setTouchEnabled(true);

        auto win = CCDirector::sharedDirector()->getWinSize();

        m_orb = CCNode::create();
        auto sh = shape(54.f, 54.f, 27.f, C_BLACK, .25f);
        sh->setPosition({0.f, -2.f});
        m_orb->addChild(sh);
        m_orb->addChild(shape(50.f, 50.f, 25.f, C_ACCENT, .45f));
        m_orb->addChild(shape(44.f, 44.f, 22.f, C_BG));
        m_orb->addChild(shape(38.f, 38.f, 19.f, C_CARD));
        m_orb->addChild(big("N", .55f, C_TEXT));

        float x = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-x", 30.0));
        float y = static_cast<float>(Mod::get()->getSavedValue<double>("nova-bubble-y", win.height / 2.f));
        x = std::clamp(x, 26.f, win.width - 26.f);
        y = std::clamp(y, 26.f, win.height - 26.f);
        m_orb->setPosition({x, y});
        this->addChild(m_orb);

        this->scheduleUpdate();
        return true;
    }

    void update(float dt) override {
        CCLayer::update(dt);
        if (g_open) {
            this->setVisible(false);
            return;
        }
        bool inLevel = PlayLayer::get() != nullptr;
        auto scene = CCScene::get();
        bool paused = scene && scene->getChildByType<PauseLayer>(0) != nullptr;
        this->setVisible(!inLevel || paused);
    }

    bool ccTouchBegan(CCTouch* t, CCEvent*) override {
        if (!this->isVisible() || g_open) return false;
        auto p = this->convertToNodeSpace(t->getLocation());
        if (ccpDistance(p, m_orb->getPosition()) > 28.f) return false;
        m_dragging = false;
        m_start = p;
        m_grab = m_orb->getPosition() - p;
        m_orb->stopAllActions();
        m_orb->runAction(CCEaseExponentialOut::create(CCScaleTo::create(.12f, 1.1f)));
        return true;
    }

    void ccTouchMoved(CCTouch* t, CCEvent*) override {
        auto p = this->convertToNodeSpace(t->getLocation());
        if (!m_dragging && ccpDistance(p, m_start) > 8.f) m_dragging = true;
        if (!m_dragging) return;
        auto win = CCDirector::sharedDirector()->getWinSize();
        auto np = p + m_grab;
        np.x = std::clamp(np.x, 26.f, win.width - 26.f);
        np.y = std::clamp(np.y, 26.f, win.height - 26.f);
        m_orb->setPosition(np);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        m_orb->stopAllActions();
        m_orb->runAction(CCEaseBackOut::create(CCScaleTo::create(.2f, 1.f)));
        if (!m_dragging) {
            if (auto menu = NovaMenu::create()) menu->show();
            return;
        }
        auto pos = m_orb->getPosition();
        Mod::get()->setSavedValue<double>("nova-bubble-x", pos.x);
        Mod::get()->setSavedValue<double>("nova-bubble-y", pos.y);
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = false;
        m_orb->stopAllActions();
        m_orb->setScale(1.f);
    }
};

class $modify(NovaMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto director = CCDirector::sharedDirector();
        if (!director->getNotificationNode()) {
            director->setNotificationNode(CCNode::create());
        }
        auto holder = director->getNotificationNode();
        if (!holder->getChildByID("nova-floating-bubble")) {
            auto bubble = NovaBubble::create();
            bubble->setID("nova-floating-bubble");
            holder->addChild(bubble, 999);
        }
        return true;
    }
};

// Noclip + Respawn Delay
class $modify(NovaPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        auto mod = Mod::get();

        // Noclip: skip the death completely
        if (mod->getSavedValue<bool>("nova-noclip", false)) return;

        PlayLayer::destroyPlayer(player, obj);

        // Respawn Delay: replace the game's 1 second wait with ours
        if (!mod->getSavedValue<bool>("nova-respawn", false)) return;
        bool dead = (m_player1 && m_player1->m_isDead) ||
                    (m_player2 && m_player2->m_isDead);
        if (!dead) return;

        double d = mod->getSavedValue<double>("nova-respawn-delay", 1.0);
        float delay = static_cast<float>(std::clamp(d, 0.0, 10.0));

        this->stopActionByTag(16);
        auto seq = CCSequence::create(
            CCDelayTime::create(delay),
            CCCallFunc::create(this, callfunc_selector(PlayLayer::delayedResetLevel)),
            nullptr
        );
        seq->setTag(16);
        this->runAction(seq);
    }
};
