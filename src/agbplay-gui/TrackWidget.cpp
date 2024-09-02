#include "TrackWidget.h"

#include <fmt/core.h>

#include "Types.h"

TrackWidget::TrackWidget(size_t trackNo, QWidget *parent)
    : QWidget(parent), trackNo(trackNo)
{
    setFixedHeight(36);

    layout.setRowMinimumHeight(0, 4);
    layout.setRowStretch(0, 0);

    layout.setColumnStretch(COL_PADL, 1);

    setAudible(true);

    /* Label */
    trackNoLabel.setFixedSize(24, 32);
    trackNoLabel.setAutoFillBackground(true);
    trackNoLabel.setText(QString::number(trackNo));
    trackNoLabel.setAlignment(Qt::AlignCenter);
    trackNoLabel.setLineWidth(1);
    trackNoLabel.setMidLineWidth(1);
    trackNoLabel.setFrameStyle(QFrame::Box | QFrame::Raised);
    layout.addWidget(&trackNoLabel, 1, COL_LABEL, 2, 1);

    /* Spacer */
    layout.setColumnStretch(COL_SPACE1, 0);
    layout.setColumnMinimumWidth(COL_SPACE1, 6);

    /* Buttons */
    connect(&muteButton, &QAbstractButton::clicked, [this](bool) { setMuted(!isMuted()); });
    muteButton.setFixedSize(16, 16);
    muteButton.setText("M");
    muteButton.setToolTip("Mute/Unmute Track");
    setMuted(false, true);
    layout.addWidget(&muteButton, 1, COL_BUTTONS1);

    connect(&soloButton, &QAbstractButton::clicked, [this](bool) { setSolo(!isSolo()); });
    soloButton.setFixedSize(16, 16);
    soloButton.setText("S");
    soloButton.setToolTip("Solo/De-solo Track");
    setSolo(false, true);
    layout.addWidget(&soloButton, 2, COL_BUTTONS1);

    connect(&analyzerButton, &QAbstractButton::clicked, [this](bool) { setAnalyzer(!isAnalyzing()); });
    analyzerButton.setFixedSize(16, 16);
    analyzerButton.setText("A");
    analyzerButton.setToolTip("Enable/Disable Chord Analyzer");
    setAnalyzer(false);
    layout.addWidget(&analyzerButton, 1, COL_BUTTONS2);
    
    /* Spacer */
    layout.setColumnStretch(COL_SPACE2, 0);
    layout.setColumnMinimumWidth(COL_SPACE2, 2);

    /* Labels */
    posLabel.setFixedSize(64, 16);
    posLabel.setFont(labelMonospaceFont);
    posLabel.setText("0x0000000");
    posLabel.setToolTip("Track Position");
    posLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&posLabel, 1, COL_INST, 1, 5);

    restLabel.setFixedSize(20, 16);
    restLabel.setFont(labelFont);
    restLabel.setText("0");
    restLabel.setToolTip("Note Rest");
    restLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&restLabel, 1, COL_MOD);

    voiceTypeLabel.setFixedSize(35, 16);
    voiceTypeLabel.setFont(labelFont);
    voiceTypeLabel.setText("-");
    voiceTypeLabel.setToolTip("Voice Type");
    voiceTypeLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&voiceTypeLabel, 1, COL_PITCH);

    instNoLabel.setFixedSize(20, 16);
    instNoLabel.setFont(labelFont);
    instNoLabel.setText("0");
    instNoLabel.setToolTip("Instrument Number");
    instNoLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&instNoLabel, 2, COL_INST);

    layout.setColumnStretch(COL_SPACE3, 0);
    layout.setColumnMinimumWidth(COL_SPACE3, 2);

    volLabel.setFixedSize(20, 16);
    volLabel.setFont(labelFont);
    volLabel.setText("0");
    volLabel.setToolTip("Volume");
    volLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&volLabel, 2, COL_VOL);

    layout.setColumnStretch(COL_SPACE4, 0);
    layout.setColumnMinimumWidth(COL_SPACE4, 2);

    panLabel.setFixedSize(20, 16);
    panLabel.setFont(labelFont);
    panLabel.setText("C");
    panLabel.setToolTip("Pan");
    panLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&panLabel, 2, COL_PAN);

    layout.setColumnStretch(COL_SPACE5, 0);
    layout.setColumnMinimumWidth(COL_SPACE5, 2);

    modLabel.setFixedSize(20, 16);
    modLabel.setFont(labelFont);
    modLabel.setText("0");
    modLabel.setToolTip("LFO Modulation");
    modLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&modLabel, 2, COL_MOD);

    layout.setColumnStretch(COL_SPACE6, 0);
    layout.setColumnMinimumWidth(COL_SPACE6, 2);

    pitchLabel.setFixedSize(35, 16);
    pitchLabel.setFont(labelFont);
    pitchLabel.setText("0");
    pitchLabel.setToolTip("Final Pitch");
    pitchLabel.setAlignment(Qt::AlignRight);
    layout.addWidget(&pitchLabel, 2, COL_PITCH);

    /* Spacer */
    layout.setColumnStretch(COL_SPACE5, 0);
    layout.setColumnMinimumWidth(COL_SPACE7, 10);

    /* Keyboard and VU bars */
    layout.addLayout(&vuBarKeyboardLayout, 1, COL_KEYS, 2, 1);
    layout.setContentsMargins(0, 0, 0, 0);
    layout.setHorizontalSpacing(0);
    layout.setVerticalSpacing(0);

    vuBarKeyboardLayout.addWidget(&keyboardWidget);
    vuBarKeyboardLayout.addWidget(&vuBarWidget);
    vuBarKeyboardLayout.setContentsMargins(0, 0, 0, 0);
    vuBarKeyboardLayout.setSpacing(0);

    vuBarLayout.addWidget(&vuBarWidgetLeft);
    vuBarLayout.addWidget(&vuBarWidgetRight);
    vuBarLayout.setContentsMargins(0, 0, 0, 0);
    vuBarLayout.setSpacing(0);
    vuBarWidgetLeft.setFixedHeight(8);
    vuBarWidgetRight.setFixedHeight(8);

    layout.setColumnStretch(COL_PADR, 1);

    connect(&keyboardWidget, &KeyboardWidget::pressedChanged, this, &TrackWidget::updateAnalyzer);
}

