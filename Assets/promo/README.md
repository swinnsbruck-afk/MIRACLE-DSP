# Promotional graphics

Rebuilds `Release/<version>/Mockups` from real editor screenshots.

1. Build the screenshot tool (needs JUCE at `~/Developer/JUCE`):

   ```bash
   cmake -S Assets/promo -B ~/Developer/MiracleDSP-promo-build -DCMAKE_BUILD_TYPE=Release
   cmake --build ~/Developer/MiracleDSP-promo-build -j8
   ```

2. Render the screenshots. The first argument is a scratch folder; the tool deletes it
   and writes a demo sample library there. The tool saves plugin settings to
   `~/Library/Application Support/MIRACLE DSP`, so back that folder up first.

   ```bash
   ~/Developer/MiracleDSP-promo-build/PromoShots_artefacts/Release/PromoShots "/tmp/promo/Sample Library" /tmp/promo/shots
   ```

3. Compose the mockups (needs Pillow):

   ```bash
   python3 Assets/promo/make_mockups.py /tmp/promo/shots "Release/MIRACLE DSP 1.4.0/Mockups"
   ```
