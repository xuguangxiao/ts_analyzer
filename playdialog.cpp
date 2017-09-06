#include "playdialog.h"
#include "ui_playdialog.h"
#include <QMessageBox>
#include <QMetaMethod>

PlayDialog::PlayDialog(ts_parse_context *tpc, QList<bool> &ts_packet_select, QString& filename, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlayDialog),
    tssd(tpc, ts_packet_select, filename, this)
{
    mpPlayer = NULL;
    mpRenderer = NULL;
    mpVideoFilter = NULL;
    mpAudioFilter = NULL;
    mpSubtitle = NULL;
    this->tpc = tpc;
    this->ts_packet_select.clear();
    this->ts_packet_select.append(ts_packet_select);
    ui->setupUi(this);

    mpSubtitle = new SubtitleFilter(this);

    mpPlayer = new AVPlayer(this);
    VideoRenderer *vo = VideoRenderer::create(VideoRendererId_Direct2D);
    if (!vo || !vo->isAvailable() || !vo->widget()) {
        QMessageBox::critical(0, QString::fromLatin1("QtAV"), tr("Video renderer is ") + tr("not availabe on your platform!"));
    }
    setRenderer(vo);
    mpSubtitle->setPlayer(mpPlayer);
    mpVideoFilter = new LibAVFilterVideo(this);
    mpVideoFilter->setEnabled(true);
    mpPlayer->installFilter(mpVideoFilter);
    mpVideoFilter->setOptions("");
    mpAudioFilter = new LibAVFilterAudio(this);
    mpAudioFilter->setEnabled(true);
    mpAudioFilter->installTo(mpPlayer);
    mpAudioFilter->setOptions("");
    connect(&tssd, SIGNAL(update_stream_info(int,int)), this, SLOT(slot_update_stream_info(int,int)));
}

PlayDialog::~PlayDialog()
{
    delete ui;
}

void PlayDialog::showEvent(QShowEvent *) {
    mpPlayer->stop(); //if no stop, in setPriority decoder will reopen
    mpPlayer->setFrameRate(0.0);
    mpPlayer->setInterruptOnTimeout(true);
    mpPlayer->setInterruptTimeout(30000.0);
    mpPlayer->setBufferMode(QtAV::BufferPackets);
    mpPlayer->setBufferValue(-1);
    mpPlayer->setRepeat(false);

    tssd.open(QIODevice::ReadOnly);
    mpPlayer->setIODevice(&tssd);
    mpPlayer->play();
    // mpPlayer->play(tr("cuc_ieschool.ts"));

//     static const QMetaMethod update_stream_signal = QMetaMethod::fromSignal(&TSStreamDevice::update_stream_info);
//     if (!tssd.isSignalConnected(update_stream_signal)) {
//     }
}

bool PlayDialog::setRenderer(VideoRenderer *renderer) {
    if (!renderer)
        return false;
    if (!renderer->widget()) {
        QMessageBox::warning(0, QString::fromLatin1("QtAV"), tr("Can not use this renderer"));
        return false;
    }
    mpSubtitle->uninstall();
    // renderer->widget()->setMouseTracking(true); //mouseMoveEvent without press.
    mpPlayer->setRenderer(renderer);
    QWidget *r = 0;
    if (mpRenderer)
        r = mpRenderer->widget();
    //release old renderer and add new
    if (r) {
        ui->gridLayout->removeWidget(r);
        if (r->testAttribute(Qt::WA_DeleteOnClose)) {
            r->close();
        } else {
            r->close();
            delete r;
        }
        r = 0;
    }
    mpRenderer = renderer;
    ui->gridLayout->addWidget(renderer->widget());
    mpPlayer->renderer()->forcePreferredPixelFormat(false);
    mpSubtitle->installTo(mpRenderer);
    return true;
}

void PlayDialog::closeEvent(QCloseEvent *) {
    mpPlayer->stop();
    emit finished(0);
}

void PlayDialog::slot_update_stream_info(int ts_index, int pes_index) {
    this->setWindowTitle(tr("Playing TS: %1 PES %2").arg(ts_index).arg(pes_index));
}