TrackWidget::~TrackWidget()
{
}

void TrackWidget::setMuted(bool muted, bool visualOnly)
{
    this->muted = muted;

    QPalette pal;

    if (muted) {
        setSolo(false, visualOnly);
        pal.setColor(QPalette::Button, QColor(0xcc, 0x00, 0x00));
    } else {
        pal.setColor(QPalette::Button, QColor(0x60, 0x60, 0x60));
    }

    muteButton.setPalette(pal);

    emit muteOrSoloChanged(visualOnly);
}

bool TrackWidget::isMuted() const
{
    return muted;
}

void TrackWidget::setSolo(bool solo, bool visualOnly)
{
    this->solo = solo;

    QPalette pal;

    if (solo) {
        setMuted(false, visualOnly);
        pal.setColor(QPalette::Button, QColor(0x00, 0xcc, 0x00));
    } else {
        pal.setColor(QPalette::Button, QColor(0x60, 0x60, 0x60));
    }

    soloButton.setPalette(pal);

    emit muteOrSoloChanged(visualOnly);
}

bool TrackWidget::isSolo() const
{
    return solo;
}

void TrackWidget::setAnalyzer(bool analyzer)
{
    const bool analyzerChanged = this->analyzer != analyzer;
    this->analyzer = analyzer;

    QPalette pal;

    if (analyzer) {
        pal.setColor(QPalette::Button, QColor(0, 170, 170));
        keyboardWidget.setPressedColor(QColor(255, 150, 0));
    } else {
        pal.setColor(QPalette::Button, QColor(96, 96, 96));
        keyboardWidget.setPressedColor(QColor(255, 0, 255));
    }

    analyzerButton.setPalette(pal);

    if (analyzerChanged)
        updateAnalyzer();
}

