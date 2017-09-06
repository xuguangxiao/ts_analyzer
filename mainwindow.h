#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QList>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSplitter>
#include <QFile>
extern "C" {
#include "ffmpeg/include/libavformat/avformat.h"
#include "ffmpeg/include/libavutil/intreadwrite.h"
}
#include "ts_parser.h"
#include "parse_thread.h"
#include "proto_tree.h"
#include "byte_view_text.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QString cur_filename;
    QFile cur_file_handle;

    void init_stream_list_tree();
    void init_ffmpeg_packet_table();
    void init_ts_packet_table();
    void init_pes_packet_table();
    void init_nal_table();
    void init_info_table();

    void insert_info_table(QString section);
    void insert_info_table(QString title, QString info);

    QProgressBar* parse_progress;
    QLabel* status_label;
    int64_t pes_packet_total_len;
    int64_t ts_packet_total_len;

    parse_thread thread;

    int progress_bar_dissmiss_timer_id;

    // ts packet select
    QList<bool> ts_packet_select;

    QTableWidget* tableWidgetTSPacketList;
    ProtoTree* proto_tree_ts;
    ByteViewText* byte_view_ts;
    QSplitter* ts_view_splitter;
    uint8_t* cur_data_buf;

    QTableWidget* tableWidgetPESPacketList;
    ProtoTree* proto_tree_pes;
    ByteViewText* byte_view_pes;
    QSplitter* pes_view_splitter;

private:
    Ui::MainWindow *ui;

protected:
    virtual void timerEvent(QTimerEvent* e);
    virtual void showEvent(QShowEvent *event);
    virtual void resizeEvent(QResizeEvent*);

public slots:
    void slot_file_open();
    void slot_exit();
    void slot_about();
    void slot_update_parse_progress(int progress);
    void slot_parse_finished();
    void slot_stream_view();
    void slot_FFmpeg_stream_packet_view();
    void slot_ts_packet_view();
    void slot_pes_packet_view();
    void slot_h264_nal_view();
    void slot_play_stream();
    void slot_enable_selected_packets();
    void slot_disable_selected_packets();
    void slot_tableWidgetTSPacketList_cellChanged(int row, int column);
    void slot_tableWidgetTSPacketList_itemSelectionChanged();
    void slot_tableWidgetTSPacketList_customContextMenuRequested(const QPoint &pos);
    void slot_tableWidgetPESPacketList_itemSelectionChanged();
    void slot_tableWidgetPESPacketList_customContextMenuRequested(const QPoint &pos);
private slots:
    void on_treeWidgetStreamPacketList_itemSelectionChanged();
    void on_tableWidgetFFmpegAVPacketList_itemSelectionChanged();
    void on_tableWidgetH264NALList_itemSelectionChanged();
    void on_tabWidget_currentChanged(int index);
    void on_treeWidgetStreamPacketList_customContextMenuRequested(const QPoint &pos);
    void on_tableWidgetFFmpegAVPacketList_customContextMenuRequested(const QPoint &pos);
    void on_tableWidgetH264NALList_customContextMenuRequested(const QPoint &pos);
};

#endif // MAINWINDOW_H
