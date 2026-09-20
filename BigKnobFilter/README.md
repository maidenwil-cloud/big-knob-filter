# Big Knob — Filtre Passif 11 positions (VST3 / AU)

Projet source JUCE. Compile en local sur ton Mac pour obtenir un vrai plugin
`.vst3` et `.component` (AU), utilisables dans Reaper.

## Pré-requis

- **Xcode** (depuis l'App Store, avec les Command Line Tools : `xcode-select --install`)
- **CMake** ≥ 3.22 : `brew install cmake` (installe [Homebrew](https://brew.sh) si besoin)
- Une connexion internet la première fois (CMake télécharge JUCE automatiquement depuis GitHub)

## Compiler

Dans le Terminal, depuis ce dossier (`BigKnobFilter/`) :

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

La première commande télécharge JUCE (~300 Mo) et génère un projet Xcode.
La seconde compile les trois formats (VST3, AU, Standalone) en Release.

Tu peux aussi ouvrir `build/BigKnobFilter.xcodeproj` dans Xcode et faire
**Product → Build** sur le scheme `BigKnobFilter_All` si tu préfères l'interface graphique.

## Installer les plugins

`COPY_PLUGIN_AFTER_BUILD` est activé dans le `CMakeLists.txt` : à la fin du
build, JUCE copie automatiquement les plugins aux bons endroits :

- VST3 → `~/Library/Audio/Plug-Ins/VST3/Big Knob — Passive Filter.vst3`
- AU → `~/Library/Audio/Plug-Ins/Components/Big Knob — Passive Filter.component`

Si ça n'a pas fonctionné automatiquement, copie-les toi-même depuis
`build/BigKnobFilter_artefacts/Release/VST3/` et
`build/BigKnobFilter_artefacts/Release/AU/`.

## Faire apparaître le plugin dans Reaper

1. Ouvre Reaper → **Options → Preferences → Plug-ins → VST**.
2. Clique **Re-scan** (ou **Clear cache/re-scan** si tu ne le vois pas).
3. Le plugin apparaît sous le nom **"Big Knob — Passive Filter"** dans la
   liste FX (catégorie *Fx/Filter*, développeur *Independent*).
4. Pour l'AU : Reaper le détecte aussi automatiquement via
   `~/Library/Audio/Plug-Ins/Components` — pas de réglage séparé nécessaire.

Si macOS bloque le plugin (Gatekeeper, "développeur non identifié") parce
qu'il n'est pas signé/notarié, exécute une fois :

```bash
xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Big Knob — Passive Filter.vst3"
xattr -cr ~/Library/Audio/Plug-Ins/Components/"Big Knob — Passive Filter.component"
```

(Comme le plugin est compilé en local et non téléchargé, il n'a en général
même pas l'attribut de quarantaine — cette étape est un filet de sécurité.)

## Ce que fait le plugin

Même moteur que la version web : commutateur à 11 crans fixes (pas un potard
continu), résonance qui monte vers les butées, perte de niveau passive et
grain (waveshaper tanh) qui augmente vers les extrêmes — jusqu'au
sifflement en position 11. Les 11 réglages exacts sont dans
`Source/PluginProcessor.h`, tableau `kPositions`.

## Paramètres automatisables (DAW)

- **Position** (1–11, entier) — automatisable dans Reaper comme n'importe quel paramètre
- **Grain** (0–1) — quantité de saturation passive
- **Output** (0–1.5) — gain de sortie
- **Bypass** (on/off)