bool TrackWidget::isAnalyzing() const
{
    return analyzer;
}

void TrackWidget::setAudible(bool audible)
{
    if (audible) {
        trackNoLabel.setPalette(labelUnmutedPalette);
        posLabel.setPalette(posPalette);
        restLabel.setPalette(restPalette);
        voiceTypeLabel.setPalette(voiceTypePalette);
        instNoLabel.setPalette(instNoPalette);
        volLabel.setPalette(volPalette);
        panLabel.setPalette(panPalette);
        modLabel.setPalette(modPalette);
        pitchLabel.setPalette(pitchPalette);
    } else {
        trackNoLabel.setPalette(labelMutedPalette);
        posLabel.setPalette(mutedLabelPalette);
        restLabel.setPalette(mutedLabelPalette);
        voiceTypeLabel.setPalette(mutedLabelPalette);
        instNoLabel.setPalette(mutedLabelPalette);
        volLabel.setPalette(mutedLabelPalette);
        panLabel.setPalette(mutedLabelPalette);
        modLabel.setPalette(mutedLabelPalette);
        pitchLabel.setPalette(mutedLabelPalette);
    }

    keyboardWidget.setMuted(!audible);
    vuBarWidgetLeft.setMuted(!audible);
    vuBarWidgetRight.setMuted(!audible);
}

const std::bitset<128> TrackWidget::getPressed() const
{
    return keyboardWidget.getPressedKeys();
}

#define setFmtText(...) setText(QString::fromStdString(fmt::format(__VA_ARGS__)))

void TrackWidget::setVisualizerState(const MP2KVisualizerStateTrack &state)
{
    if (oldTrackPtr != state.trackPtr) {
        oldTrackPtr = state.trackPtr;
        posLabel.setFmtText("0x{:07X}", state.trackPtr); // TODO patt/pend COLOR
    }

    if (oldIsCalling != state.isCalling) {
        oldIsCalling = state.isCalling;
        if (state.isCalling)
            posLabel.setPalette(posPalette);
        else
            posLabel.setPalette(posCallPalette);
    }

    if (oldEnvLFloat != state.envLFloat) {
        oldEnvLFloat = state.envLFloat;
        vuBarWidgetLeft.setLevel(state.envLFloat * 3.0f, 1.0f);
    }

    if (oldEnvRFloat != state.envRFloat) {
        oldEnvRFloat = state.envRFloat;
        vuBarWidgetRight.setLevel(state.envRFloat * 3.0f, 1.0f);
    }

    if (oldVol != state.vol) {
        oldVol = state.vol;
        volLabel.setText(QString::number(state.vol));
    }

    if (oldMod != state.mod) {
        oldMod = state.mod;
        modLabel.setText(QString::number(state.mod));
    }

    if (oldProg != state.prog) {
        oldProg = state.prog;
        if (state.prog == PROG_UNDEFINED)
            instNoLabel.setText("-");
        else
            instNoLabel.setText(QString::number(state.prog));
    }

    if (oldPan != state.pan) {
        oldPan = state.pan;
        if (state.pan < 0)
            panLabel.setFmtText("L{}", -state.pan);
        else if (state.pan > 0)
            panLabel.setFmtText("R{}", state.pan);
        else
            panLabel.setText("C");
    }

    if (oldPitch != state.pitch) {
        oldPitch = state.pitch;
        pitchLabel.setText(QString::number(state.pitch));
    }

    if (oldDelay != state.delay) {
        oldDelay = state.delay;
        restLabel.setText(QString::number(state.delay));
    }

    keyboardWidget.setPressedKeys(state.activeNotes);

    if (oldActiveVoiceTypes != static_cast<int>(state.activeVoiceTypes)) {
        oldActiveVoiceTypes = static_cast<int>(state.activeVoiceTypes);

        const char *s;
        switch (state.activeVoiceTypes) {
        case VoiceFlags::NONE: s = "-"; break;
        case VoiceFlags::PCM: s = "PCM"; break;
        case VoiceFlags::DPCM_GAMEFREAK: s = "DPCM"; break;
        case VoiceFlags::ADPCM_CAMELOT: s = "ADPCM"; break;
        case VoiceFlags::SYNTH_PWM: s = "PWM"; break;
        case VoiceFlags::SYNTH_SAW: s = "Saw"; break;
        case VoiceFlags::SYNTH_TRI: s = "Tri."; break;
        case VoiceFlags::PSG_SQ_12: s = "Sq.12"; break;
        case VoiceFlags::PSG_SQ_25: s = "Sq.25"; break;
        case VoiceFlags::PSG_SQ_50: s = "Sq.50"; break;
        case VoiceFlags::PSG_SQ_75: s = "Sq.75"; break;
        case VoiceFlags::PSG_SQ_12_SWEEP: s = "Sq.12S"; break;
        case VoiceFlags::PSG_SQ_25_SWEEP: s = "Sq.25S"; break;
        case VoiceFlags::PSG_SQ_50_SWEEP: s = "Sq.50S"; break;
        case VoiceFlags::PSG_SQ_75_SWEEP: s = "Sq.75S"; break;
        case VoiceFlags::PSG_WAVE: s = "Wave"; break;
        case VoiceFlags::PSG_NOISE_7: s = "Ns.7"; break;
        case VoiceFlags::PSG_NOISE_15: s = "Ns.15"; break;
        default: s = "Multi"; break;
        }
        voiceTypeLabel.setText(s);
    }
}

