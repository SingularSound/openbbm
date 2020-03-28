#ifndef EDITOR_UTIL_H
#define EDITOR_UTIL_H

#include <QObject>
#include <QWidget>
#include <QAbstractItemModel>

#include <midi_editor/editordata.h>

static std::map<int, std::pair<int, int>> quantize_map(int b) {
    std::map<int, std::pair<int, int>> qs;
    for (int i = 7; i+1; --i) {
        qs[b/double(1<<i)] = std::make_pair(i,0); // usual notes
    }
    for (int i = 7; i+1; --i) {
        qs[b/double(1<<i)*3/2] = std::make_pair(i,-1); // dotted notes
    }
    for (int i = 5; i+1; --i) {
        qs[b/double(1<<i)/3] = std::make_pair(i+1,3); // triple notes
    }
    for (int i = 4; i+1; --i){
        qs[b/double(1<<i)/5] = std::make_pair(i+2,5); // quintuple notes
    }
    return qs;
}

static bool make_quantized(QAbstractItemModel* m) {
    auto b = m->property(EditorData::BarLength).toInt();
    auto qm = quantize_map(b);
    auto mcc = m->columnCount()-m->property(EditorData::AfterNote).toBool();
    auto q = true;
    std::vector<int> l(mcc);
    auto qme = qm.end();
    for (int i = mcc-1; i+1; --i) {
        auto it = qm.find(m->headerData(i, Qt::Horizontal, EditorData::H::Length).toInt());
        q = it != qme;
        if (q) {
            l[i] = it->first;
        } else {
            break;
        }
    }
    m->setProperty(EditorData::Quantized, q ? QVariant(true) : QVariant());
    if (!q) {
        for (auto i = mcc-1; i+1; --i) {
            m->setHeaderData(i, Qt::Horizontal, QVariant(), EditorData::H::Quantized);
        }
    } else {
        auto ts = m->property(EditorData::TimeSignature).toPoint();
        auto nl = b/ts.y();
        std::vector<int> qq(mcc+2), qp(mcc);
        for (int i = 0, t = 0, prev = 0; i < mcc; t += l[i++]) {
            auto& cur = qm.find(l[i])->second;
            if (!qp[i]) {
                if (auto tuplet = cur.second){
                    if (tuplet > 0) {
                       auto ok = true;
                        for (int k = tuplet-1; k && ok; --k) {
                            ok |= qm.find(l[i+k])->second == cur;
                        }
                        if (ok) {
                            for (int k = tuplet-1; k+1; --k) {
                                qp[i+k] = 1+k;
                            }
                        }
                    }
                }
            }
            if (i) {
                auto p = qp[i-1];
                qq[i] = t%nl ? (prev < cur.first ? prev : cur.first) - 2 - (p && p==cur.second) : 0;
            }
            prev = cur.first;
        }
        for (auto i = mcc-1; i+1; --i) {
            auto& cur = qm.find(l[i])->second;
            m->setHeaderData(i, Qt::Horizontal, QPoint(cur.first, (cur.second & 0xFF) | ((qp[i] & 0xFF) << 8) | ((qq[i] & 0xFF) << 16) | ((qq[i+1] & 0xFF) << 24)), EditorData::H::Quantized);
        }
    }
    return q;
}

static std::map<int, std::pair<std::pair<int, int>, double>> quantize_opt(double scale)
{
    static const int tuplet_type[] = { 1, 3, 5, };
    static const int tuplet_num[] = { 1, 2, 4, };
    std::map<int, std::pair<std::pair<int, int>, double>> ret;
    for (unsigned long t = 0; t < sizeof(tuplet_type)/sizeof(tuplet_type[0]); ++t) {
        auto n = tuplet_type[t];
        for (int i = 0, den; (den = n*(1<<i)) < 128; ++i) {
            auto fig = scale*den;
            int num = static_cast<int>(fig+.5);
            if (!num){
                continue;
            }
            auto diff = abs(scale-double(num)/den);
            if (ret.find(n) == ret.end() || diff < ret[n].second) {
                auto d = den*tuplet_num[t]/n;
                while (!(num & 1) && !(d & 1)) {
                    num >>= 1;
                    d >>= 1;
                }
                ret[n] = std::make_pair(std::make_pair(num, d), diff);
            }
        }
    }
    for (unsigned long t = 0; t < sizeof(tuplet_type)/sizeof(tuplet_type[0]); ++t){
        auto n = tuplet_type[t];
        if (n){
            // XXX: check this
            if (t ? !(ret[n].first.first % n) : !ret[n].first.first && !ret[n].first.second){
                ret.erase(n);
            }
        }
    }
    return ret;
}

#endif // EDITOR_UTIL_H
