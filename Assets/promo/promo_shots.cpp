#include "PluginProcessor.h"
#include "PluginEditor.h"
// Renders promotional screenshots of the real editor with a demo sample library.
static void writeWav (juce::File f, std::function<float(double)> fn, double secs, double sr = 44100)
{
    f.getParentDirectory().createDirectory(); f.deleteFile(); juce::WavAudioFormat wav; auto os = f.createOutputStream();
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.release(), sr, 1, 16, {}, 0));
    juce::AudioBuffer<float> b (1, (int) (sr * secs));
    for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (0, i, fn (i / sr));
    w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}
static float noise() { static juce::Random r (76); return r.nextFloat() * 2.0f - 1.0f; }
static double tone (double f, double t, int harm) { double s = 0; for (int h = 1; h <= harm; ++h) s += std::sin (2 * M_PI * f * h * t) / h; return s * 0.6; }

int main (int, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File lib (argv[1]), out (argv[2]);
    lib.deleteRecursively(); out.createDirectory();
    const double C2 = 65.41, C3 = 130.81, C4 = 261.63;
    writeWav (lib.getChildFile ("Bass/Slap Bass C2.wav"), [=](double t){ return (float) (std::exp (-t * 3) * tone (C2, t, 8)); }, 1.5);
    writeWav (lib.getChildFile ("Drums/Funk Break 100bpm.wav"), [](double t){
        const double beat = 0.6, b = std::fmod (t, beat), e = std::fmod (t, beat / 2); const int n = (int) (t / beat) % 4;
        double s = 0.25 * std::exp (-e * 60) * noise();                                   // hats
        if (n == 0 || n == 2) s += 0.9 * std::exp (-b * 18) * std::sin (2 * M_PI * (50 + 90 * std::exp (-b * 30)) * b);  // kick
        if (n == 1 || n == 3) s += 0.7 * std::exp (-b * 22) * noise();                       // snare
        return (float) s; }, 4.8);
    writeWav (lib.getChildFile ("Drums/Snare Gate.wav"), [](double t){ return (float) (t < 0.35 ? 0.8 * noise() * std::exp (-t * 4) : 0.0); }, 0.5);
    writeWav (lib.getChildFile ("Keys/Bell Piano C4.wav"), [=](double t){ return (float) (std::exp (-t * 1.5) * (std::sin (2 * M_PI * C4 * t) + 0.4 * std::sin (2 * M_PI * C4 * 3.01 * t))); }, 2.5);
    writeWav (lib.getChildFile ("Keys/Electric Tine C3.wav"), [=](double t){ return (float) (std::exp (-t * 2) * std::sin (2 * M_PI * C3 * t + 2 * std::exp (-t * 6) * std::sin (2 * M_PI * C3 * 7 * t))); }, 2.0);
    writeWav (lib.getChildFile ("Pads/Analog Sweep C3.wav"), [=](double t){ return (float) (std::min (1.0, t) * tone (C3, t, 2 + (int) (t * 4))); }, 3.0);
    writeWav (lib.getChildFile ("Pads/Glass Pad C3.wav"), [=](double t){ return (float) (std::sin (t * 1.2) * 0.5 * (std::sin (2 * M_PI * C3 * t) + std::sin (2 * M_PI * C3 * 2.005 * t))); }, 3.0);
    writeWav (lib.getChildFile ("Strings/Digital Choir C3.wav"), [=](double t){ return (float) (std::min (1.0, t * 3) * (0.8 + 0.2 * std::sin (t * 30)) * tone (C3, t, 5)); }, 2.5);
    writeWav (lib.getChildFile ("Strings/Orchestra Hit C3.wav"), [=](double t){ return (float) (std::exp (-t * 2.2) * (tone (C3, t, 10) + 0.3 * std::exp (-t * 20) * noise())); }, 2.0);

    MiracleAudioProcessor p; p.prepareToPlay (48000, 512);
    auto set = [&](const char* id, float v){ auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    auto blocks = [&](int n, int note = -1){ juce::AudioBuffer<float> b (2, 512);
        for (int k = 0; k < n; ++k) { juce::MidiBuffer m; if (k == 0 && note >= 0) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0); p.processBlock (b, m); } };
    auto shot = [&](juce::Component& e, const juce::String& n){ juce::MessageManager::getInstance()->runDispatchLoopUntil (400);
        auto img = e.createComponentSnapshot (e.getLocalBounds(), true, 2.0f); juce::PNGImageFormat png; auto f = out.getChildFile (n); f.deleteFile();
        auto os = f.createOutputStream(); png.writeImageToStream (img, *os); };
    auto pressUpper = [](MiracleAudioProcessorEditor& e){ for (auto* c : e.getChildren()) if (auto* k = dynamic_cast<KeyCap*> (c)) if (k->getName() == "Upper") { k->onClick(); break; } };

    p.loadFolder (0, lib.getChildFile ("Bass")); p.loadFolder (1, lib); p.selectSound (1, 1);   // Drums/Funk Break 100bpm
    set ("U_loopmode", 1); set ("U_f_peak", 40); set ("U_oscmix", 45); set ("U_detune", 20); set ("U_tsync", 1);
    blocks (2);
    const int sync = p.getSyncKey (1);
    printf ("sync key %d, sample %.1f bpm, host %.1f bpm\n", sync, p.getSampleBpm (1), p.getHostBpm());
    blocks (160, sync);
    p.keyboardState.noteOn (1, sync, 0.8f); p.keyboardState.noteOn (1, 36, 0.8f); p.keyboardState.noteOn (1, 43, 0.8f);
    {
        std::unique_ptr<MiracleAudioProcessorEditor> e (dynamic_cast<MiracleAudioProcessorEditor*> (p.createEditor()));
        pressUpper (*e);
        e->setEditPanelVisible (false); shot (*e, "compact.png");
        e->setEditPanelVisible (true);  shot (*e, "edit.png");
    }
    // reverse demo: orchestra hit played backwards, note held, playhead running
    for (int n : { sync, 36, 43 }) p.keyboardState.noteOff (1, n, 0.0f);
    { juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOff (1, sync), 0); p.processBlock (b, m); }
    blocks (60);
    set ("U_tsync", 0); set ("U_loopmode", 0); set ("U_reverse", 1); p.selectSound (1, 8);   // Strings/Orchestra Hit C3
    blocks (2); blocks (70, 60);
    p.keyboardState.noteOn (1, 60, 0.8f); p.keyboardState.noteOn (1, 64, 0.8f); p.keyboardState.noteOn (1, 67, 0.8f);
    {
        std::unique_ptr<MiracleAudioProcessorEditor> e (dynamic_cast<MiracleAudioProcessorEditor*> (p.createEditor()));
        pressUpper (*e); e->setEditPanelVisible (true); shot (*e, "edit_reverse.png");
    }
    return 0;
}
