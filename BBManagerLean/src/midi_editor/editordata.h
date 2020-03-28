#ifndef EDITORDATA_H
#define EDITORDATA_H

#include <Qt>

struct EditorData {
    struct V {
        enum {
            UserRole = Qt::UserRole,
            Id,
            Name,
            Count,
            Unsupported,
            Highlight,
        };
    private: V();
    };
    struct H {
        enum {
            UserRole = Qt::UserRole,
            Tick,
            Length,
            Played,
            Highlight,
            Quantized
        };
    private: H();
    };
    struct X {
        enum {
            UserRole = Qt::UserRole,
            Velocity,
            Notes,
            Pressed,
            Played,
            HighlightRow,
            HighlightColumn,
            DrawBorder,
            DrawNumber,
        };
    private: X();
    };
    static const char* Scale;
    static const char* BarLength;
    static const char* TickCount;
    static const char* TimeSignature;
    static const char* Drumset;
    static const char* Highlighted;
    static const char* AfterNote;
    static const char* Quantized;
    static const char* MultiNoteSelector;
    static const char* PlayerPosition;
    static const char* VisualizerScheme;
    static const char* ButtonStyle;
    static const char* ShowVelocity;
    static const char* Tempo;
private: EditorData();
};

#endif // EDITORDATA_H
