# Notices and third-party licences

Quadrilo is free software, licensed under the GNU General Public License,
version 3 or (at your option) any later version. See [`LICENSE`](LICENSE).

Copyright (C) 2026 duddex

Quadrilo is an independent project. The delay engine and the user interface
(drawn in code, with no third-party artwork) are original. No code, samples or
graphics from other plugins are included.

## JUCE

Quadrilo is built with the [JUCE framework](https://juce.com), included as a
git submodule in `libs/JUCE`. Quadrilo uses JUCE under the
**GNU Affero General Public License v3.0** (one of JUCE's two licence options).
See [`licenses/AGPL-3.0.txt`](licenses/AGPL-3.0.txt) and `libs/JUCE/LICENSE.md`.

    Copyright (c) Raw Material Software Limited

GPLv3 section 13 allows combining GPLv3 code with AGPLv3 code. The
Quadrilo source code stays under GPLv3-or-later, and the JUCE parts stay under
AGPLv3. Distributed binaries contain both, so whoever distributes them must
follow both licences. In particular, the complete corresponding source code
must be made available.

JUCE bundles more third-party software, listed in `libs/JUCE/JUCE.spdx.json`.

## VST3 SDK

The VST3 plugin is built with the Steinberg VST3 SDK, which JUCE bundles and
which is licensed under the MIT licence. See
`libs/JUCE/modules/juce_audio_processors/format_types/VST3_SDK/` in the JUCE
submodule for its licence text.

VST is a registered trademark of Steinberg Media Technologies GmbH.