#undef setFmtText

void TrackWidget::setPressed(const std::bitset<128> &pressed)
{
    keyboardWidget.setPressedKeys(pressed);
}

size_t TrackWidget::getTrackNo() const
{
    return trackNo;
}

void TrackWidget::updateAnalyzer()
{
    emit analyzerChanged();
}

const QPalette TrackWidget::labelUnmutedPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#173917"));
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    pal.setColor(QPalette::Light, QColor("#6ffd6f"));
    pal.setColor(QPalette::Mid, QColor("#4af44a"));
    pal.setColor(QPalette::Dark, QColor("#009200"));
    return pal;
}();

const QPalette TrackWidget::labelMutedPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#391717"));
    pal.setColor(QPalette::WindowText, QColor(255, 255, 255));
    pal.setColor(QPalette::Light, QColor("#fd6f6f"));
    pal.setColor(QPalette::Mid, QColor("#f44a4a"));
    pal.setColor(QPalette::Dark, QColor("#920000"));
    return pal;
}();

const QPalette TrackWidget::mutedLabelPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#a0a0a0"));
    return pal;
}();

const QPalette TrackWidget::posPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#27b927"));
    return pal;
}();

const QPalette TrackWidget::posCallPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e7e700"));
    return pal;
}();

const QPalette TrackWidget::restPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#ff2121"));
    return pal;
}();

const QPalette TrackWidget::voiceTypePalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e7c900"));
    return pal;
}();

const QPalette TrackWidget::instNoPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e762bc"));
    return pal;
}();

const QPalette TrackWidget::volPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#2ac642"));
    return pal;
}();

const QPalette TrackWidget::panPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#e67920"));
    return pal;
}();

const QPalette TrackWidget::modPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#00d0db"));
    return pal;
}();

const QPalette TrackWidget::pitchPalette = []() {
    QPalette pal;
    pal.setColor(QPalette::WindowText, QColor("#b800e7"));
    return pal;
}();

const QFont TrackWidget::labelFont = []() {
    QFont font;
    font.setPointSize(8);
    return font;
}();

const QFont TrackWidget::labelMonospaceFont = []() {
    QFont font;
    font.setPointSize(8);
    font.setFixedPitch(true);
    return font;
}();
