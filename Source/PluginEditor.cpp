/*
    MIRACLE DSP – Digital Sample Player
    Copyright (C) 2026 Stefan Schäfer (ESS76)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "PluginEditor.h"

#include <algorithm>

using namespace params;

namespace
{
namespace colours
{
const juce::Colour chassis    { 0xff2d3339 };   // slate body
const juce::Colour chassisHi  { 0xff3c434a };
const juce::Colour chassisLo  { 0xff1b1f23 };
const juce::Colour fascia     { 0xff262422 };   // warm charcoal control panel
const juce::Colour silk       { 0xffd8d6cd };
const juce::Colour silkDim    { 0xff9a9992 };
const juce::Colour stripeRed  { 0xffc9261d };
const juce::Colour amber      { 0xffe7a820 };
const juce::Colour logoYellow { 0xfff2b518 };
const juce::Colour keyGrey    { 0xff8e908d };
const juce::Colour keyWhite   { 0xffe8e5da };
const juce::Colour keyBlack   { 0xff17181a };
const juce::Colour lcdLit     { 0xff8fd27a };   // backlit green display
const juce::Colour lcdDark    { 0xff14300f };
const juce::Colour scope      { 0xff86e06c };   // green on dark glass
const juce::Colour glass      { 0xff0c0f0c };
}

juce::Font uiFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

// Design size of the window; the edit panel extends it downwards
constexpr int editorWidth = 1360;
constexpr int compactHeight = 452;
constexpr int editPanelHeight = 318;

constexpr int stripHeight = 36;     // rear strip
constexpr int panelHeight = 156;    // control panel
constexpr int barHeight = 26;       // bar in front of the keys
constexpr int leftColumn = 280;     // disk tray, drive, wheels
constexpr int faderWidth = 44;
constexpr int backgroundScale = 2;  // panel artwork stays sharp on Retina displays

// Keypad grid inside the control panel
constexpr int keyWidth = 34;
constexpr int keyHeight = 14;
constexpr int columnX[] = { 525, 605, 720, 766, 812, 868, 924, 980 };
constexpr int rowY[] = { 22, 56, 90, 124 };

// 7-segment patterns, bit 0 = a (top) .. bit 6 = g (middle)
int segmentMask (juce::juce_wchar c)
{
    switch (juce::CharacterFunctions::toUpperCase (c))
    {
        case '0': case 'O': return 0x3f;
        case '1': case 'I': return 0x06;
        case '2': return 0x5b;
        case '3': return 0x4f;
        case '4': return 0x66;
        case '5': case 'S': return 0x6d;
        case '6': return 0x7d;
        case '7': return 0x07;
        case '8': return 0x7f;
        case '9': return 0x6f;
        case 'A': return 0x77;
        case 'B': return 0x7c;
        case 'C': return 0x39;
        case 'D': return 0x5e;
        case 'E': return 0x79;
        case 'F': return 0x71;
        case 'G': return 0x3d;
        case 'H': return 0x76;
        case 'J': return 0x1e;
        case 'L': return 0x38;
        case 'N': return 0x54;
        case 'P': return 0x73;
        case 'R': return 0x50;
        case 'T': return 0x78;
        case 'U': return 0x3e;
        case 'Y': return 0x6e;
        case '-': return 0x40;
        default:  return 0;
    }
}

juce::Path segmentPath (int segment, float w, float h)
{
    const float t = w * 0.17f;
    const float gap = t * 0.18f;
    juce::Path p;

    auto horizontal = [&] (float y)
    {
        const float x0 = gap + t * 0.5f, x1 = w - gap - t * 0.5f;
        p.startNewSubPath (x0, y);
        p.lineTo (x0 + t * 0.5f, y - t * 0.5f);
        p.lineTo (x1 - t * 0.5f, y - t * 0.5f);
        p.lineTo (x1, y);
        p.lineTo (x1 - t * 0.5f, y + t * 0.5f);
        p.lineTo (x0 + t * 0.5f, y + t * 0.5f);
        p.closeSubPath();
    };

    auto vertical = [&] (float x, float y0, float y1)
    {
        y0 += gap + t * 0.5f;
        y1 -= gap + t * 0.5f;
        p.startNewSubPath (x, y0);
        p.lineTo (x + t * 0.5f, y0 + t * 0.5f);
        p.lineTo (x + t * 0.5f, y1 - t * 0.5f);
        p.lineTo (x, y1);
        p.lineTo (x - t * 0.5f, y1 - t * 0.5f);
        p.lineTo (x - t * 0.5f, y0 + t * 0.5f);
        p.closeSubPath();
    };

    switch (segment)
    {
        case 0: horizontal (t * 0.5f); break;
        case 1: vertical (w - t * 0.5f, 0.0f, h * 0.5f); break;
        case 2: vertical (w - t * 0.5f, h * 0.5f, h); break;
        case 3: horizontal (h - t * 0.5f); break;
        case 4: vertical (t * 0.5f, h * 0.5f, h); break;
        case 5: vertical (t * 0.5f, 0.0f, h * 0.5f); break;
        case 6: horizontal (h * 0.5f); break;
        default: break;
    }

    return p;
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float r)
{
    juce::ColourGradient grad (juce::Colour (0xff5a6066), c.x - r, c.y - r, juce::Colour (0xff15181b), c.x + r, c.y + r, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (juce::Colours::black);
    g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 0.8f);
    g.drawLine (c.x - r * 0.6f, c.y - r * 0.6f, c.x + r * 0.6f, c.y + r * 0.6f, 1.2f);
    g.drawLine (c.x - r * 0.6f, c.y + r * 0.6f, c.x + r * 0.6f, c.y - r * 0.6f, 1.2f);
}

// Recessed dark glass window
void drawInset (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setColour (colours::glass);
    g.fillRoundedRectangle (r, corner);
    juce::ColourGradient shade (juce::Colours::black.withAlpha (0.6f), r.getX(), r.getY(),
                                juce::Colours::transparentBlack, r.getX(), r.getY() + 10.0f, false);
    g.setGradientFill (shade);
    g.fillRoundedRectangle (r, corner);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, corner, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (r.getX() + corner, r.getBottom() + 0.5f, r.getRight() - corner, r.getBottom() + 0.5f, 1.0f);
}

void addGrain (juce::Image& image, juce::Rectangle<int> area, float amount, int seed)
{
    juce::Random rng (seed);
    juce::Image::BitmapData data (image, juce::Image::BitmapData::readWrite);
    for (int y = area.getY(); y < area.getBottom(); ++y)
        for (int x = area.getX(); x < area.getRight(); ++x)
        {
            const auto c = data.getPixelColour (x, y);
            const float n = (rng.nextFloat() - 0.5f) * amount;
            data.setPixelColour (x, y, n > 0.0f ? c.brighter (n) : c.darker (-n));
        }
}

const char* const audioPatterns = "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3";
const char* const audioExtensions = "wav;aif;aiff;flac;ogg;mp3";

juce::String arrow (bool up)
{
    return juce::String::fromUTF8 (up ? "\xe2\x96\xb2" : "\xe2\x96\xbc");
}
} // namespace

//==============================================================================
MiracleLookAndFeel::MiracleLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::chassis);
    setColour (juce::Label::textColourId, colours::silk);
    setColour (juce::ComboBox::textColourId, colours::scope);
    setColour (juce::ComboBox::arrowColourId, colours::scope);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff15181b));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::lcdLit);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::lcdDark);
    setColour (juce::PopupMenu::textColourId, colours::silk);
    setColour (juce::ListBox::backgroundColourId, colours::glass);
    setColour (juce::ListBox::outlineColourId, juce::Colours::black);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff15181b));
    setColour (juce::TooltipWindow::textColourId, colours::silk);
}

void MiracleLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                           juce::Slider::SliderStyle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float cx = bounds.getCentreX();
    const float capW = std::min (w > 30 ? 28.0f : 20.0f, bounds.getWidth() - 4.0f);
    const float capH = w > 30 ? 15.0f : 11.0f;

    // silkscreen scale
    g.setColour (colours::silkDim);
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = bounds.getY() + (float) i / 10.0f * bounds.getHeight();
        const float len = i % 5 == 0 ? 5.0f : 3.0f;
        g.drawLine (cx - capW * 0.5f - len - 1.0f, ty, cx - capW * 0.5f - 1.0f, ty, 0.8f);
    }

    // slot
    auto slot = juce::Rectangle<float> (cx - 2.0f, bounds.getY() - 3.0f, 4.0f, bounds.getHeight() + 6.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (slot, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawVerticalLine ((int) slot.getRight(), slot.getY(), slot.getBottom());

    // black cap with white line
    auto cap = juce::Rectangle<float> (cx - capW * 0.5f, pos - capH * 0.5f, capW, capH);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (cap.translated (0.0f, 2.0f), 1.5f);
    juce::ColourGradient body (juce::Colour (0xff3b3d40), cap.getX(), cap.getY(), juce::Colour (0xff0d0e0f), cap.getX(), cap.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (cap, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.fillRect (cap.getX() + 1.0f, pos - 1.0f, capW - 2.0f, 2.0f);
}

void MiracleLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat();
    const auto key = r.removeFromLeft (30.0f).withSizeKeepingCentre (26.0f, 12.0f).translated (0.0f, down ? 1.0f : 0.0f);
    const bool on = b.getToggleState();

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (key.translated (0.0f, 2.0f), 1.0f);
    g.setColour (on ? colours::amber : colours::keyGrey);
    g.fillRoundedRectangle (key, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawHorizontalLine ((int) key.getY(), key.getX() + 1.0f, key.getRight() - 1.0f);

    g.setColour (highlighted ? colours::silk.brighter() : colours::silk);
    g.setFont (uiFont (9.5f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (4.0f), juce::Justification::centredLeft);
}

void MiracleLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&)
{
    drawInset (g, juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f), 2.0f);

    juce::Path tri;
    const float cx = (float) width - 12.0f, cy = (float) height * 0.5f;
    tri.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (colours::scope);
    g.fillPath (tri);
}

void MiracleLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                        bool vertical, int thumbStart, int thumbSize, bool over, bool)
{
    const auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, width, thumbSize)
                                : juce::Rectangle<int> (thumbStart, y, thumbSize, height);
    g.setColour (colours::scope.withAlpha (over ? 0.6f : 0.35f));
    g.fillRoundedRectangle (thumb.reduced (3, 2).toFloat(), 2.0f);
}

//==============================================================================
void LedDisplay::show (const juce::String& l, const juce::String& r, const juce::String& c)
{
    if (l == left && r == right && c == caption)
        return;

    left = l;
    right = r;
    caption = c;
    repaint();
}

void LedDisplay::drawDigits (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& text, int digits)
{
    const float h = area.getHeight();
    const float digitW = area.getWidth() / (float) digits;
    const float w = std::min (digitW * 0.8f, h * 0.56f);

    // right-justified like a numeric readout
    const auto padded = juce::String::repeatedString (" ", std::max (0, digits - text.length())) + text.substring (0, digits);

    for (int d = 0; d < digits; ++d)
    {
        const int mask = segmentMask (padded[d]);
        const float x = area.getX() + (float) d * digitW + (digitW - w) * 0.5f;
        const auto transform = juce::AffineTransform::shear (-0.08f, 0.0f).translated (x + h * 0.08f, area.getY());

        for (int s = 0; s < 7; ++s)
        {
            auto p = segmentPath (s, w, h);
            p.applyTransform (transform);
            g.setColour ((mask >> s) & 1 ? colours::lcdDark : colours::lcdDark.withAlpha (0.07f));
            g.fillPath (p);
        }
    }
}

void LedDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // black bezel
    g.setColour (juce::Colour (0xff0d0e0f));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);

    // backlit green window
    auto window = r.reduced (4.0f);
    juce::ColourGradient lit (colours::lcdLit.brighter (0.15f), window.getCentreX(), window.getY(),
                              colours::lcdLit.darker (0.18f), window.getCentreX(), window.getBottom(), false);
    g.setGradientFill (lit);
    g.fillRect (window);

    auto inner = window.reduced (9.0f, 6.0f);
    auto captionArea = inner.removeFromBottom (12.0f);
    inner.removeFromBottom (3.0f);
    auto leftArea = inner.removeFromLeft (inner.getWidth() * 0.36f);
    inner.removeFromLeft (inner.getWidth() * 0.08f);

    drawDigits (g, leftArea, left, 2);
    drawDigits (g, inner, right, 4);

    g.setColour (colours::lcdDark);
    g.setFont (uiFont (10.0f, true));
    g.drawFittedText (caption.toUpperCase(), captionArea.toNearestInt(), juce::Justification::centred, 1, 0.7f);
}

//==============================================================================
void WaveformView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    drawInset (g, r, 3.0f);

    g.setColour (colours::scope.withAlpha (0.07f));
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / 8.0f), r.getY() + 2.0f, r.getBottom() - 2.0f);
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 2.0f, r.getRight() - 2.0f);

    auto bank = audioProcessor.getBank (half);
    if (bank == nullptr || bank->samples.empty())
    {
        g.setColour (colours::scope.withAlpha (0.7f));
        g.setFont (uiFont (11.0f, true));
        const bool unreadable = bank != nullptr && bank->index >= 0;
        g.drawFittedText (bank == nullptr ? "NO FOLDER LOADED - PLAYING BUILT-IN INIT WAVE"
                          : unreadable    ? "CANNOT READ THIS FILE - PLAYING BUILT-IN INIT WAVE"
                                          : "NO AUDIO FILES IN THIS FOLDER - PLAYING BUILT-IN INIT WAVE",
                          getLocalBounds().reduced (10), juce::Justification::centred, 2);
        return;
    }

    auto value = [&] (int idx) { return audioProcessor.apvts.getRawParameterValue (halfId (half, idx))->load(); };

    const auto& ws = bank->samples[0];
    const int n = (int) ws.data.size();
    if (n < 2)
        return;

    auto plot = r.reduced (6.0f, 18.0f);
    const float mid = plot.getCentreY();
    const float amp = plot.getHeight() * 0.5f;
    auto xFor = [&] (double sampleIndex) { return plot.getX() + (float) (sampleIndex / n) * plot.getWidth(); };

    // play range and loop region, computed like the voice does
    const int start = juce::jlimit (0, n - 2, (int) ((double) n * value (hpSmpStart) / 1000.0));
    const int end = juce::jlimit (start + 1, n, (int) ((double) n * value (hpSmpEnd) / 1000.0));
    const int loopMode = (int) value (hpLoopMode);
    int loopStart = -1, loopEnd = -1;

    if (loopMode == 2 && ws.fileLoopStart >= 0)
    {
        loopStart = ws.fileLoopStart;
        loopEnd = ws.fileLoopEnd;
    }
    else if (loopMode == 1)
    {
        loopStart = start + (int) ((double) (end - start) * value (hpLoopStart) / 1000.0);
        loopEnd = start + (int) ((double) (end - start) * value (hpLoopEnd) / 1000.0);
        if (loopEnd - loopStart < 2)
            loopStart = loopEnd = -1;
    }

    if (loopStart >= 0)
    {
        g.setColour (colours::amber.withAlpha (0.14f));
        g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (loopStart), plot.getY(), xFor (loopEnd), plot.getBottom()));
    }

    const int columns = std::max (1, (int) plot.getWidth());
    juce::Path wave;
    for (int c = 0; c < columns; ++c)
    {
        const int s0 = (int) ((juce::int64) c * n / columns);
        const int s1 = std::max (s0 + 1, (int) ((juce::int64) (c + 1) * n / columns));
        float lo = 1.0f, hi = -1.0f;
        const int step = std::max (1, (s1 - s0) / 64);
        for (int s = s0; s < s1 && s < n; s += step)
        {
            lo = std::min (lo, ws.data[(size_t) s]);
            hi = std::max (hi, ws.data[(size_t) s]);
        }
        wave.addRectangle (plot.getX() + (float) c, mid - hi * amp, 1.0f, std::max (1.0f, (hi - lo) * amp));
    }

    g.setColour (colours::scope);
    g.fillPath (wave);

    // dim the region outside the play range
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (plot.getX(), plot.getY(), xFor (start), plot.getBottom()));
    g.fillRect (juce::Rectangle<float>::leftTopRightBottom (xFor (end), plot.getY(), plot.getRight(), plot.getBottom()));

    if (loopStart >= 0)
    {
        g.setColour (colours::amber);
        g.drawVerticalLine ((int) xFor (loopStart), plot.getY(), plot.getBottom());
        g.drawVerticalLine ((int) xFor (loopEnd), plot.getY(), plot.getBottom());
    }

    g.setColour (colours::scope);
    g.setFont (uiFont (10.5f, true));
    g.drawText (ws.name.toUpperCase(), r.reduced (8.0f, 3.0f).removeFromTop (13.0f), juce::Justification::centredLeft, true);

    juce::String info = "ROOT " + juce::MidiMessage::getMidiNoteName (ws.rootNote, true, true, 3);
    if (! ws.rootKnown)
        info << " (DEFAULT)";
    info << "   " << juce::String (ws.fileRate / 1000.0, 1) << " KHZ   " << juce::String ((double) n / ws.fileRate, 2) << " S";
    if (loopMode == 2)
        info << (ws.fileLoopStart >= 0 ? "   FILE LOOP" : "   NO FILE LOOP");

    g.setColour (colours::scope.withAlpha (0.7f));
    g.drawText (info, r.reduced (8.0f, 3.0f).removeFromBottom (13.0f), juce::Justification::centredLeft, true);
}

//==============================================================================
void KeyCap::setLit (bool shouldBeLit)
{
    if (lit != shouldBeLit)
    {
        lit = shouldBeLit;
        repaint();
    }
}

void KeyCap::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.0f, 1.0f);
    r.removeFromBottom (2.0f);
    if (down)
        r.translate (0.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRect (r.translated (0.0f, down ? 1.0f : 2.0f).expanded (0.5f, 0.0f));

    auto base = cap;
    if (highlighted)
        base = base.brighter (0.08f);
    if (down)
        base = base.darker (0.15f);

    juce::ColourGradient grad (base.brighter (0.08f), r.getX(), r.getY(), base.darker (0.15f), r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (cap.getPerceivedBrightness() < 0.3f ? 0.15f : 0.35f));
    g.drawHorizontalLine ((int) r.getY(), r.getX() + 1.0f, r.getRight() - 1.0f);

    if (lit)
    {
        // status LED set into the key
        const auto dot = juce::Rectangle<float> (r.getCentreX() - 2.5f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
        g.setColour (juce::Colour (0xffff3a20).withAlpha (0.35f));
        g.fillEllipse (dot.expanded (2.5f));
        g.setColour (juce::Colour (0xffff3a20));
        g.fillEllipse (dot);
    }
}

//==============================================================================
SoundSlot::SoundSlot (MiracleAudioProcessor& p, int h) : audioProcessor (p), half (h)
{
    shownOctave = getOctave();
    setTooltip ("Click to edit this half, scroll to change the sound");
}

int SoundSlot::getOctave() const
{
    return juce::roundToInt (audioProcessor.apvts.getRawParameterValue (halfId (half, hpOctave))->load());
}

void SoundSlot::syncOctave()
{
    const int octave = getOctave();
    if (octave != shownOctave)
    {
        shownOctave = octave;
        repaint();
    }
}

void SoundSlot::refresh()
{
    const int n = audioProcessor.getNumSounds (half);
    const int index = audioProcessor.getSoundIndex (half);
    auto list = audioProcessor.getSoundList (half);

    if (list == nullptr)
    {
        soundName = "NO DISK";
        counter = {};
    }
    else if (n == 0)
    {
        soundName = "NO AUDIO FILES";
        counter = "0";
    }
    else
    {
        soundName = list->displayName (index).fromLastOccurrenceOf ("/", false, false);
        counter = juce::String (index + 1) + "/" + juce::String (n);

        auto bank = audioProcessor.getBank (half);
        if (bank != nullptr && bank->samples.empty())
            soundName = "UNREADABLE: " + soundName;
    }

    repaint();
}

void SoundSlot::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    const auto text = active ? colours::scope : colours::scope.withAlpha (0.5f);

    if (active)
    {
        g.setColour (colours::scope.withAlpha (0.10f));
        g.fillRect (r);
    }

    r.reduce (5, 0);
    g.setFont (uiFont (10.5f, true));
    g.setColour (text);
    g.drawText (half == 0 ? "LOW" : "UPP", r.removeFromLeft (28), juce::Justification::centredLeft);

    g.setColour (shownOctave != 0 ? colours::amber : text.withMultipliedAlpha (0.6f));
    g.drawText (juce::String (shownOctave > 0 ? "+" : "") + juce::String (shownOctave) + " OCT",
                r.removeFromRight (40), juce::Justification::centredRight);
    g.setColour (text.withMultipliedAlpha (0.8f));
    g.drawText (counter, r.removeFromRight (46), juce::Justification::centredRight);
    g.setColour (text);
    g.drawText (soundName.toUpperCase(), r, juce::Justification::centredLeft, true);
}

void SoundSlot::mouseDown (const juce::MouseEvent&)
{
    if (onSelect)
        onSelect();
}

void SoundSlot::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (onSelect)
        onSelect();
    if (wheel.deltaY != 0.0f)
        audioProcessor.stepSound (half, wheel.deltaY > 0.0f ? -1 : 1);
}

//==============================================================================
void ControlWheel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    // recessed slot
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    auto wheel = r.reduced (3.0f, 2.0f);
    juce::ColourGradient body (juce::Colour (0xff0b0b0c), wheel.getX(), wheel.getY(), juce::Colour (0xff0b0b0c), wheel.getX(), wheel.getBottom(), false);
    body.addColour (0.5, juce::Colour (0xff3a3b3e));
    g.setGradientFill (body);
    g.fillRoundedRectangle (wheel, 2.0f);

    // ridges rotate with the wheel; spacing shrinks towards the edges like a cylinder
    const float travel = spring ? value : value * 2.0f - 1.0f;
    const float phase = -travel * 1.2f; // wheel up moves the marks up
    for (int i = -12; i <= 12; ++i)
    {
        const float a = (float) i * 0.19f + phase;
        if (std::abs (a) >= juce::MathConstants<float>::halfPi)
            continue;

        const float y = wheel.getCentreY() + std::sin (a) * wheel.getHeight() * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.22f * std::cos (a)));
        g.drawHorizontalLine ((int) y, wheel.getX() + 1.0f, wheel.getRight() - 1.0f);
    }

    // centre mark
    if (std::abs (phase) < juce::MathConstants<float>::halfPi)
    {
        const float y = wheel.getCentreY() + std::sin (phase) * wheel.getHeight() * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.85f * std::cos (phase)));
        g.fillRect (wheel.getX() + 1.0f, y - 1.0f, wheel.getWidth() - 2.0f, 2.0f);
    }
}

void ControlWheel::setPosition (float newValue)
{
    if (isMouseButtonDown() || std::abs (newValue - value) < 0.002f)
        return;

    value = newValue;
    repaint();
}

void ControlWheel::mouseDown (const juce::MouseEvent&)
{
    dragStart = value;
}

void ControlWheel::mouseDrag (const juce::MouseEvent& e)
{
    const float range = spring ? 2.0f : 1.0f;
    const float next = dragStart - (float) e.getDistanceFromDragStartY() / (float) getHeight() * range;
    value = juce::jlimit (spring ? -1.0f : 0.0f, 1.0f, next);
    repaint();
    if (onChange)
        onChange (value);
}

void ControlWheel::mouseUp (const juce::MouseEvent&)
{
    if (spring)
    {
        value = 0.0f;
        repaint();
        if (onChange)
            onChange (value);
    }
}

//==============================================================================
void RealisticKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                       juce::Colour, juce::Colour)
{
    auto key = area.reduced (0.7f, 0.0f);

    juce::ColourGradient body (isDown ? juce::Colour (0xffd0cabb) : juce::Colour (0xfff1ecdf), key.getX(), key.getY(),
                               isDown ? juce::Colour (0xffb9b2a1) : juce::Colour (0xffe6e0d0), key.getX(), key.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (key.withTrimmedTop (-4.0f), 2.0f);

    if (isOver && ! isDown)
    {
        g.setColour (juce::Colours::black.withAlpha (0.03f));
        g.fillRect (key);
    }

    // front lip of the key
    g.setColour (isDown ? juce::Colour (0xffa39c8c) : juce::Colour (0xffcbc4b3));
    g.fillRoundedRectangle (key.removeFromBottom (isDown ? 5.0f : 8.0f), 2.0f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawVerticalLine ((int) area.getX(), area.getY(), area.getBottom());

    if (note == splitNote)
    {
        juce::Path tri;
        const float cx = area.getCentreX(), y = area.getBottom() - 22.0f;
        tri.addTriangle (cx - 5.0f, y + 7.0f, cx + 5.0f, y + 7.0f, cx, y);
        g.setColour (colours::stripeRed);
        g.fillPath (tri);
    }
}

void RealisticKeyboard::drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                       juce::Colour)
{
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (area.translated (1.5f, 1.5f), 2.0f);

    juce::ColourGradient body (juce::Colour (0xff040404), area.getX(), area.getY(),
                               juce::Colour (0xff1e1e1f), area.getX(), area.getBottom(), false);
    g.setGradientFill (body);
    g.fillRoundedRectangle (area, 2.0f);

    // glossy top face
    auto top = area.reduced (area.getWidth() * 0.13f, 0.0f).withTrimmedBottom (isDown ? 5.0f : 11.0f);
    juce::ColourGradient face (juce::Colour (isDown ? 0xff232324 : 0xff141415), top.getX(), top.getY(),
                               juce::Colour (isDown ? 0xff353536 : 0xff434345), top.getX(), top.getBottom(), false);
    g.setGradientFill (face);
    g.fillRoundedRectangle (top, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (isOver ? 0.32f : 0.2f));
    g.drawLine (top.getX() + 1.5f, top.getY() + 3.0f, top.getX() + 1.5f, top.getBottom() - 3.0f, 1.0f);
}

//==============================================================================
MiracleAudioProcessorEditor::MiracleAudioProcessorEditor (MiracleAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      editKey ("Edit", colours::keyWhite),
      upperSlot (p, 1),
      lowerSlot (p, 0),
      waveform (p),
      keyboard (p.keyboardState)
{
    setLookAndFeel (&laf);

    addAndMakeVisible (led);
    buildKeypad();

    editKey.setTooltip ("Open or close the edit panel with all parameters");
    editKey.onClick = [this] { setEditPanelVisible (! editOpen); };
    addAndMakeVisible (editKey);

    for (auto* slot : { &upperSlot, &lowerSlot })
    {
        const int half = slot == &lowerSlot ? 0 : 1;
        slot->onSelect = [this, half] { if (currentHalf != half) selectHalf (half); };
        addAndMakeVisible (slot);
    }

    soundList.setModel (this);
    soundList.setRowHeight (18);
    soundList.setWantsKeyboardFocus (true);
    soundList.setOutlineThickness (0);
    addAndMakeVisible (soundList);

    pitchWheel.onChange = [this] (float v)
    {
        audioProcessor.sendUiMidi (juce::MidiMessage::pitchWheel (1, juce::jlimit (0, 16383, 8192 + juce::roundToInt (v * 8191.0f))));
    };
    modWheel.onChange = [this] (float v)
    {
        audioProcessor.sendUiMidi (juce::MidiMessage::controllerEvent (1, 1, juce::roundToInt (v * 127.0f)));
    };
    addAndMakeVisible (pitchWheel);
    addAndMakeVisible (modWheel);

    volumeFader.setSliderStyle (juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, globalId (gpVolume), volumeFader);
    volumeFader.setDoubleClickReturnValue (true, globalDefs[gpVolume].def);
    volumeFader.onValueChange = [this]
    {
        if (volumeFader.isMouseButtonDown())
            showParam (gpVolume, true, volumeFader.getValue());
    };
    addAndMakeVisible (volumeFader);

    keyboard.setAvailableRange (36, 96); // 61 keys, C1..C6
    keyboard.setOctaveForMiddleC (3);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setBlackNoteLengthProportion (0.64f);
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.55f));
    addAndMakeVisible (keyboard);

    // Edit panel
    loopModeBox.addItemList (juce::StringArray::fromTokens (halfDefs[hpLoopMode].choices, "|", ""), 1);
    addChildComponent (loopModeBox);
    loopLabel.setText ("LOOP MODE", juce::dontSendNotification);
    loopLabel.setFont (uiFont (9.5f, true));
    loopLabel.setColour (juce::Label::textColourId, colours::silk);
    loopLabel.setJustificationType (juce::Justification::centredLeft);
    addChildComponent (loopLabel);
    addChildComponent (waveform);

    for (int idx : { hpSmpStart, hpSmpEnd, hpLoopStart, hpLoopEnd,
                     hpCoarse, hpFine, hpDetune, hpOscMix, hpMixVel, hpLevel, hpLfoSpeed, hpLfoDepth,
                     hpCutoff, hpReso, hpKeyTrack })
        addFader (idx, false);

    addToggle (hpMixMode, false);
    addToggle (hpMono, false);

    for (int idx = hpFAtt; idx <= hpARelVel; ++idx)
        addFader (idx, false);

    for (int idx : { gpSplit, gpRate, gpOctave, gpTune, gpBend, gpVoices })
        addFader (idx, true);
    addToggle (gpLoFi, true);

    // Parameter list for the keypad, in number order
    for (int i = 0; i < kNumHP; ++i)
        entries.push_back ({ halfDefs[i].number, i, false });
    for (int i = 0; i < kNumGP; ++i)
        entries.push_back ({ globalDefs[i].number, i, true });
    std::sort (entries.begin(), entries.end(), [] (const ParamEntry& a, const ParamEntry& b) { return a.number < b.number; });

    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].number == halfDefs[hpCutoff].number)
            selectedEntry = i;

    editOpen = audioProcessor.isEditPanelOpen();
    setSize (editorWidth, compactHeight + (editOpen ? editPanelHeight : 0));
    selectHalf (0);
    startTimerHz (30);
}

MiracleAudioProcessorEditor::~MiracleAudioProcessorEditor()
{
    stopTimer();
    soundList.setModel (nullptr);
    setLookAndFeel (nullptr);
}

const Def& MiracleAudioProcessorEditor::defFor (int index, bool global) const
{
    return global ? globalDefs[index] : halfDefs[index];
}

KeyCap& MiracleAudioProcessorEditor::addKey (const juce::String& label, juce::Colour colour, int column, int row,
                                             std::function<void()> action)
{
    PanelKey key;
    key.cap = std::make_unique<KeyCap> (label, colour);
    key.label = label;
    key.column = column;
    key.row = row;
    key.cap->onClick = std::move (action);
    addAndMakeVisible (key.cap.get());
    keys.push_back (std::move (key));
    return *keys.back().cap;
}

void MiracleAudioProcessorEditor::buildKeypad()
{
    using namespace colours;

    // Digit block: 1-4 are amber, they also pick the program after 0/Prog
    int digit = 1;
    for (int row = 0; row < 3; ++row)
        for (int col = 2; col < 5; ++col, ++digit)
        {
            const int d = digit;
            auto& key = addKey (juce::String (d), d <= 4 ? amber : keyGrey, col, row, [this, d] { pressDigit (d); });
            if (d <= 4)
                programKeys[d - 1] = &key;
        }

    addKey ("Cancel", keyWhite, 2, 3, [this] { pressCancel(); });
    addKey ("0/Prog", amber, 3, 3, [this] { pressProgram(); });
    addKey ("Enter", keyGrey, 4, 3, [this] { pressEnter(); });

    // Control block
    addKey ("Param", keyGrey, 0, 2, [this] { pressParam(); });
    addKey ("Value", keyGrey, 1, 2, [this] { pressValue(); });
    addKey ("Off/" + arrow (false), keyGrey, 0, 3, [this] { pressStep (-1); }).setRepeatSpeed (400, 70);
    addKey ("On/" + arrow (true), keyGrey, 1, 3, [this] { pressStep (1); }).setRepeatSpeed (400, 70);

    // Sound and octave
    addKey ("Snd " + arrow (true), keyGrey, 5, 0, [this] { audioProcessor.stepSound (currentHalf, 1); }).setRepeatSpeed (400, 120);
    addKey ("Snd " + arrow (false), keyGrey, 5, 1, [this] { audioProcessor.stepSound (currentHalf, -1); }).setRepeatSpeed (400, 120);
    addKey ("Oct " + arrow (true), keyGrey, 5, 2, [this] { shiftOctave (1); });
    addKey ("Oct " + arrow (false), keyGrey, 5, 3, [this] { shiftOctave (-1); });

    // Keyboard halves and folder load
    upperKey = &addKey ("Upper", keyBlack, 6, 0, [this] { selectHalf (1); });
    lowerKey = &addKey ("Lower", keyBlack, 6, 1, [this] { selectHalf (0); });
    addKey ("Upper", keyWhite, 7, 0, [this] { chooseFolder (1); });
    addKey ("Lower", keyWhite, 7, 1, [this] { chooseFolder (0); });
}

MiracleAudioProcessorEditor::Fader& MiracleAudioProcessorEditor::addFader (int index, bool global)
{
    auto f = std::make_unique<Fader>();
    f->index = index;
    f->global = global;

    const auto& def = defFor (index, global);
    f->slider.setSliderStyle (juce::Slider::LinearVertical);
    f->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    f->slider.setTooltip (juce::String (def.number) + "  " + def.name);

    auto* raw = f.get();
    f->slider.onValueChange = [this, raw]
    {
        if (raw->slider.isMouseButtonDown())
            showParam (raw->index, raw->global, raw->slider.getValue());
    };
    f->slider.onDragStart = [this, raw] { showParam (raw->index, raw->global, raw->slider.getValue()); };

    f->label.setText (def.shortName, juce::dontSendNotification);
    f->label.setJustificationType (juce::Justification::centred);
    f->label.setFont (uiFont (8.5f, true));
    f->label.setColour (juce::Label::textColourId, colours::silk);
    f->label.setMinimumHorizontalScale (0.65f);
    f->label.setBorderSize ({});
    f->label.setInterceptsMouseClicks (false, false);

    if (global)
    {
        f->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.apvts, globalId (index), f->slider);
        f->slider.setDoubleClickReturnValue (true, def.def);
    }

    addChildComponent (f->slider);
    addChildComponent (f->label);
    faders.push_back (std::move (f));
    return *faders.back();
}

MiracleAudioProcessorEditor::Toggle& MiracleAudioProcessorEditor::addToggle (int index, bool global)
{
    auto t = std::make_unique<Toggle>();
    t->index = index;
    t->global = global;
    t->button.setButtonText (defFor (index, global).shortName);

    auto* raw = t.get();
    t->button.onClick = [this, raw]
    {
        showParam (raw->index, raw->global, raw->button.getToggleState() ? 1.0 : 0.0);
    };

    if (global)
        t->attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            audioProcessor.apvts, globalId (index), t->button);

    addChildComponent (t->button);
    toggles.push_back (std::move (t));
    return *toggles.back();
}

void MiracleAudioProcessorEditor::selectHalf (int half)
{
    currentHalf = half;
    lowerKey->setLit (half == 0);
    upperKey->setLit (half == 1);
    lowerSlot.setActive (half == 0);
    upperSlot.setActive (half == 1);
    attachHalf();
    waveform.setHalf (half);
    lastBankVersion = -1; // forces sound list refresh
    lastTouch = 0;
    timerCallback();
}

void MiracleAudioProcessorEditor::attachHalf()
{
    using APVTS = juce::AudioProcessorValueTreeState;

    for (auto& f : faders)
    {
        if (f->global)
            continue;
        f->attachment.reset();
        f->attachment = std::make_unique<APVTS::SliderAttachment> (audioProcessor.apvts, halfId (currentHalf, f->index), f->slider);
        f->slider.setDoubleClickReturnValue (true, halfDefs[f->index].def);
    }

    for (auto& t : toggles)
    {
        if (t->global)
            continue;
        t->attachment.reset();
        t->attachment = std::make_unique<APVTS::ButtonAttachment> (audioProcessor.apvts, halfId (currentHalf, t->index), t->button);
    }

    loopModeAttachment.reset();
    loopModeAttachment = std::make_unique<APVTS::ComboBoxAttachment> (audioProcessor.apvts, halfId (currentHalf, hpLoopMode), loopModeBox);
}

void MiracleAudioProcessorEditor::setEditPanelVisible (bool open)
{
    editOpen = open;
    audioProcessor.setEditPanelOpen (open);
    editKey.setLit (open);
    setSize (editorWidth, compactHeight + (open ? editPanelHeight : 0));
}

//==============================================================================
float MiracleAudioProcessorEditor::currentValue (const ParamEntry& e) const
{
    const auto id = e.global ? globalId (e.index) : halfId (currentHalf, e.index);
    return audioProcessor.apvts.getRawParameterValue (id)->load();
}

void MiracleAudioProcessorEditor::setValue (const ParamEntry& e, int value)
{
    const auto& def = defFor (e.index, e.global);
    const auto id = e.global ? globalId (e.index) : halfId (currentHalf, e.index);

    if (auto* param = audioProcessor.apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) juce::jlimit (def.minV, def.maxV, value)));
        param->endChangeGesture();
    }
}

void MiracleAudioProcessorEditor::showSelected()
{
    if (selectedEntry >= 0)
    {
        const auto& e = entries[(size_t) selectedEntry];
        showParam (e.index, e.global, currentValue (e));
    }
}

void MiracleAudioProcessorEditor::pressDigit (int digit)
{
    lastTouch = juce::Time::currentTimeMillis();

    if (padMode == PadMode::program)
    {
        if (digit >= 1 && digit <= MiracleAudioProcessor::numPrograms)
            audioProcessor.selectHalfProgram (currentHalf, digit - 1);

        padMode = PadMode::idle;
        lastTouch = 0;
        return;
    }

    if (padMode == PadMode::value)
    {
        if (padDigits.length() < 4)
            padDigits << digit;

        const auto& e = entries[(size_t) selectedEntry];
        led.show (juce::String (e.number), padDigits, "ENTER TO CONFIRM");
        return;
    }

    padMode = PadMode::param;
    padDigits << digit;

    if (padDigits.length() < 2)
    {
        led.show (padDigits + "-", {}, "PARAMETER NUMBER");
        return;
    }

    const int number = padDigits.getIntValue();
    padDigits.clear();

    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].number == number)
        {
            selectedEntry = i;
            padMode = PadMode::value;
            showSelected();
            return;
        }

    led.show ("--", {}, "NO SUCH PARAMETER");
}

void MiracleAudioProcessorEditor::pressParam()
{
    padMode = PadMode::param;
    padDigits.clear();
    led.show ("--", {}, "PARAMETER NUMBER");
    lastTouch = juce::Time::currentTimeMillis();
}

void MiracleAudioProcessorEditor::pressValue()
{
    padMode = PadMode::value;
    padDigits.clear();
    showSelected();
}

void MiracleAudioProcessorEditor::pressStep (int delta)
{
    padDigits.clear();

    if (padMode == PadMode::param)
    {
        const int count = (int) entries.size();
        selectedEntry = ((selectedEntry + delta) % count + count) % count;
        showSelected();
        return;
    }

    const auto& e = entries[(size_t) selectedEntry];
    padMode = PadMode::value;
    setValue (e, juce::roundToInt (currentValue (e)) + delta);
    showSelected();
}

void MiracleAudioProcessorEditor::pressProgram()
{
    // The key doubles as digit 0 while a number is being typed
    if (padDigits.isNotEmpty() || padMode == PadMode::value)
    {
        pressDigit (0);
        return;
    }

    padMode = PadMode::program;
    led.show ("PR", {}, "PROGRAM 1-4");
    lastTouch = juce::Time::currentTimeMillis();
}

void MiracleAudioProcessorEditor::pressEnter()
{
    if (padMode == PadMode::value && padDigits.isNotEmpty())
    {
        setValue (entries[(size_t) selectedEntry], padDigits.getIntValue());
        padDigits.clear();
        showSelected();
    }
}

void MiracleAudioProcessorEditor::pressCancel()
{
    padMode = PadMode::idle;
    padDigits.clear();
    lastTouch = 0;
    showIdle();
}

void MiracleAudioProcessorEditor::shiftOctave (int delta)
{
    const ParamEntry e { halfDefs[hpOctave].number, hpOctave, false };
    setValue (e, juce::roundToInt (currentValue (e)) + delta);
    showParam (hpOctave, false, currentValue (e));
}

//==============================================================================
int MiracleAudioProcessorEditor::getNumRows()
{
    return audioProcessor.getNumSounds (currentHalf);
}

void MiracleAudioProcessorEditor::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    auto list = audioProcessor.getSoundList (currentHalf);
    if (list == nullptr)
        return;

    if (selected)
    {
        g.setColour (colours::lcdLit);
        g.fillRect (0, 0, width, height);
    }

    const auto main = selected ? colours::lcdDark : colours::scope;
    auto r = juce::Rectangle<int> (0, 0, width, height).reduced (6, 0);
    g.setFont (uiFont (10.5f, true));
    g.setColour (main.withAlpha (0.6f));
    g.drawText (juce::String (row + 1).paddedLeft ('0', 3), r.removeFromLeft (28), juce::Justification::centredLeft);

    const auto name = list->displayName (row);
    const bool inSubfolder = name.containsChar ('/');
    const auto folderPart = inSubfolder ? name.upToLastOccurrenceOf ("/", true, false) : juce::String();
    const auto filePart = inSubfolder ? name.fromLastOccurrenceOf ("/", false, false) : name;

    if (folderPart.isNotEmpty())
    {
        g.setColour (main.withAlpha (0.55f));
        const int w = std::min (r.getWidth() / 2, juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), folderPart)) + 2);
        g.drawText (folderPart, r.removeFromLeft (w), juce::Justification::centredLeft, true);
    }

    g.setColour (main);
    g.drawText (filePart, r, juce::Justification::centredLeft, true);
}

void MiracleAudioProcessorEditor::selectedRowsChanged (int row)
{
    if (updatingList || row < 0 || row == audioProcessor.getSoundIndex (currentHalf))
        return;

    audioProcessor.selectSound (currentHalf, row);
}

void MiracleAudioProcessorEditor::refreshSounds()
{
    lowerSlot.refresh();
    upperSlot.refresh();

    const juce::ScopedValueSetter<bool> guard (updatingList, true);
    soundList.updateContent();
    const int index = audioProcessor.getSoundIndex (currentHalf);
    if (index >= 0)
    {
        soundList.selectRow (index, true, true);
        soundList.scrollToEnsureRowIsOnscreen (index);
    }
    else
    {
        soundList.deselectAllRows();
    }
    soundList.repaint();
    waveform.repaint();

    const auto folder = audioProcessor.getFolder (currentHalf);
    folderText = folder == juce::File() ? juce::String ("NO DISK") : folder.getFileName();
    repaint (driveSlot);
}

void MiracleAudioProcessorEditor::chooseFolder (int half)
{
    if (currentHalf != half)
        selectHalf (half);

    auto start = audioProcessor.getFolder (half);
    if (! start.isDirectory())
        start = juce::File::getSpecialLocation (juce::File::userMusicDirectory);

    fileChooser = std::make_unique<juce::FileChooser> (
        juce::String ("Sample folder or audio file for ") + (half == 0 ? "LOWER" : "UPPER") + " half", start, audioPatterns);

    fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectDirectories
                                  | juce::FileBrowserComponent::canSelectFiles,
                              [this, half] (const juce::FileChooser& fc)
                              {
                                  const auto result = fc.getResult();
                                  if (result.exists())
                                      audioProcessor.loadFolderOrFile (half, result);
                              });
}

bool MiracleAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& path : files)
    {
        const juce::File file (path);
        if (file.isDirectory() || file.hasFileExtension (audioExtensions))
            return true;
    }

    return false;
}

void MiracleAudioProcessorEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    int half = currentHalf;
    if (lowerSlot.getBounds().contains (x, y))
        half = 0;
    else if (upperSlot.getBounds().contains (x, y))
        half = 1;

    for (const auto& path : files)
    {
        const juce::File file (path);
        if (file.isDirectory() || file.hasFileExtension (audioExtensions))
        {
            if (half != currentHalf)
                selectHalf (half);
            audioProcessor.loadFolderOrFile (half, file);
            return;
        }
    }
}

void MiracleAudioProcessorEditor::showParam (int index, bool global, double value)
{
    const auto& def = defFor (index, global);
    const int v = juce::roundToInt (value);
    juce::String valueText = juce::String (v);
    juce::String caption = juce::String (global ? "" : (currentHalf == 0 ? "LOWER " : "UPPER ")) + def.name;

    if (def.type == tBool)
        valueText = v != 0 ? "ON" : "OFF";
    else if (def.type == tChoice)
        caption << ": " << juce::StringArray::fromTokens (def.choices, "|", "")[v];
    else if (global && index == gpSplit)
        caption << ": " << juce::MidiMessage::getMidiNoteName (v, true, true, 3);

    led.show (juce::String (def.number), valueText, caption);
    lastTouch = juce::Time::currentTimeMillis();

    // Faders and keypad work on the same parameter
    for (int i = 0; i < (int) entries.size(); ++i)
        if (entries[(size_t) i].index == index && entries[(size_t) i].global == global)
            selectedEntry = i;
}

void MiracleAudioProcessorEditor::showIdle()
{
    const int program = audioProcessor.getHalfProgram (currentHalf) + 1;
    const int index = audioProcessor.getSoundIndex (currentHalf);
    auto list = audioProcessor.getSoundList (currentHalf);

    juce::String caption = currentHalf == 0 ? "LOWER" : "UPPER";
    caption << " P" << program << "  ";
    caption << (list == nullptr ? juce::String ("INIT WAVE") : index >= 0 ? list->displayName (index) : juce::String ("NO FILES"));

    led.show ((currentHalf == 0 ? "L" : "U") + juce::String (program), index >= 0 ? juce::String (index + 1) : juce::String(), caption);
}

void MiracleAudioProcessorEditor::timerCallback()
{
    const int version = audioProcessor.getBankVersion();
    if (version != lastBankVersion)
    {
        lastBankVersion = version;
        refreshSounds();
    }

    lowerSlot.syncOctave();
    upperSlot.syncOctave();

    pitchWheel.setPosition (audioProcessor.getPitchBend());
    modWheel.setPosition (audioProcessor.getModWheel());

    for (int i = 0; i < MiracleAudioProcessor::numPrograms; ++i)
        programKeys[i]->setLit (audioProcessor.getHalfProgram (currentHalf) == i);

    const int split = (int) audioProcessor.apvts.getRawParameterValue (globalId (gpSplit))->load();
    if (split != keyboard.splitNote)
    {
        keyboard.splitNote = split;
        keyboard.repaint();
    }

    // repaint the waveform only when something it shows has changed
    static constexpr int watched[] = { hpLoopMode, hpLoopStart, hpLoopEnd, hpSmpStart, hpSmpEnd };
    juce::int64 hash = version * 31 + currentHalf;
    for (int idx : watched)
        hash = hash * 1009 + (juce::int64) audioProcessor.apvts.getRawParameterValue (halfId (currentHalf, idx))->load();
    if (hash != (juce::int64) waveform.getProperties().getWithDefault ("hash", 0))
    {
        waveform.getProperties().set ("hash", hash);
        waveform.repaint();
    }

    const auto idleTime = juce::Time::currentTimeMillis() - lastTouch;
    if (padMode == PadMode::idle && idleTime > 2500)
        showIdle();
    else if (padMode != PadMode::idle && idleTime > 8000)
        pressCancel();
}

//==============================================================================
void MiracleAudioProcessorEditor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    background = juce::Image (juce::Image::RGB, w * backgroundScale, h * backgroundScale, true);
    const auto artworkScale = juce::AffineTransform::scale ((float) backgroundScale);

    const juce::Rectangle<int> strip (leftColumn, 0, w - leftColumn, stripHeight);
    const juce::Rectangle<int> panel (leftColumn, stripHeight, w - leftColumn, panelHeight);
    const juce::Rectangle<int> bar (leftColumn, panel.getBottom(), w - leftColumn, barHeight);

    {
        juce::Graphics g (background);
        g.addTransform (artworkScale);

        // slate body
        juce::ColourGradient body (colours::chassisHi, 0.0f, 0.0f, colours::chassis, 0.0f, (float) compactHeight, false);
        g.setGradientFill (body);
        g.fillRect (0, 0, w, compactHeight);
        g.setColour (colours::chassis.darker (0.1f));
        g.fillRect (0, compactHeight, w, h - compactHeight);

        // rear strip and bar in front of the keys are separate mouldings
        g.setColour (colours::chassis.brighter (0.06f));
        g.fillRect (strip);
        g.fillRect (bar);

        // dark control panel
        g.setColour (colours::fascia);
        g.fillRect (panel);
    }

    addGrain (background, background.getBounds(), 0.04f, 76);

    juce::Graphics g (background);
    g.addTransform (artworkScale);

    // moulding edges
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.drawHorizontalLine (panel.getY(), (float) panel.getX(), (float) panel.getRight());
    g.drawHorizontalLine (panel.getBottom(), (float) panel.getX(), (float) panel.getRight());
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (panel.getY() + 1, (float) panel.getX(), (float) panel.getRight());
    g.drawHorizontalLine (panel.getBottom() + 1, (float) panel.getX(), (float) panel.getRight());
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawVerticalLine (leftColumn, 0.0f, (float) (compactHeight - 12));
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawVerticalLine (leftColumn - 1, 0.0f, (float) (compactHeight - 12));

    // end cheeks
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRect (0, 0, 8, compactHeight);
    g.fillRect (w - 12, 0, 12, compactHeight);

    for (auto x : { leftColumn + 12, w - 26 })
    {
        drawScrew (g, { (float) x, 26.0f }, 3.5f);
        drawScrew (g, { (float) x, (float) bar.getCentreY() }, 3.5f);
    }

    // rear panel print
    g.setColour (colours::silkDim);
    g.setFont (uiFont (7.5f, true));
    int labelX = leftColumn + 120;
    for (auto* text : { "POWER", "LINE", "FUSE" })
    {
        g.drawText (text, labelX, 8, 44, 10, juce::Justification::centredLeft);
        labelX += 48;
    }
    labelX = leftColumn + 520;
    for (auto* text : { "MIDI IN", "MIDI OUT" })
    {
        g.drawText (text, labelX, 8, 50, 10, juce::Justification::centredLeft);
        labelX += 56;
    }
    g.drawText ("AUDIO OUT", leftColumn + 680, 8, 60, 10, juce::Justification::centredLeft);

    // edit key caption
    g.setColour (colours::silk);
    g.setFont (uiFont (9.0f, true));
    g.drawText ("EDIT PANEL", editKey.getBounds().translated (-66, -1).withWidth (60), juce::Justification::centredRight);

    // stripes through the panel
    const int stripeY = panel.getY() + 112;
    auto stripe = [&] (int x0, int x1)
    {
        g.setColour (colours::silk.withAlpha (0.85f));
        g.fillRect (x0, stripeY - 4, x1 - x0, 1);
        g.setColour (colours::stripeRed);
        g.fillRect (x0, stripeY, x1 - x0, 3);
    };
    stripe (leftColumn + 16, 412);
    stripe (columnX[5] + keyWidth + 14, w - 22);

    // maker badge above the left stripe
    {
        auto badge = juce::Rectangle<float> ((float) leftColumn + 18.0f, (float) stripeY - 28.0f, 76.0f, 19.0f);
        g.setColour (colours::silk);
        g.drawRoundedRectangle (badge, 3.0f, 1.2f);
        g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)).withExtraKerningFactor (0.06f));
        g.drawText ("ESS76", badge, juce::Justification::centred);
    }

    // logo
    {
        const float x = (float) columnX[7] + 52.0f, baseline = (float) stripeY - 13.0f;
        const auto heavy = juce::Font (juce::FontOptions ("Arial Black", 49.0f, juce::Font::plain)).withHorizontalScale (1.1f);

        juce::GlyphArrangement name;
        name.addLineOfText (heavy, "MIRACLE", x, baseline);
        juce::Path namePath;
        name.createPath (namePath);
        const auto nameBounds = namePath.getBounds();
        juce::ColourGradient yellow (colours::logoYellow.brighter (0.15f), 0.0f, nameBounds.getY(),
                                     colours::logoYellow.darker (0.12f), 0.0f, nameBounds.getBottom(), false);
        g.setGradientFill (yellow);
        g.fillPath (namePath);

        juce::GlyphArrangement dsp;
        dsp.addLineOfText (heavy.withHeight (31.0f), "DSP", nameBounds.getRight() + 8.0f, baseline);
        juce::Path dspPath;
        dsp.createPath (dspPath);
        g.setColour (colours::logoYellow);
        g.strokePath (dspPath, juce::PathStrokeType (1.0f));

        g.setColour (colours::silk);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::plain)).withExtraKerningFactor (0.16f));
        g.drawText ("Digital Sample Player", (int) x, stripeY + 7, 230, 14, juce::Justification::centredLeft);
        g.setColour (colours::logoYellow);
        g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::italic)).withExtraKerningFactor (0.08f));
        g.drawText ("just like 1985", w - 130, stripeY + 7, 106, 14, juce::Justification::centredRight);
    }

    // keypad captions and group lines
    g.setFont (uiFont (8.5f, true));
    for (const auto& key : keys)
    {
        const auto b = key.cap->getBounds();
        g.setColour (colours::silk);
        g.drawText (key.label, b.getX() - 10, b.getY() - 11, b.getWidth() + 20, 10, juce::Justification::centred);
    }

    auto group = [&] (int x0, int x1, int y, const juce::String& text)
    {
        g.setColour (colours::silk.withAlpha (0.8f));
        g.fillRect (x0, y, x1 - x0, 1);
        g.setFont (uiFont (7.5f, true));
        g.drawText (text, x0, y + 1, x1 - x0, 10, juce::Justification::centred);
    };

    const int groupY = panel.getY() + rowY[3] + keyHeight + 3;
    group (columnX[0], columnX[1] + keyWidth, groupY, "CONTROL");
    group (columnX[2], columnX[4] + keyWidth, groupY, "SELECT");
    group (columnX[5], columnX[5] + keyWidth, groupY, "SOUND");
    const int groupY2 = panel.getY() + rowY[1] + keyHeight + 3;
    group (columnX[6], columnX[6] + keyWidth, groupY2, "HALF");
    group (columnX[7], columnX[7] + keyWidth, groupY2, "LOAD");

    {
        const auto v = volumeFader.getBounds();
        g.setColour (colours::silk);
        g.setFont (uiFont (7.5f, true));
        g.drawText ("VOLUME", v.getX() - 10, panel.getY() + 142, v.getWidth() + 20, 10, juce::Justification::centred);
    }

    // disk tray
    {
        auto tray = juce::Rectangle<float> (18.0f, 14.0f, (float) leftColumn - 34.0f, 204.0f);
        g.setColour (colours::chassisLo);
        g.fillRoundedRectangle (tray, 3.0f);
        juce::ColourGradient lip (juce::Colours::black.withAlpha (0.7f), 0.0f, tray.getY(),
                                  juce::Colours::transparentBlack, 0.0f, tray.getY() + 16.0f, false);
        g.setGradientFill (lip);
        g.fillRoundedRectangle (tray, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (tray.reduced (0.5f), 3.0f, 1.0f);
    }

    // wheel well
    {
        auto well = juce::Rectangle<float> (18.0f, 264.0f, (float) leftColumn - 34.0f, (float) compactHeight - 264.0f - 18.0f);
        g.setColour (colours::fascia);
        g.fillRoundedRectangle (well, 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (well, 2.0f, 1.0f);
        g.setColour (colours::silk.withAlpha (0.5f));
        g.fillRect (well.getX() + 8.0f, well.getBottom() - 8.0f, well.getWidth() - 16.0f, 1.0f);

        g.setColour (colours::silk);
        g.setFont (uiFont (8.5f, true));
        for (auto* wheel : { &pitchWheel, &modWheel })
            g.drawText (wheel == &pitchWheel ? "PITCH" : "MOD/MIX",
                        wheel->getX() - 20, wheel->getY() - 14, wheel->getWidth() + 40, 11, juce::Justification::centred);
    }

    // keyboard well
    const auto kb = keyboard.getBounds();
    g.setColour (juce::Colours::black);
    g.fillRect (kb.getX() - 4, kb.getY() - 2, kb.getWidth() + 8, kb.getHeight() + 6);

    g.setColour (colours::silkDim.withAlpha (0.75f));
    g.setFont (uiFont (8.0f));
    g.drawText ("Inspired by a classic 1980s sampler. Not affiliated with Ensoniq or Creative Technology.",
                juce::Rectangle<int> (leftColumn, compactHeight - 12, w - leftColumn, 11), juce::Justification::centred);

    if (! editOpen)
        return;

    // edit panel: silkscreened section frames on the slate surface
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawHorizontalLine (compactHeight, 0.0f, (float) w);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine (compactHeight + 1, 0.0f, (float) w);

    for (const auto& s : sections)
    {
        g.setColour (colours::fascia.withAlpha (0.8f));
        g.fillRect (s.bounds);

        g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)).withExtraKerningFactor (0.1f));
        const int textW = juce::roundToInt (juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), s.title)) + 12;
        const auto titleRect = juce::Rectangle<int> (s.bounds.getX() + 10, s.bounds.getY() - 6, textW, 12);

        g.setColour (colours::silk);
        g.drawText (s.title, titleRect, juce::Justification::centred);

        const float y = (float) s.bounds.getY();
        g.setColour (colours::stripeRed);
        g.fillRect ((float) titleRect.getRight(), y - 1.0f, (float) (s.bounds.getRight() - titleRect.getRight()), 2.0f);
    }
}

void MiracleAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    // drive slot with the folder name on its label
    if (! driveSlot.isEmpty())
    {
        auto r = driveSlot.toFloat();
        g.setColour (juce::Colour (0xff0a0b0c));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);
        g.setColour (juce::Colour (0xff24272a));
        g.fillRect (r.getX() + 8.0f, r.getBottom() - 8.0f, r.getWidth() - 48.0f, 2.0f);
        g.fillRect (r.getRight() - 30.0f, r.getCentreY() - 4.0f, 20.0f, 8.0f);
        g.setColour (colours::scope.withAlpha (0.75f));
        g.setFont (uiFont (9.0f, true));
        g.drawFittedText (folderText.toUpperCase(), driveSlot.reduced (10, 0).withTrimmedRight (34).withTrimmedBottom (10), juce::Justification::centredLeft, 1, 0.7f);
    }
}

void MiracleAudioProcessorEditor::resized()
{
    sections.clear();

    const int w = getWidth();

    // keypad, display and volume inside the control panel
    for (auto& key : keys)
        key.cap->setBounds (columnX[key.column], stripHeight + rowY[key.row], keyWidth, keyHeight + 3);

    led.setBounds (500, stripHeight + 8, 190, 66);
    volumeFader.setBounds (430, stripHeight + 12, 40, 128);
    editKey.setBounds (w - 72, 10, keyWidth, keyHeight + 3);
    editKey.setLit (editOpen);

    // left column: tray, drive slot, wheels
    {
        auto tray = juce::Rectangle<int> (22, 20, leftColumn - 42, 194);
        upperSlot.setBounds (tray.removeFromTop (20));
        lowerSlot.setBounds (tray.removeFromTop (20));
        tray.removeFromTop (3);
        soundList.setBounds (tray);

        driveSlot = { 18, 226, leftColumn - 34, 30 };

        const int wheelY = 292, wheelH = compactHeight - 292 - 36;
        pitchWheel.setBounds (80, wheelY, 30, wheelH);
        modWheel.setBounds (168, wheelY, 30, wheelH);
    }

    const int keyboardTop = stripHeight + panelHeight + barHeight;
    const auto kbBounds = juce::Rectangle<int> (leftColumn + 12, keyboardTop, w - leftColumn - 30, compactHeight - keyboardTop - 16);
    keyboard.setBounds (kbBounds);
    keyboard.setKeyWidth ((float) kbBounds.getWidth() / 36.0f);

    // edit panel
    auto findFader = [this] (int index, bool global) -> Fader*
    {
        for (auto& f : faders)
            if (f->index == index && f->global == global)
                return f.get();
        return nullptr;
    };

    auto findToggle = [this] (int index, bool global) -> Toggle*
    {
        for (auto& t : toggles)
            if (t->index == index && t->global == global)
                return t.get();
        return nullptr;
    };

    for (auto& f : faders)
    {
        f->slider.setVisible (editOpen);
        f->label.setVisible (editOpen);
    }
    for (auto& t : toggles)
        t->button.setVisible (editOpen);

    waveform.setVisible (editOpen);
    loopModeBox.setVisible (editOpen);
    loopLabel.setVisible (editOpen);

    if (editOpen)
    {
        auto placeFader = [] (Fader* f, juce::Rectangle<int> r)
        {
            if (f == nullptr)
                return;
            f->label.setBounds (r.removeFromBottom (12));
            r.removeFromBottom (5);
            f->slider.setBounds (r.reduced (0, 7));
        };

        auto section = [&] (juce::Rectangle<int> area, const juce::String& title,
                            std::vector<std::pair<int, bool>> list,
                            std::vector<std::pair<int, bool>> toggleList = {})
        {
            sections.push_back ({ title, area });
            auto inner = area.reduced (6, 5);
            inner.removeFromTop (6);

            if (! toggleList.empty())
            {
                auto col = inner.removeFromRight (84);
                const int h = 24;
                col = col.withSizeKeepingCentre (col.getWidth(), h * (int) toggleList.size());
                for (auto [idx, global] : toggleList)
                    if (auto* t = findToggle (idx, global))
                        t->button.setBounds (col.removeFromTop (h));
            }

            const int fw = list.empty() ? 0 : inner.getWidth() / (int) list.size();
            for (auto [idx, global] : list)
                placeFader (findFader (idx, global), inner.removeFromLeft (fw));
        };

        auto area = juce::Rectangle<int> (0, compactHeight, w, editPanelHeight).reduced (14, 0);
        area.removeFromTop (14);

        auto row1 = area.removeFromTop (138);
        {
            auto wave = row1.removeFromLeft (620);
            sections.push_back ({ "WAVESAMPLE", wave });
            auto inner = wave.reduced (8, 6);
            inner.removeFromTop (6);

            auto right = inner.removeFromRight (4 * faderWidth + 92);
            inner.removeFromRight (8);
            waveform.setBounds (inner);

            auto combo = right.removeFromLeft (84).withSizeKeepingCentre (84, 44);
            loopLabel.setBounds (combo.removeFromTop (14));
            loopModeBox.setBounds (combo.removeFromTop (26));
            right.removeFromLeft (8);
            for (int idx : { hpSmpStart, hpSmpEnd, hpLoopStart, hpLoopEnd })
                placeFader (findFader (idx, false), right.removeFromLeft (faderWidth));

            row1.removeFromLeft (10);
            auto osc = row1.removeFromLeft (6 * faderWidth + 84 + 12);
            row1.removeFromLeft (10);
            auto lfo = row1.removeFromLeft (2 * faderWidth + 12);
            row1.removeFromLeft (10);

            section (osc, "OSCILLATORS",
                     { { hpCoarse, false }, { hpFine, false }, { hpDetune, false }, { hpOscMix, false },
                       { hpMixVel, false }, { hpLevel, false } },
                     { { hpMixMode, false }, { hpMono, false } });
            section (lfo, "LFO", { { hpLfoSpeed, false }, { hpLfoDepth, false } });
            section (row1, "FILTER", { { hpCutoff, false }, { hpReso, false }, { hpKeyTrack, false } });
        }

        area.removeFromTop (16);

        auto row2 = area.removeFromTop (138);
        {
            auto master = row2.removeFromRight (6 * faderWidth + 84 + 12);
            row2.removeFromRight (10);
            auto filterEnv = row2.removeFromLeft ((row2.getWidth() - 10) / 2);
            row2.removeFromLeft (10);

            std::vector<std::pair<int, bool>> fk, ak;
            for (int i = 0; i < 10; ++i)
            {
                fk.push_back ({ hpFAtt + i, false });
                ak.push_back ({ hpAAtt + i, false });
            }
            section (filterEnv, "FILTER ENVELOPE", fk);
            section (row2, "AMPLITUDE ENVELOPE", ak);
            section (master, "MASTER",
                     { { gpSplit, true }, { gpRate, true }, { gpOctave, true }, { gpTune, true }, { gpBend, true }, { gpVoices, true } },
                     { { gpLoFi, true } });
        }
    }

    renderBackground();
    repaint();
}
