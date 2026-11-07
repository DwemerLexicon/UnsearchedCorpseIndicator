# Features

- Shows an icon above the corpses of dead NPCs that you have not searched.
- Also shows on ash piles and ghost remains, such as those left by ghosts, ice
  wraiths, Ash Spawn and Disintegrate.
- Can be configured to show for all corpses, or only those killed by you or a
  follower. By default, the unsearched icon shows for all corpses.
- Can be set to show upon death, or set to a custom delay.
- Icon automatically disappears once you search the body, even if you leave
  items behind.
- Icon display can be toggled on/off via a configurable hotkey (L by default).
  Can be configured to require a hotkey modifier too, such as ALT or SHIFT.

# Requirements

- SKSE
- Address Library for SKSE Plugins

# Installation

- Install with your mod manager of choice.
- Can be safely installed and uninstalled at any time.

# Configuration

Navigate to `SKSE\Plugins\UnsearchedCorpsesIndicator.ini` to adjust the
following settings:

- `fDelay` - can be set to any positive number to delay how long after death the
  icon appears above the corpse.
- `bAllCorpses` - Show icons on all dead NPCs, not just ones killed by you or
  followers (default: 1). 0 = player/follower kills only, 1 = all corpses
- `iToggleKey` - DirectX scan code for toggling icons on/off (default: 38 = L).
  You can view key scancodes here:
  https://geckwiki.com/index.php?title=DirectX_Scancodes
- `iToggleModifier` - DirectX scan code of a key that must be held with
  `iToggleKey` to toggle icons on/off, e.g. 42 = Left Shift, 56 = Left Alt
  (default: 0 = none)
- `fFadeStartDistance` - The distance at which the icon begins fading out
  (default: 1000). Icons are fully visible before this distance, then gradually
  fade until they reach `fFadeMaxDistance`.
- `fFadeMaxDistance` - The distance at which the icon fully disappears (default:
  1500). Increase this value if you want icons to remain visible from farther
  away.
- `iMaxIcons` - The maximum number of corpse icons that can be displayed at the
  same time, from 1 to 100 (default: 48). Decrease it if you want to reduce
  screen clutter.
- `fScale` - The icon's size (default: 1.0). For example, 0.6 makes it smaller
  and 1.5 makes it larger.
- `iOpacity` - How solid the icon is, from 0 (invisible) to 100 (default: 100).
  It still fades out with distance.
- `bFixedSize` - Keep the icon the same size at any distance instead of
  shrinking as it gets farther away (default: 0). 0 = shrinks with distance, 1 =
  same size at any distance
- `sColor` - Tint the icon with a hex colour (default: blank). You can pick one
  with any colour picker like
  [this one](https://htmlcolorcodes.com/color-picker/) and paste the code in,
  with or without the #. The colour is multiplied with the icon, so `FFFFFF`
  leaves it unchanged. Bright, saturated colours work best.

# Credits

- JerryYOJ for Status Indicator Framework, which versions prior to 2.0.0 used to
  show the icon - please be sure to endorse over there.
- SKSE team.
- powerof3, CharmedBaryon, alandtse and everyone who contributed to CommonLibSSE
  and CommonLibSSE-NG.

# License

Copyright (C) 2026 DwemerLexicon

Unsearched Corpse Indicator is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your option)
any later version. See [LICENSE](LICENSE).

Versions 1.3.2 and earlier were released under the MIT License.
