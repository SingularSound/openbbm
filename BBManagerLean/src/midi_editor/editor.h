#ifndef EDITOR_H
#define EDITOR_H

#include <list>
#include <functional>
#include <QObject>
#include <QTableWidget>

#include <model/filegraph/midiParser.h>

class SongPart : public QObject
{
    Q_OBJECT
public:
    SongPart(const QString& name, const QByteArray& data) : m_name(name), m_data(data) {}
    ~SongPart() { for (const auto& dtor : m_dtor) if (dtor) dtor(); }
    operator QByteArray() const { return m_cache; }
    QString& name() { return m_name; }
    MIDIPARSER_MidiTrack& data() { return m_data; }
    std::map<int, int>& qq() { return m_qq; }
    void OnDestroy(const std::function<void()>& dtor) { m_dtor.push_back(dtor); }

public slots:
    void accept() { emit done(this, true); }
    void cancel() { emit done(this, false); }

signals:
    void done(SongPart* self, bool accept);
    void quantize(bool original = false);
    void quantizeChanged(bool quantized);
    void colorSchemeChanged(bool color);
    void barCountChanged(int bars);
    void tempoChanged(int bpm);
    void timeSignatureChanged(int num, int den);
    void nTickChanged(int nTick);
    void visualizerSchemeChanged(int scheme); // 0 - disabled, 1 - no scroll, 2 - ensure visible, 3 - always in the middle
    void buttonStyleChanged(bool button);
    void showVelocityChanged(bool show);

private:
    QString m_name;
    MIDIPARSER_MidiTrack m_data;
    std::map<int, int> m_qq;
    QByteArray m_cache;
    std::list<std::function<void()>> m_dtor;
};

#include <QScrollBar>
#include <set>
class EditorTableWidget : public QTableWidget
{
    friend class EditorVerticalHeader;
    friend class EditorHorizontalHeader;
    Q_OBJECT
    class QUndoStack* mp_undoStack;
    class EditorDialog* m_editor;
    QTimer* m_resizer;
    bool m_dont_leave;
    bool m_resize_block;

protected:
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void keyPressEvent(QKeyEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void paintEvent(QPaintEvent* event);

public slots:
    void zoom(double = 0);
    void changeLayout(SongPart* sp, std::set<int> grid = std::set<int>(), const MIDIPARSER_MidiTrack* data = nullptr, const std::map<int, int>* qq = nullptr);
    void splitMergeColumns(int from, int to, std::vector<int>& parts);
    void removeColumn(int col);
    void resizeColumnsToContents();

signals:
    void accept();
    void cancel();
    void changed();

public:
    explicit EditorTableWidget(QWidget* parent = nullptr);
    ~EditorTableWidget();
    std::set<int> grid() const;
    QUndoStack* undoStack() const { return mp_undoStack; }
    int minimumColumnSize() const;
    int note(int row, int col) const;
    void setNote(int row, int col, int vel, bool count = true, int offset = -1);
    void dropUnusedUnsupportedRows();
    MIDIPARSER_MidiTrack collect() const;
};
#endif // EDITOR_H
