#pragma once

#include <QIcon>

enum class MediaGlyph { Restart, Back, Play, Pause, Step };

QIcon mediaIcon(MediaGlyph glyph);
