#ifndef PLAYDIALOG_H
#define PLAYDIALOG_H

#include <QDialog>
#include "ts_parser.h"
#include <QList>
#include <QtAV>
#include <QtAVWidgets>
#include "tsstreamdevice.h"

namespace Ui {
class PlayDialog;
}

using namespace QtAV;

class PlayDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlayDialog(ts_parse_context* tpc, QList<bool> &ts_packet_select, QString &filename, QWidget *parent = 0);
    ~PlayDialog();

    ts_parse_context* tpc;
    QList<bool> ts_packet_select;

    QtAV::AVPlayer *mpPlayer;
    QtAV::VideoRenderer *mpRenderer;
    QtAV::LibAVFilterVideo *mpVideoFilter;
    QtAV::LibAVFilterAudio *mpAudioFilter;
    QtAV::SubtitleFilter *mpSubtitle;

    bool setRenderer(VideoRenderer* renderer);

    TSStreamDevice tssd;

private:
    Ui::PlayDialog *ui;
protected:
    virtual void showEvent(QShowEvent*);
    virtual void closeEvent(QCloseEvent*);
public slots:
    void slot_update_stream_info(int ts_index, int pes_index);
};

#endif // PLAYDIALOG_H
