#include "PerfUI/JournalWindow.h"
#include <algorithm>

namespace PerfUI {

JournalWindow::JournalWindow(std::string name)
    : Panel(std::move(name))
{
    setFocusable(true);
    initQuestData();
    buildUI();
}

void JournalWindow::initQuestData() {
    m_quests = {
        {
            "The Way of the Voice",
            "MAIN QUEST",
            "High Hrothgar",
            "I have been summoned by the Greybeards to their monastery at High Hrothgar on the slopes of the Throat of the World. They wish to speak to me about being Dragonborn.",
            {
                { "Speak to the Greybeards", true },
                { "Demonstrate your 'Unrelenting Force' Shout", true },
                { "Speak to Arngeir", false },
                { "Learn the word of power from Einarth", false }
            }
        },
        {
            "Bleak Falls Barrow",
            "MAIN QUEST",
            "Whiterun Hold",
            "Farengar Secret-Fire, the court wizard in Dragonsreach, has asked me to retrieve a Dragonstone from the ancient Nordic ruins of Bleak Falls Barrow.",
            {
                { "Retrieve the Dragonstone", true },
                { "Deliver the Dragonstone to Farengar", false }
            }
        },
        {
            "Dragon Rising",
            "MAIN QUEST",
            "Western Watchtower",
            "A dragon has attacked the Western Watchtower outside Whiterun. Jarl Balgruuf has dispatched Irileth and a squad of Whiterun Guards to investigate.",
            {
                { "Meet Irileth near the Western Watchtower", true },
                { "Slay the dragon Mirmulnir", true },
                { "Investigate the dragon", true },
                { "Report back to Jarl Balgruuf", false }
            }
        },
        {
            "The Golden Claw",
            "SIDE QUEST",
            "Riverwood",
            "Lucan Valerius of the Riverwood Trader had a valuable golden claw stolen by bandits who fled to Bleak Falls Barrow. He wants me to recover it.",
            {
                { "Find the secret of Bleak Falls Barrow", true },
                { "Find the Golden Claw", true },
                { "Bring the claw to Lucan", false }
            }
        },
        {
            "A Night to Remember",
            "DAEDRIC QUEST",
            "The Bannered Mare",
            "I engaged in a drinking contest with a mysterious stranger named Sam Guevenne. I woke up with a massive headache and an angry Priestess of Dibella.",
            {
                { "Help clean up the Temple of Dibella", true },
                { "Ask about Sam and the staff in Rorikstead", true },
                { "Talk to Ysolda in Whiterun", false },
                { "Find Sam Guevenne", false }
            }
        },
        {
            "Discerning the Transmundane",
            "DAEDRIC QUEST",
            "Septimus Signus' Outpost",
            "Septimus Signus, an eccentric hermit living in an ice cavern north of the College of Winterhold, believes he can unlock a mysterious Dwemer lockbox.",
            {
                { "Inscribe the Lexicon in Blackreach", true },
                { "Deliver the Lexicon to Septimus", true },
                { "Harvest blood of High Elf, Wood Elf, Dark Elf, Orc, and Falmer", false }
            }
        },
        {
            "Elder Knowledge",
            "MAIN QUEST",
            "Tower of Mzark",
            "To learn the Dragonrend shout, Paarthurnax says I must find an Elder Scroll. The College of Winterhold may have information on where one might be hidden.",
            {
                { "Talk to Urag gro-Shub at the College", true },
                { "Locate the Elder Scroll in Blackreach", false }
            }
        },
        {
            "Alduin's Wall",
            "MAIN QUEST",
            "Sky Haven Temple",
            "Delphine and Esbern believe the ancient Blades temple in the Reach holds the secret to defeating Alduin.",
            {
                { "Escort Esbern to Riverwood", true },
                { "Gain entrance to Sky Haven Temple", false }
            }
        },
        {
            "The Horn of Jurgen Windcaller",
            "MAIN QUEST",
            "Ustengrav",
            "The Greybeards sent me to retrieve the Horn of Jurgen Windcaller from Ustengrav to complete my initiation.",
            {
                { "Retrieve the Horn of Jurgen Windcaller", false }
            }
        },
        {
            "No Stone Unturned",
            "MISCELLANEOUS",
            "Skyrim",
            "I found an unusual gem that glows faintly. Vex in the Thieves Guild informed me it is one of the Stones of Barenziah.",
            {
                { "Recover the 24 Stones of Barenziah (8/24)", false }
            }
        }
    };
}

void JournalWindow::buildUI() {
    // Window Styling
    backgroundColor(Color::BackgroundBase());
    borderColor(Color::BorderFocus());
    borderWidth(2.0f);
    cornerRadius(12.0f);
    shadow(true, Color(0, 0, 0, 190), 28.0f, { 0.0f, 8.0f });

    layout()
        .direction(LayoutDirection::Vertical)
        .padding(20.0f)
        .gap(14.0f);

    // 1. Top Header Row
    auto* header = add<Panel>("Header");
    header->backgroundColor(Color::Transparent())
           .borderWidth(0.0f);
    header->layout()
           .direction(LayoutDirection::Horizontal)
           .justify(JustifyContent::SpaceBetween)
           .alignment(Alignment::Center);

    auto* titleStack = header->add<Panel>("TitleStack");
    titleStack->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    titleStack->layout().direction(LayoutDirection::Vertical).gap(4.0f);

    auto* titleText = titleStack->add<Text>("PERFUI - SKYRIM SE JOURNAL");
    titleText->color(Color::NordicGold()).fontSize(20.0f).bold(true);

    auto* subTitleText = titleStack->add<Text>("Retained-Mode C++20 UI | Layout Engine & Widget Demo");
    subTitleText->color(Color::TextSecondary()).fontSize(12.0f);

    auto* badge = header->add<Panel>("StatusBadge");
    badge->backgroundColor(Color(25, 35, 48, 220))
         .borderColor(Color::BorderSubtle())
         .borderWidth(1.0f)
         .cornerRadius(6.0f);
    badge->layout().padding(10.0f, 5.0f);
    auto* badgeText = badge->add<Text>("D3D11 / Hook Active");
    badgeText->color(Color::TextSuccess()).fontSize(12.0f).bold(true);

    // 2. Horizontal Divider
    auto* divider = add<Panel>("Divider");
    divider->backgroundColor(Color::BorderSubtle())
           .borderWidth(0.0f);
    divider->layout().height(1.0f);

    // 3. Main Two-Column Layout (Sidebar + Details)
    auto* mainBody = add<Panel>("MainBody");
    mainBody->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    mainBody->layout()
            .flex(1.0f)
            .direction(LayoutDirection::Horizontal)
            .gap(16.0f);

    // 3A. Left Sidebar (Quest List)
    auto* sidebar = mainBody->add<Panel>("Sidebar");
    sidebar->backgroundColor(Color(18, 22, 29, 180))
           .borderColor(Color::BorderSubtle())
           .borderWidth(1.0f)
           .cornerRadius(8.0f);
    sidebar->layout()
           .width(250.0f)
           .direction(LayoutDirection::Vertical)
           .padding(10.0f)
           .gap(8.0f);

    auto* listHeader = sidebar->add<Text>("ACTIVE QUESTS");
    listHeader->color(Color::TextAccent()).fontSize(13.0f).bold(true);

    auto* questScrollView = sidebar->add<ScrollView>("QuestScrollView");
    questScrollView->layout().flex(1.0f).gap(6.0f);

    for (size_t i = 0; i < m_quests.size(); ++i) {
        auto* btn = questScrollView->add<Button>(m_quests[i].title);
        btn->layout().width(DimensionConstraint::Flex(1.0f)).padding(10.0f, 8.0f);
        btn->fontSize(13.0f);
        btn->normalColor(Color(24, 30, 40, 220), Color::BorderSubtle(), Color::TextPrimary());
        btn->hoverColor(Color(36, 46, 62, 240), Color::BorderFocus(), Color::White());
        btn->pressedColor(Color::BackgroundActive(), Color::BorderFocus(), Color::White());

        btn->onClick([this, i]() {
            selectQuest(i);
        });

        m_questButtons.push_back(btn);
    }

    // 3B. Right Details Panel
    auto* details = mainBody->add<Panel>("DetailsPanel");
    details->backgroundColor(Color::BackgroundElevated())
           .borderColor(Color::BorderStrong())
           .borderWidth(1.0f)
           .cornerRadius(8.0f);
    details->layout()
           .flex(1.0f)
           .direction(LayoutDirection::Vertical)
           .padding(18.0f)
           .gap(12.0f);

    // Tags Row
    auto* tagRow = details->add<Panel>("TagRow");
    tagRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    tagRow->layout().direction(LayoutDirection::Horizontal).gap(8.0f);

    auto* catBadge = tagRow->add<Panel>("CatBadge");
    catBadge->backgroundColor(Color(45, 38, 20, 230))
            .borderColor(Color::NordicGold())
            .borderWidth(1.0f)
            .cornerRadius(4.0f);
    catBadge->layout().padding(8.0f, 3.0f);
    m_questCategoryText = catBadge->add<Text>("MAIN QUEST");
    m_questCategoryText->color(Color::TextAccent()).fontSize(11.0f).bold(true);

    auto* locBadge = tagRow->add<Panel>("LocBadge");
    locBadge->backgroundColor(Color(25, 32, 42, 230))
            .borderColor(Color::BorderSubtle())
            .borderWidth(1.0f)
            .cornerRadius(4.0f);
    locBadge->layout().padding(8.0f, 3.0f);
    m_questLocationText = locBadge->add<Text>("High Hrothgar");
    m_questLocationText->color(Color::TextSecondary()).fontSize(11.0f);

    // Title
    m_questTitleText = details->add<Text>("The Way of the Voice");
    m_questTitleText->color(Color::TextPrimary()).fontSize(19.0f).bold(true);

    // Inner Divider
    auto* innerDivider = details->add<Panel>("InnerDivider");
    innerDivider->backgroundColor(Color::BorderSubtle()).borderWidth(0.0f);
    innerDivider->layout().height(1.0f);

    // Description (Word wrapped)
    m_questDescText = details->add<Text>("Description placeholder...");
    m_questDescText->wrap(true);
    m_questDescText->color(Color(200, 205, 215, 255)).fontSize(14.0f);

    // Objectives Section
    auto* objHeader = details->add<Text>("OBJECTIVES:");
    objHeader->color(Color::TextAccent()).fontSize(13.0f).bold(true);

    m_objectivesContainer = details->add<Panel>("ObjectivesContainer");
    m_objectivesContainer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    m_objectivesContainer->layout().direction(LayoutDirection::Vertical).gap(6.0f);

    for (int i = 0; i < 4; ++i) {
        auto* objText = m_objectivesContainer->add<Text>("");
        objText->fontSize(13.0f);
        m_objectiveTexts.push_back(objText);
    }

    // Action Buttons at bottom
    auto* actionRow = details->add<Panel>("ActionRow");
    actionRow->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    actionRow->layout()
             .direction(LayoutDirection::Horizontal)
             .justify(JustifyContent::End)
             .gap(10.0f)
             .margin(0.0f, 10.0f, 0.0f, 0.0f);

    auto* setActiveBtn = actionRow->add<Button>("Set Active Quest");
    setActiveBtn->layout().padding(14.0f, 6.0f);
    setActiveBtn->fontSize(12.0f);
    setActiveBtn->normalColor(Color(35, 55, 35, 230), Color::TextSuccess(), Color::White());
    setActiveBtn->hoverColor(Color(45, 75, 45, 255), Color::TextSuccess(), Color::White());

    auto* showMapBtn = actionRow->add<Button>("Show on Map");
    showMapBtn->layout().padding(14.0f, 6.0f);
    showMapBtn->fontSize(12.0f);
    showMapBtn->normalColor(Color(30, 40, 55, 230), Color::BorderSubtle(), Color::TextPrimary());

    // 4. Footer Hint
    auto* footer = add<Panel>("Footer");
    footer->backgroundColor(Color::Transparent()).borderWidth(0.0f);
    footer->layout().direction(LayoutDirection::Horizontal).justify(JustifyContent::SpaceBetween);

    auto* hintText = footer->add<Text>("[F11] Toggle   [ESC] Close   [Scroll Wheel] Scroll Quests   [LMB] Select");
    hintText->color(Color(110, 120, 135, 255)).fontSize(12.0f);

    updateDetailsPanel();
}

void JournalWindow::selectQuest(size_t index) {
    if (index >= m_quests.size()) return;
    m_selectedIndex = index;

    // Highlight selected button
    for (size_t i = 0; i < m_questButtons.size(); ++i) {
        if (i == index) {
            m_questButtons[i]->normalColor(Color(45, 58, 78, 250), Color::BorderFocus(), Color::White());
        } else {
            m_questButtons[i]->normalColor(Color(24, 30, 40, 220), Color::BorderSubtle(), Color::TextPrimary());
        }
    }

    updateDetailsPanel();
}

void JournalWindow::updateDetailsPanel() {
    if (m_selectedIndex >= m_quests.size()) return;

    const auto& quest = m_quests[m_selectedIndex];
    if (m_questCategoryText) m_questCategoryText->text(quest.category);
    if (m_questLocationText) m_questLocationText->text(quest.location);
    if (m_questTitleText) m_questTitleText->text(quest.title);
    if (m_questDescText) m_questDescText->text(quest.description);

    for (size_t i = 0; i < m_objectiveTexts.size(); ++i) {
        if (i < quest.objectives.size()) {
            m_objectiveTexts[i]->setVisible(true);
            std::string status = quest.objectives[i].second ? "[Completed] " : "[Incomplete] ";
            m_objectiveTexts[i]->text(status + quest.objectives[i].first);
            m_objectiveTexts[i]->color(quest.objectives[i].second ? Color::TextSuccess() : Color(200, 205, 215, 255));
        } else {
            m_objectiveTexts[i]->setVisible(false);
        }
    }

    markLayoutDirty();
}

void JournalWindow::measure(Dimensions availableSize) {
    float targetW = (std::min)(availableSize.width * 0.90f, 920.0f);
    float targetH = (std::min)(availableSize.height * 0.88f, 560.0f);
    targetW = (std::max)(targetW, 600.0f);
    targetH = (std::max)(targetH, 380.0f);

    layout().width(targetW).height(targetH);
    Panel::measure(availableSize);
}

void JournalWindow::arrange(const Rect& finalRect) {
    float w = layout().width().value;
    float h = layout().height().value;
    float x = (std::max)(0.0f, (finalRect.width - w) * 0.5f);
    float y = (std::max)(0.0f, (finalRect.height - h) * 0.5f);

    Panel::arrange(Rect{ x, y, w, h });
}

} // namespace PerfUI
