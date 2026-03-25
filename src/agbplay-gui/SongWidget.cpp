#include "SongWidget.hpp"

#include "Types.hpp"

#include <fmt/core.h>

SongWidget::SongWidget(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(16 + 2 + 32);

    layout.addLayout(&upperLayout);
    layout.addSpacing(2);
    layout.addLayout(&lowerLayout);

    upperLayout.addStretch(1);
    lowerLayout.addStretch(1);

    QPalette labelPal;
    labelPal.setColor(QPalette::WindowText, QColor(255, 255, 255));

    QFont titleFont;
    titleFont.setUnderline(true);
    titleLabel.setFixedSize(320, 16);
    titleLabel.setFont(titleFont);
    titleLabel.setPalette(labelPal);
    titleLabel.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    upperLayout.addWidget(&titleLabel, 0);

    upperLayout.addSpacing(10);

    bpmLabel.setFixedSize(50, 16);
    bpmLabel.setPalette(labelPal);
    bpmLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&bpmLabel, 0);

    upperLayout.addSpacing(5);

    bpmFactorLabel.setFixedSize(60, 16);
    bpmFactorLabel.setPalette(labelPal);
    bpmFactorLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    upperLayout.addWidget(&bpmFactorLabel, 0);

    upperLayout.addSpacing(10);

    chnLabel.setFixedSize(70, 16);
    chnLabel.setPalette(labelPal);
    chnLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&chnLabel, 0);

    upperLayout.addSpacing(30);

    timeLabel.setFixedSize(30, 16);
    timeLabel.setPalette(labelPal);
    timeLabel.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    upperLayout.addWidget(&timeLabel, 0);

    QFont font;
    font.setPointSize(18);
    QPalette chordPal;
    chordPal.setColor(QPalette::WindowText, QColor("#37dcdc"));
    chordLabel.setFixedSize(128, 32);
    chordLabel.setPalette(labelPal);
    chordLabel.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    chordLabel.setFont(font);
    chordLabel.setFrameStyle(QFrame::Panel | QFrame::Plain);
    lowerLayout.addWidget(&chordLabel, 0);

    lowerLayout.addSpacing(10);

    keyboardWidget.setFixedHeight(32);
    keyboardWidget.setPressedColor(QColor(255, 150, 0));
    lowerLayout.addWidget(&keyboardWidget, 0);

    layout.setSpacing(0);
    layout.setContentsMargins(0, 0, 0, 0);

    upperLayout.addStretch(1);
    lowerLayout.addStretch(1);

    reset();
}

SongWidget::~SongWidget()
{
}

#define setFmtText(...) setText(QString::fromStdString(fmt::format(__VA_ARGS__)))

void SongWidget::setVisualizerState(const MP2KVisualizerStatePlayer &state, size_t activeChannels)
{
    if (oldBpm != state.bpm) {
        oldBpm = state.bpm;
        bpmLabel.setFmtText("{} BPM", state.bpm);
    }

    if (oldBpmFactor != state.bpmFactor) {
        oldBpmFactor = state.bpmFactor;
        bpmFactorLabel.setFmtText("(x {:.4})", state.bpmFactor);
    }

    if (oldTime != state.time) {
        oldTime = state.time;
        timeLabel.setFmtText("{:02}:{:02}", state.time / 60, state.time % 60);
    }

    if (oldActiveChannels != activeChannels) {
        oldActiveChannels = activeChannels;

        if (maxChannels < activeChannels)
            maxChannels = activeChannels;

        chnLabel.setFmtText("{}/{} Chn", activeChannels, maxChannels);
    }
}

void SongWidget::setPressed(const std::bitset<128> &pressed)
{
    keyboardWidget.setPressedKeys(pressed);
    setChord(pressed);
}

static bool isChord(uint32_t unifiedOctave, const std::vector<uint8_t> &keysToTest, size_t &keyResult, size_t start, size_t end)
{
    /* Find which chord is pressed:
     * - search only the lowest 12 keys of unifiedOctave
     * - test cyclically if all keysToTest are set for each base key */

    /* Make wraparound logic easier by just duplicating all pressed bits to one octave above. */
    unifiedOctave |= unifiedOctave << 12;

    /* Actually check whether the pressed keys correspond to the match candidate. */
    for (size_t key = start; key < end; key++) {
        bool success = true;
        for (uint8_t keyToTest : keysToTest) {
            if (!(unifiedOctave & (1 << (keyToTest + key)))) {
                success = false;
                break;
            }
        }

        if (success) {
            keyResult = key;
            return true;
        }
    }

    return false;
}

struct ChordMatch {
    const char *suffix;
    std::vector<uint8_t> keysToTest;
    bool ignoreRoot;
};

