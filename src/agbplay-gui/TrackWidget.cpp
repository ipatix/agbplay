#include "TrackWidget.h"

TrackWidget::TrackWidget(size_t trackNo, QWidget *parent)
    : QWidget(parent)
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
    setMuted(false);
    layout.addWidget(&muteButton, 1, COL_BUTTONS1);

    connect(&soloButton, &QAbstractButton::clicked, [this](bool) { setSolo(!isSolo()); });
    soloButton.setFixedSize(16, 16);
    soloButton.setText("S");
    soloButton.setToolTip("Solo/De-solo Track");
    setSolo(false);
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
    voiceTypeLabel.setText("NONE");
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

    vuBarKeyboardLayout.addWidget(&vuBarWidget);
    vuBarKeyboardLayout.addWidget(&keyboardWidget);
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

void TrackWidget::setMuted(bool muted)
{
    this->muted = muted;

    if (muted) {
        setSolo(false);
        muteButton.setStyleSheet("QPushButton {background-color: #CC0000;};");
    } else {
        muteButton.setStyleSheet("QPushButton {background-color: #606060;};");
    }

    emit muteOrSoloChanged();
}

bool TrackWidget::isMuted() const
{
    return muted;
}

void TrackWidget::setSolo(bool solo)
{
    this->solo = solo;

    if (solo) {
        setMuted(false);
        soloButton.setStyleSheet("QPushButton {background-color: #00CC00;};");
    } else {
        soloButton.setStyleSheet("QPushButton {background-color: #606060;};");
    }

    emit muteOrSoloChanged();
}

bool TrackWidget::isSolo() const
{
    return solo;
}

void TrackWidget::setAnalyzer(bool analyzer)
{
    const bool analyzerChanged = this->analyzer != analyzer;
    this->analyzer = analyzer;

    if (analyzer) {
        analyzerButton.setStyleSheet("QPushButton {background-color: #00AAAA;};");
        keyboardWidget.setPressedColor(QColor(0, 220, 220));
    } else {
        analyzerButton.setStyleSheet("QPushButton {background-color: #606060;};");
        keyboardWidget.setPressedColor(QColor(255, 0, 255));
    }

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

void TrackWidget::setPressed(const std::bitset<128> &pressed)
{
    keyboardWidget.setPressedKeys(pressed);
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
