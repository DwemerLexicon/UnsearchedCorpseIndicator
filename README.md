# Features

- Shows an icon above the corpses of dead NPCs that you have not searched.
- Can be configured to show for all corpses, or only those killed by you or a follower. By default, the unsearched icon shows for all  corpses.
- Can be set to show immediately upon death, or set to a custom delay.
- Icon automatically disappears once you search the body, even if you leave items behind.
- Icon display can be toggled on/off via a configurable hotkey (L by default).

# Requirements

- Status Indicator Framework﻿ (SIF) is required

# Installation

- Install with your mod manager of choice.
- Can install and uninstall anytime.

# Configuration

Navigate to `SKSE\Plugins\UnsearchedCorpsesIndicator.ini` to adjust the following settings:

- `fDelay` - can be set to any positve number to delay how long after death the icon appears above the corpse.
- `bAllCorpses` - Show icons on all dead NPCs, not just ones killed by you or followers (default: 1). 0 = player/follower kills only, 1 = all corpses
- `iToggleKey` - DirectX scan code for toggling icons on/off (default: 38 = L)
- `iToggleModifier` - DirectX scan code of a key that must be held with `iToggleKey`, e.g. 42 = Left Shift (default: 0 = none)

If you navigate to `SKSE\Plugins\SIF\UnsearchedCorpsesIndicator.json`, you can also adjust the following:

- `"fadeMaxDistance": 1500` - The distance at which the icon fully disappears. Increase this value if you want icons to remain visible from farther away.
- `"fadeStartDistance": 1000` - The distance at which the icon begins fading out. Icons are fully visible before this distance, then gradually fade until they reach fadeMaxDistance.
- `"maxInstances": 48` - The maximum number of corpse icons that can be displayed at the same time (48 is also SIF's limit). Increase this if you want more icons visible at once. Decrease it if you want to reduce screen clutter.

# Credits

- JerryYOJ for Status Indicator Framework - please be sure to endorse over there.
- SKSE team.
- powerof3 and the CommonLibSSE contributors.

# License

Copyright (C) 2026 DwemerLexicon

Unsearched Corpse Indicator is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. See [LICENSE](LICENSE).

Versions 1.3.2 and earlier were released under the MIT License.