void SongWidget::setChord(const std::bitset<128> &pressed)
{
    static const char *keyTable[12] = {
        "C", "D♭", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B",
    };

    /* Check if no key is pressed. We impliclty also do this in the loop below, but any() uses popcnt and is faster. */
    if (!pressed.any()) {
        chordLabel.setText("");
        return;
    }

    /* C++20 does not yet have bit operators on bitfields, so perform manually on uint32_t.
     * We merge the 'pressed' bitfield into just a single octave, which discards octave information.
     * Octave information is for the most part irrelevant to determine chords for us. */
    size_t rootKey = pressed.size();
    uint32_t unifiedOctave = 0;

    for (size_t key = 0, wrappedKey = 0; key < pressed.size(); key++, wrappedKey++) {
        if (wrappedKey >= 12)
            wrappedKey -= 12;

        /* Because we checked earlier, at least one key must be pressed */
        if (rootKey == pressed.size() && pressed[key])
            rootKey = wrappedKey;

        unifiedOctave |= pressed[key] << wrappedKey;
    }

    size_t numKeysUnifiedOctave = 0;
    for (size_t i = 0; i < 12; i++) {
        if (unifiedOctave & (1 << i))
            numKeysUnifiedOctave += 1;
    }

    static const std::vector<std::vector<ChordMatch>> chordMatches = {
        {   // 0 pressed keys
            ChordMatch{"PRECONDITION ERROR", {}, false},
        },
        {   // 1 pressed key
            ChordMatch{" (pure)", {0}, false},
        },
        {   // 2 pressed keys
            ChordMatch{"5", {0, 7}, true},
            ChordMatch{"", {0, 4}, false},      // without perfect fifth
            ChordMatch{"m", {0, 3}, false},   // without perfect fifth
        },
        {   // 3 pressed keys
            ChordMatch{"", {0, 4, 7}, true},
            ChordMatch{"m", {0, 3, 7}, true},
            ChordMatch{"sus2", {0, 2, 7}, false},
            ChordMatch{"sus4", {0, 5, 7}, true},
            ChordMatch{"dim", {0, 3, 6}, true},
            ChordMatch{"aug", {0, 4, 8}, true},
            ChordMatch{"♯4", {0, 6, 7}, true}, // lydian
            ChordMatch{"♭2", {0, 1, 7}, true}, // phyrgian
            ChordMatch{"5add♭7", {0, 7, 10}, true}, // C7 or Cmin7 without third
            ChordMatch{"5add7", {0, 7, 11}, true}, // Cmaj7 without major third
        },
        {   // 4 pressed keys
            ChordMatch{"add9", {0, 2, 4, 7}, true},
            ChordMatch{"M7", {0, 4, 7, 11}, true},
            ChordMatch{"6", {0, 3, 7, 10}, false},
            ChordMatch{"m7", {0, 3, 7, 10}, true},
            ChordMatch{"7", {0, 4, 7, 10}, true},
            ChordMatch{"dim7", {0, 3, 6, 9}, false},
            ChordMatch{"m7♭5", {0, 3, 6, 10}, true},
            ChordMatch{"mM7", {0, 3, 7, 11}, true},
        },
        {
            // 5 pressed keys
            ChordMatch{"M9", {0, 4, 7, 11, 2}, true},
            ChordMatch{"9", {0, 4, 7, 10, 2}, true},
            ChordMatch{"m9", {0, 3, 7, 10, 2}, true},
        },
        // TODO implement more
    };

    if (numKeysUnifiedOctave >= chordMatches.size()) {
        chordLabel.setText("?");
        return;
    }

    for (const ChordMatch &m : chordMatches.at(numKeysUnifiedOctave)) {
        size_t identifiedKey;
        size_t keySearchStart = 0;
        size_t keySearchEnd = 12;

        if (!m.ignoreRoot) {
            keySearchStart = rootKey;
            keySearchEnd = rootKey + 1;
        }

        if (isChord(unifiedOctave, m.keysToTest, identifiedKey, keySearchStart, keySearchEnd)) {
            chordLabel.setFmtText("{}{}", keyTable[identifiedKey], m.suffix);
            return;
        }
    }

    chordLabel.setText("?");
}

#undef setFmtText

void SongWidget::reset()
{
    titleLabel.setText("No game loaded");
    bpmLabel.setText("150 BPM");
    bpmFactorLabel.setText("(x 1)");
    chnLabel.setText("0/0 Chn");
    timeLabel.setText("00:00");
    chordLabel.setText("---");
}

void SongWidget::resetMaxChannels()
{
    maxChannels = 0;
}
