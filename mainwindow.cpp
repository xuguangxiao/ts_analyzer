#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QFile>
#include "playdialog.h"

enum {
    STREAM_TABLE_COLUMN_STREAM = 0,
    STREAM_TABLE_COLUMN_INDEX,
    STREAM_TABLE_COLUMN_LENGTH,
    STREAM_TABLE_COLUMN_OFFSET,
    STREAM_TABLE_COLUMN_PTS,
    STREAM_TABLE_COLUMN_DTS,
    STREAM_TABLE_COLUMN_TS_COUNT,
    STREAM_TABLE_COLUMN_START_TS_INDEX,
    STREAM_TABLE_COLUMN_END_TS_INDEX,
    STREAM_TABLE_COLUMN_COUNT,
};

const char* stream_table_col_headers[] = {
    "Stream",
    "Index",
    "Length",
    "Offset",
    "PTS",
    "DTS",
    "TS Count",
    "Start TS",
    "End TS",
};

const int stream_table_col_widths[] = {
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    100,
    100,
};

enum {
    FFMPEG_PACKET_TABLE_COLUMN_INDEX = 0,
    FFMPEG_PACKET_TABLE_COLUMN_TYPE,
    FFMPEG_PACKET_TABLE_COLUMN_LENGTH,
    FFMPEG_PACKET_TABLE_COLUMN_OFFSET,
    FFMPEG_PACKET_TABLE_COLUMN_PTS,
    FFMPEG_PACKET_TABLE_COLUMN_DTS,
    FFMPEG_PACKET_TABLE_COLUMN_TS_COUNT,
    FFMPEG_PACKET_TABLE_COLUMN_START_TS_INDEX,
    FFMPEG_PACKET_TABLE_COLUMN_END_TS_INDEX,
    FFMPEG_PACKET_TABLE_COLUMN_COUNT,
};

const char* ffmpeg_packet_table_col_headers[] = {
    "Index",
    "Stream Type",
    "Length",
    "Offset",
    "PTS",
    "DTS",
    "TS Count",
    "Start TS",
    "End TS",
};

const int ffmpeg_packet_table_col_widths[] = {
    -1,
    120,
    -1,
    -1,
    -1,
    -1,
    -1,
    100,
    100,
};

enum {
    TS_PACKET_TABLE_COLUMN_SELECT = 0,
    TS_PACKET_TABLE_COLUMN_INDEX,
    TS_PACKET_TABLE_COLUMN_OFFSET,
    TS_PACKET_TABLE_COLUMN_PID,
    TS_PACKET_TABLE_COLUMN_TYPE,
    TS_PACKET_TABLE_COLUMN_CC,
    TS_PACKET_TABLE_COLUMN_FRAME,
    TS_PACKET_TABLE_COLUMN_PES_INDEX,
    TS_PACKET_TABLE_COLUMN_COUNT,
};

const char* ts_packet_table_col_headers[] = {
    "Select",
    "Index",
    "Offset",
    "PID",
    "Type",
    "Continuty Counter",
    "Frame/Slice",
    "PES Packet Index",
};

const int ts_packet_table_col_widths[] = {
    -1,
    -1,
    -1,
    -1,
    -1,
    170,
    120,
    170,
};

enum {
    PES_PACKET_TABLE_COLUMN_INDEX = 0,
    PES_PACKET_TABLE_COLUMN_OFFSET,
    PES_PACKET_TABLE_COLUMN_LENGTH,
    PES_PACKET_TABLE_COLUMN_STREAM_ID,
    PES_PACKET_TABLE_COLUMN_STREAM_TYPE,
    PES_PACKET_TABLE_COLUMN_TS_PID,
    PES_PACKET_TABLE_COLUMN_TS_COUNT,
    PES_PACKET_TABLE_COLUMN_START_TS_INDEX,
    PES_PACKET_TABLE_COLUMN_END_TS_INDEX,
    PES_PACKET_TABLE_COLUMN_PTS,
    PES_PACKET_TABLE_COLUMN_DTS,
    PES_PACKET_TABLE_COLUMN_NAL_COUNT,
    PES_PACKET_TABLE_COLUMN_SLICE_TYPES,
    PES_PACKET_TABLE_COLUMN_COUNT,
};

const char* pes_packet_table_col_headers[] = {
    "Index",
    "Offset",
    "Length",
    "Stream ID",
    "Stream Type",
    "TS PID",
    "TS Count",
    "Start TS",
    "End TS",
    "PTS",
    "DTS",
    "NAL Count",
    "Slice Types",
};

const int pes_packet_table_col_widths[] = {
    -1,
    -1,
    -1,
    -1,
    120,
    -1,
    -1,
    100,
    100,
    -1,
    -1,
    -1,
    400,
};

enum {
    NAL_TABLE_COLUMN_INDEX = 0,
    NAL_TABLE_COLUMN_TYPE,
    NAL_TABLE_COLUMN_LENGTH,
    NAL_TABLE_COLUMN_SLICE_TYPE,
    NAL_TABLE_COLUMN_PES_PACKET_INDEX,
    NAL_TABLE_COLUMN_TS_COUNT,
    NAL_TABLE_COLUMN_START_TS_INDEX,
    NAL_TABLE_COLUMN_END_TS_INDEX,
    NAL_TABLE_COLUMN_COUNT,
};

const char* nal_table_col_headers[] = {
    "Index",
    "Type",
    "Length",
    "Slice Type",
    "PES Packet Index",
    "TS Count",
    "Start TS",
    "End TS",
};

const int nal_table_col_widths[] = {
    -1,
    400,
    -1,
    -1,
    150,
    -1,
    100,
    100,
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ts_view_splitter = new QSplitter(Qt::Vertical, ui->tabTSPacketView);
    ui->gridLayoutTSView->addWidget(ts_view_splitter);
    tableWidgetTSPacketList = new QTableWidget(ts_view_splitter);
    ts_view_splitter->addWidget(tableWidgetTSPacketList);
    proto_tree_ts = new ProtoTree(PROTO_TREE_TYPE_TS, ts_view_splitter);
    ts_view_splitter->addWidget(proto_tree_ts);
    byte_view_ts = new ByteViewText(ts_view_splitter);
    byte_view_ts->setMonospaceFont(QApplication::font());
    ts_view_splitter->addWidget(byte_view_ts);
    connect(proto_tree_ts, SIGNAL(protoItemSelected(int,int,int,int)), byte_view_ts, SLOT(protoItemSelected(int,int,int,int)));
    cur_data_buf = NULL;

    connect(tableWidgetTSPacketList, SIGNAL(itemSelectionChanged()), this, SLOT(slot_tableWidgetTSPacketList_itemSelectionChanged()));
    connect(tableWidgetTSPacketList, SIGNAL(cellChanged(int,int)), this, SLOT(slot_tableWidgetTSPacketList_cellChanged(int,int)));
    connect(tableWidgetTSPacketList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slot_tableWidgetTSPacketList_customContextMenuRequested(QPoint)));

    pes_view_splitter = new QSplitter(Qt::Vertical, ui->tabPESPacketView);
    ui->gridLayoutPESView->addWidget(pes_view_splitter);
    tableWidgetPESPacketList = new QTableWidget(pes_view_splitter);
    pes_view_splitter->addWidget(tableWidgetPESPacketList);
    proto_tree_pes = new ProtoTree(PROTO_TREE_TYPE_PES, pes_view_splitter);
    pes_view_splitter->addWidget(proto_tree_pes);
    byte_view_pes = new ByteViewText(pes_view_splitter);
    byte_view_pes->setMonospaceFont(QApplication::font());
    pes_view_splitter->addWidget(byte_view_pes);
    connect(proto_tree_pes, SIGNAL(protoItemSelected(int, int, int, int)), byte_view_pes, SLOT(protoItemSelected(int, int, int, int)));

    connect(tableWidgetPESPacketList, SIGNAL(itemSelectionChanged()), this, SLOT(slot_tableWidgetPESPacketList_itemSelectionChanged()));
    connect(tableWidgetPESPacketList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slot_tableWidgetPESPacketList_customContextMenuRequested(QPoint)));

    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(slot_file_open()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(slot_exit()));
    connect(ui->actionAbout, SIGNAL(triggered(bool)), this, SLOT(slot_about()));
    connect(ui->actionStream, SIGNAL(triggered(bool)), this, SLOT(slot_stream_view()));
    connect(ui->actionFFmpeg_AVPacket, SIGNAL(triggered(bool)), this, SLOT(slot_FFmpeg_stream_packet_view()));
    connect(ui->actionTS_Packet, SIGNAL(triggered(bool)), this, SLOT(slot_ts_packet_view()));
    connect(ui->actionPES_Packet_P, SIGNAL(triggered(bool)), this, SLOT(slot_pes_packet_view()));
    connect(ui->actionH_264_NAL_N, SIGNAL(triggered(bool)), this, SLOT(slot_h264_nal_view()));
    connect(ui->actionPlay_Stream, SIGNAL(triggered(bool)), this, SLOT(slot_play_stream()));

    av_register_all();

    init_stream_list_tree();
    init_ffmpeg_packet_table();
    init_ts_packet_table();
    init_pes_packet_table();
    init_nal_table();
    init_info_table();

    status_label = new QLabel(ui->statusBar);
    ui->statusBar->insertWidget(0, status_label);
    status_label->setSizePolicy(status_label->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
    status_label->setMinimumWidth(800);
    // status_label->setMaximumWidth(800);
    parse_progress = new QProgressBar(ui->statusBar);
    ui->statusBar->insertWidget(1, parse_progress);

    parse_progress->setVisible(false);
    parse_progress->setRange(0, 100);

    connect(&thread, SIGNAL(update_parse_progress(int)), this, SLOT(slot_update_parse_progress(int)));
    connect(&thread, SIGNAL(parse_finished()), this, SLOT(slot_parse_finished()));

    progress_bar_dissmiss_timer_id = -1;

    showMaximized();

    connect(ui->actionEnable_Selected_Packets, SIGNAL(triggered(bool)), this, SLOT(slot_enable_selected_packets()));
    connect(ui->actionDisable_Selected_Packets, SIGNAL(triggered(bool)), this, SLOT(slot_disable_selected_packets()));
    ui->actionEnable_Selected_Packets->setEnabled(false);
    ui->actionDisable_Selected_Packets->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete ts_view_splitter;
    if (cur_data_buf) {
        free(cur_data_buf);
    }

    delete pes_view_splitter;
}

void MainWindow::slot_about() {

}

void MainWindow::slot_exit() {
    this->close();
}

void MainWindow::slot_file_open() {
    QString new_filename = QFileDialog::getOpenFileName(this, tr("Open stream file"), tr(""), tr(""));
    if (new_filename.isNull()) {
        return;
    }

    cur_filename = new_filename;

    this->setWindowTitle(tr("TS Stream Analyzer - ") + cur_filename);

    ui->treeWidgetStreamPacketList->clear();
    ui->tableWidgetFFmpegAVPacketList->clearContents();
    tableWidgetTSPacketList->clearContents();
    proto_tree_ts->clear();
    byte_view_ts->clear();
    tableWidgetPESPacketList->clearContents();
    proto_tree_pes->clear();
    byte_view_pes->clear();
    ui->tableWidgetH264NALList->clearContents();
    ui->tableWidgetFFmpegAVPacketList->setRowCount(0);
    tableWidgetTSPacketList->setRowCount(0);
    tableWidgetPESPacketList->setRowCount(0);
    ui->tableWidgetH264NALList->setRowCount(0);
    ui->tableWidgetInfo->setRowCount(0);

    status_label->setText(tr("Parsing file..."));

    thread.start_parse(this);
}

void MainWindow::init_stream_list_tree() {
    ui->treeWidgetStreamPacketList->setColumnCount(STREAM_TABLE_COLUMN_COUNT);
    QStringList col_headers;
    col_headers.clear();
    for (int i=0;i<STREAM_TABLE_COLUMN_COUNT;i++) {
        col_headers.append(stream_table_col_headers[i]);
    }
    ui->treeWidgetStreamPacketList->setHeaderLabels(col_headers);
    for (int i=0;i<STREAM_TABLE_COLUMN_COUNT;i++) {
        if (stream_table_col_widths[i] >= 0) {
            ui->treeWidgetStreamPacketList->setColumnWidth(i, stream_table_col_widths[i]);
        }
    }
    ui->treeWidgetStreamPacketList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->treeWidgetStreamPacketList->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::init_ffmpeg_packet_table() {
    ui->tableWidgetFFmpegAVPacketList->setColumnCount(FFMPEG_PACKET_TABLE_COLUMN_COUNT);
    QStringList col_headers;
    col_headers.clear();
    for (int i=0;i<FFMPEG_PACKET_TABLE_COLUMN_COUNT;i++) {
        col_headers.append(ffmpeg_packet_table_col_headers[i]);
    }
    ui->tableWidgetFFmpegAVPacketList->setHorizontalHeaderLabels(col_headers);
    for (int i=0;i<FFMPEG_PACKET_TABLE_COLUMN_COUNT;i++) {
        if (ffmpeg_packet_table_col_widths[i] >= 0) {
            ui->tableWidgetFFmpegAVPacketList->setColumnWidth(i, ffmpeg_packet_table_col_widths[i]);
        }
    }

    ui->tableWidgetFFmpegAVPacketList->verticalHeader()->setVisible(false);

    ui->tableWidgetFFmpegAVPacketList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableWidgetFFmpegAVPacketList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetFFmpegAVPacketList->setShowGrid(false);

    QHeaderView* header = ui->tableWidgetFFmpegAVPacketList->verticalHeader();
    if (header) {
        header->setSectionResizeMode(QHeaderView::Fixed);
        header->setDefaultSectionSize(20);
    }
    ui->tableWidgetFFmpegAVPacketList->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::init_ts_packet_table() {
    tableWidgetTSPacketList->setColumnCount(TS_PACKET_TABLE_COLUMN_COUNT);
    QStringList col_headers;
    col_headers.clear();
    for (int i=0;i<TS_PACKET_TABLE_COLUMN_COUNT;i++) {
        col_headers.append(ts_packet_table_col_headers[i]);
    }
    tableWidgetTSPacketList->setHorizontalHeaderLabels(col_headers);
    for (int i=0;i<TS_PACKET_TABLE_COLUMN_COUNT;i++) {
        if (ts_packet_table_col_widths[i] >= 0) {
            tableWidgetTSPacketList->setColumnWidth(i, ts_packet_table_col_widths[i]);
        }
    }

    tableWidgetTSPacketList->verticalHeader()->setVisible(false);

    tableWidgetTSPacketList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableWidgetTSPacketList->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidgetTSPacketList->setShowGrid(false);

    QHeaderView* header = tableWidgetTSPacketList->verticalHeader();
    if (header) {
        header->setSectionResizeMode(QHeaderView::Fixed);
        header->setDefaultSectionSize(20);
    }
    tableWidgetTSPacketList->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::init_pes_packet_table() {
    tableWidgetPESPacketList->setColumnCount(PES_PACKET_TABLE_COLUMN_COUNT);
    QStringList col_headers;
    col_headers.clear();
    for (int i=0;i<PES_PACKET_TABLE_COLUMN_COUNT;i++) {
        col_headers.append(pes_packet_table_col_headers[i]);
    }
    tableWidgetPESPacketList->setHorizontalHeaderLabels(col_headers);
    for (int i=0;i<PES_PACKET_TABLE_COLUMN_COUNT;i++) {
        if (pes_packet_table_col_widths[i] >= 0) {
            tableWidgetPESPacketList->setColumnWidth(i, pes_packet_table_col_widths[i]);
        }
    }

    tableWidgetPESPacketList->verticalHeader()->setVisible(false);

    tableWidgetPESPacketList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tableWidgetPESPacketList->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidgetPESPacketList->setShowGrid(false);

    QHeaderView* header = tableWidgetPESPacketList->verticalHeader();
    if (header) {
        header->setSectionResizeMode(QHeaderView::Fixed);
        header->setDefaultSectionSize(20);
    }
    tableWidgetPESPacketList->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::init_nal_table() {
    ui->tableWidgetH264NALList->setColumnCount(NAL_TABLE_COLUMN_COUNT);
    QStringList col_headers;
    col_headers.clear();
    for (int i=0;i<NAL_TABLE_COLUMN_COUNT;i++) {
        col_headers.append(nal_table_col_headers[i]);
    }
    ui->tableWidgetH264NALList->setHorizontalHeaderLabels(col_headers);
    for (int i=0;i<NAL_TABLE_COLUMN_COUNT;i++) {
        if (nal_table_col_widths[i] >= 0) {
            ui->tableWidgetH264NALList->setColumnWidth(i, nal_table_col_widths[i]);
        }
    }

    ui->tableWidgetH264NALList->verticalHeader()->setVisible(false);

    ui->tableWidgetH264NALList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableWidgetH264NALList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetH264NALList->setShowGrid(false);

    QHeaderView* header = ui->tableWidgetH264NALList->verticalHeader();
    if (header) {
        header->setSectionResizeMode(QHeaderView::Fixed);
        header->setDefaultSectionSize(20);
    }
    ui->tableWidgetH264NALList->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::slot_update_parse_progress(int progress) {
    parse_progress->setVisible(true);
    parse_progress->setValue(progress);

}

void MainWindow::slot_parse_finished() {
    QStringList item_content;
    parse_progress->setValue(100);

    ui->treeWidgetStreamPacketList->setEnabled(false);
    ui->tableWidgetFFmpegAVPacketList->setEnabled(false);
    tableWidgetTSPacketList->setEnabled(false);
    tableWidgetPESPacketList->setEnabled(false);
    ui->tableWidgetH264NALList->setEnabled(false);

    int unknown_ts_count = 0;
    int audio_ts_count = 0;
    int i_slice_ts_count = 0;
    int p_slice_ts_count = 0;
    int b_slice_ts_count = 0;
    int other_ts_count = 0;

    status_label->setText(tr("Parse OK! Now updating table content..."));
    QCoreApplication::processEvents();

    parse_progress->setValue(0);
    // display stream info
    for (int i=0;i<thread.tpc->ic->nb_streams;i++) {
        const AVCodecDescriptor* codec_desc = avcodec_descriptor_get(thread.tpc->ic->streams[i]->codec->codec_id);
        item_content.clear();
        item_content.append(codec_desc->name);
        ui->treeWidgetStreamPacketList->insertTopLevelItem(i, new QTreeWidgetItem(item_content));
    }

    // display stream packets info
    ui->tableWidgetFFmpegAVPacketList->setRowCount(thread.tpc->nb_stream_packets);
    for (int i=0;i<thread.tpc->nb_stream_packets;i++) {
        item_content.clear();
        item_content.append(tr(""));
        item_content.append(QString::number(i));
        item_content.append(QString::number(thread.tpc->stream_packets[i].packet->size));
        item_content.append(QString::number(thread.tpc->stream_packets[i].packet->pos));
        item_content.append(QString::number(thread.tpc->stream_packets[i].packet->pts));
        item_content.append(QString::number(thread.tpc->stream_packets[i].packet->dts));
        int start_ts_index = thread.tpc->stream_packets[i].start_ts_index;
        int end_ts_index = thread.tpc->stream_packets[i].end_ts_index;
        if (start_ts_index >= 0 && end_ts_index >= 0) {
            item_content.append(QString::number(end_ts_index - start_ts_index + 1));
        } else {
            item_content.append(tr(""));
        }

        if (start_ts_index >= 0) {
            item_content.append(QString::number(start_ts_index));
        } else {
            item_content.append(tr(""));
        }

        if (end_ts_index >= 0) {
            item_content.append(QString::number(end_ts_index));
        } else {
            item_content.append(tr(""));
        }

        QTreeWidgetItem* top_level_item = ui->treeWidgetStreamPacketList->topLevelItem(thread.tpc->stream_packets[i].packet->stream_index);

        top_level_item->addChild(new QTreeWidgetItem(item_content));
        top_level_item->child(top_level_item->childCount() - 1)->setData(0, Qt::UserRole, QVariant(i));

        for (int j=0;j<FFMPEG_PACKET_TABLE_COLUMN_COUNT;j++) {
            if (ui->tableWidgetFFmpegAVPacketList->item(i, j) == NULL) {
                ui->tableWidgetFFmpegAVPacketList->setItem(i, j, new QTableWidgetItem(tr("")));
                ui->tableWidgetFFmpegAVPacketList->item(i, j)->setFlags(ui->tableWidgetFFmpegAVPacketList->item(i, j)->flags() & ~Qt::ItemIsEditable);
            }
        }

        ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_INDEX)->setText(QString::number(i));
        ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_TYPE)->setText(ui->treeWidgetStreamPacketList->topLevelItem(thread.tpc->stream_packets[i].packet->stream_index)->text(STREAM_TABLE_COLUMN_STREAM));
        ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_LENGTH)->setText(QString::number(thread.tpc->stream_packets[i].packet->size));
        ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_OFFSET)->setText(QString::number(thread.tpc->stream_packets[i].packet->pos));
        ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_PTS)->setText(QString::number(thread.tpc->stream_packets[i].packet->pts));
        ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_DTS)->setText(QString::number(thread.tpc->stream_packets[i].packet->dts));

        if (start_ts_index >= 0 && end_ts_index >= 0) {
            ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_TS_COUNT)->setText(QString::number(end_ts_index - start_ts_index + 1));
        }

        if (start_ts_index >= 0) {
            ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_START_TS_INDEX)->setText(QString::number(start_ts_index));
        }

        if (end_ts_index >= 0) {
            ui->tableWidgetFFmpegAVPacketList->item(i, FFMPEG_PACKET_TABLE_COLUMN_END_TS_INDEX)->setText(QString::number(end_ts_index));
        }

        parse_progress->setValue(i * 25 / thread.tpc->nb_stream_packets);
        QCoreApplication::processEvents();
    }

    ui->treeWidgetStreamPacketList->scrollToTop();
    ui->tableWidgetFFmpegAVPacketList->scrollToTop();

    ts_packet_select.clear();
    ts_packet_total_len = 0;
    // display ts packet info
    tableWidgetTSPacketList->setRowCount(thread.tpc->nb_ts_packets);
    for (int i=0;i<thread.tpc->nb_ts_packets;i++) {
        ts_packet_select.append(true);
        if (tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_SELECT) == NULL) {
            tableWidgetTSPacketList->setItem(i, TS_PACKET_TABLE_COLUMN_SELECT, new QTableWidgetItem(tr("")));
            tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_SELECT)->setFlags(tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_SELECT)->flags() | Qt::ItemIsEditable);
        }
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Checked);
        for (int j=1;j<TS_PACKET_TABLE_COLUMN_COUNT;j++) {
            if (tableWidgetTSPacketList->item(i, j) == NULL) {
                tableWidgetTSPacketList->setItem(i, j, new QTableWidgetItem(tr("")));
                tableWidgetTSPacketList->item(i, j)->setFlags(tableWidgetTSPacketList->item(i, j)->flags() & ~Qt::ItemIsEditable);
            }
        }

        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_INDEX)->setText(QString::number(i));
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_OFFSET)->setText(QString::number(thread.tpc->ts_packets[i]->start_pos));
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_PID)->setText(QString::number(thread.tpc->ts_packets[i]->pid));
        QString str_type = "";
        if (thread.tpc->ts_packets[i]->pid == SDT_PID) {
            other_ts_count ++;
            str_type = "SDT";
        } else if (thread.tpc->ts_packets[i]->pid == PAT_PID) {
            other_ts_count ++;
            str_type = "PAT";
        } else if (thread.tpc->pmt_pid == thread.tpc->ts_packets[i]->pid) {
            other_ts_count ++;
            str_type = "PMT";
        } else if (thread.tpc->cur_pmt.audio_pid == thread.tpc->ts_packets[i]->pid) {
            audio_ts_count ++;
            str_type = "Audio";
        } else if (thread.tpc->cur_pmt.video_pid == thread.tpc->ts_packets[i]->pid) {
            str_type = "Video";
        } else {
            unknown_ts_count ++;
        }
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_TYPE)->setText(str_type);
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_CC)->setText(QString::number(thread.tpc->ts_packets[i]->cc_serial));
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_FRAME)->setForeground(QBrush(Qt::black));
        QString str_frame = "";
        if (thread.tpc->cur_pmt.video_pid == thread.tpc->ts_packets[i]->pid) {
            if (thread.tpc->ts_packets[i]->pes_packet) {
                pes_packet_info* pes_packet = thread.tpc->ts_packets[i]->pes_packet;
                bool have_i_slice = false;
                bool have_p_slice = false;
                bool have_b_slice = false;
                for (int j=0;j<pes_packet->nb_nals;j++) {
                    switch ( pes_packet->nals[j]->nal_unit_type ) {
                    case NAL_UNIT_TYPE_CODED_SLICE_IDR:
                        have_i_slice = true;  // fall through
                    case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
                    case NAL_UNIT_TYPE_CODED_SLICE_AUX:
                        if (pes_packet->nals[j]->parsed) {
                            slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                            if (sh == NULL) break;
                            switch (sh->slice_type) {
                            case 2:
                            case 4:
                            case 7:
                            case 9:
                                have_i_slice = true;
                                break;
                            case 0:
                            case 3:
                            case 5:
                            case 8:
                                have_p_slice = true;
                                break;
                            case 1:
                            case 6:
                                have_b_slice = true;
                                break;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }

                if (have_i_slice) {
                    str_frame = "I Slice";
                    tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_FRAME)->setForeground(QBrush(Qt::red));
                    i_slice_ts_count ++;
                } else if (have_p_slice) {
                    str_frame = "P Slice";
                    tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_FRAME)->setForeground(QBrush(Qt::blue));
                    p_slice_ts_count ++;
                } else if (have_b_slice) {
                    str_frame = "B Slice";
                    b_slice_ts_count ++;
                } else {
                    other_ts_count ++;
                }
            }
        }
        tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_FRAME)->setText(str_frame);

        if (thread.tpc->ts_packets[i]->pes_packet) {
            tableWidgetTSPacketList->item(i, TS_PACKET_TABLE_COLUMN_PES_INDEX)->setText(QString::number(thread.tpc->ts_packets[i]->pes_packet->index));
        }
        parse_progress->setValue(i * 25 / thread.tpc->nb_ts_packets + 25);
        QCoreApplication::processEvents();

        ts_packet_total_len += TS_PACKET_LEN;
    }

    tableWidgetTSPacketList->scrollToTop();

    pes_packet_total_len = 0;
    int nal_count = 0;
    QStringList gops;
    QString gop;
    // display pes packet info
    tableWidgetPESPacketList->setRowCount(thread.tpc->nb_pes_packets);
    for (int i=0;i<thread.tpc->nb_pes_packets;i++) {
        for (int j=0;j<PES_PACKET_TABLE_COLUMN_COUNT;j++) {
            if (tableWidgetPESPacketList->item(i, j) == NULL) {
                tableWidgetPESPacketList->setItem(i, j, new QTableWidgetItem(tr("")));
                tableWidgetPESPacketList->item(i, j)->setFlags(tableWidgetPESPacketList->item(i, j)->flags() & ~Qt::ItemIsEditable);
            }
        }

        pes_packet_info* pes_packet = thread.tpc->pes_packets[i];
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_INDEX)->setText(QString::number(i));
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_OFFSET)->setText(QString::number(pes_packet->ts_start_pos));
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_LENGTH)->setText(QString::number(pes_packet->len));
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_STREAM_ID)->setText(QString::number(pes_packet->stream_id, 16));
        QString str_stream_type = "";
        if (pes_packet->start_ts_index >= 0) {
            if (pes_packet->ts_pid == thread.tpc->cur_pmt.audio_pid) {
                str_stream_type = "Audio";
            } else if (pes_packet->ts_pid == thread.tpc->cur_pmt.video_pid) {
                str_stream_type = "Video";
            }
        }
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_STREAM_TYPE)->setText(str_stream_type);
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_TS_PID)->setText(QString::number(pes_packet->ts_pid));
        if (pes_packet->start_ts_index >= 0 && pes_packet->end_ts_index >= 0) {
            tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_TS_COUNT)->setText(QString::number(pes_packet->end_ts_index - pes_packet->start_ts_index + 1));
        }
        if (pes_packet->start_ts_index >= 0) {
            tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_START_TS_INDEX)->setText(QString::number(pes_packet->start_ts_index));
        }
        if (pes_packet->end_ts_index >= 0) {
            tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_END_TS_INDEX)->setText(QString::number(pes_packet->end_ts_index));
        }
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_PTS)->setText(QString::number(pes_packet->pts));
        tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_DTS)->setText(QString::number(pes_packet->dts));
        if (pes_packet->ts_pid == thread.tpc->cur_pmt.video_pid) {
            QBrush text_brush = QBrush(Qt::black);
            tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_NAL_COUNT)->setText(QString::number(pes_packet->nb_nals));
            nal_count += pes_packet->nb_nals;
            QString str_slices;
            bool have_i_slice = false;
            bool have_p_slice = false;
            bool have_b_slice = false;
            for (int j=0;j<pes_packet->nb_nals;j++) {
                switch (pes_packet->nals[j]->nal_unit_type) {
                case NAL_UNIT_TYPE_CODED_SLICE_IDR:
                    str_slices += "IDR";
                    if (pes_packet->nals[j]->parsed) {
                        slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                        str_slices += tr("(") + get_slice_type(sh) + tr(")");
                    }
                    text_brush = QBrush(Qt::red);
                    have_i_slice = true;
                    break;
                case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
                    str_slices += "non-IDR";
                    if (pes_packet->nals[j]->parsed) {
                        slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                        str_slices += tr("(") + get_slice_type(sh) + tr(")");
                    }

                    if (str_slices.contains('P')) {
                        have_p_slice = true;
                        text_brush = QBrush(Qt::blue);
                    } else if (str_slices.contains('B')){
                        have_b_slice = true;
                    }
                    break;
                case NAL_UNIT_TYPE_CODED_SLICE_AUX:
                    str_slices += "AUX";
                    if (pes_packet->nals[j]->parsed) {
                        slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                        str_slices += tr("(") + get_slice_type(sh) + tr(")");
                    }
                    break;
                case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A:
                    str_slices += "DPA";
                    break;
                case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B:
                    str_slices += "DPB";
                    break;
                case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C:
                    str_slices += "DPC";
                    break;
                case NAL_UNIT_TYPE_SEI:
                    str_slices += "SEI";
                    break;
                case NAL_UNIT_TYPE_SPS:
                    str_slices += "SPS";
                    break;
                case NAL_UNIT_TYPE_PPS:
                    str_slices += "PPS";
                    break;
                case NAL_UNIT_TYPE_AUD:
                    str_slices += "AUD";
                    break;
                case NAL_UNIT_TYPE_END_OF_SEQUENCE:
                    str_slices += "EOS1";
                    break;
                case NAL_UNIT_TYPE_END_OF_STREAM:
                    str_slices += "EOS";
                    break;
                case NAL_UNIT_TYPE_FILLER:
                    str_slices += "Filler";
                    break;
                case NAL_UNIT_TYPE_SPS_EXT:
                    str_slices += "SPSExt";
                    break;
                case NAL_UNIT_TYPE_UNSPECIFIED:
                    str_slices += "Unspec";
                    break;
                default:
                    str_slices += "Rsrv";
                    break;
                }

                if (j < pes_packet->nb_nals - 1 && str_slices.length() > 0) {
                    str_slices += ",";
                }
            }

            if (have_i_slice) {
                if (gop.length() > 0) {
                    gops.append(gop);
                }
                gop = "I";
            } else if (have_p_slice) {
                if (gop.length() > 0) {
                    gop += "P";
                }
            } else if (have_b_slice) {
                if (gop.length() > 0) {
                    gop += "B";
                }
            }

            tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_SLICE_TYPES)->setText(str_slices);
            tableWidgetPESPacketList->item(i, PES_PACKET_TABLE_COLUMN_SLICE_TYPES)->setForeground(text_brush);
        }
        parse_progress->setValue(i * 25 / thread.tpc->nb_pes_packets + 50);
        QCoreApplication::processEvents();

        pes_packet_total_len += pes_packet->len;
    }

    tableWidgetPESPacketList->scrollToTop();

    ui->tableWidgetH264NALList->setRowCount(nal_count);
    int nal_index = 0;
    for (int i=0;i<thread.tpc->nb_pes_packets;i++) {
        pes_packet_info* pes_packet = thread.tpc->pes_packets[i];
        for (int j=0;j<pes_packet->nb_nals;j++,nal_index++) {
            for (int k=0;k<NAL_TABLE_COLUMN_COUNT;k++) {
                if (ui->tableWidgetH264NALList->item(nal_index, k) == NULL) {
                    ui->tableWidgetH264NALList->setItem(nal_index, k, new QTableWidgetItem(tr("")));
                    ui->tableWidgetH264NALList->item(nal_index, k)->setFlags(ui->tableWidgetH264NALList->item(nal_index, k)->flags() & ~Qt::ItemIsEditable);
                }
            }

            ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_INDEX)->setText(QString::number(nal_index));
            ui->tableWidgetH264NALList->item(nal_index, 0)->setData(Qt::UserRole, QVariant(i));
            QString str_type = "";
            QString str_slice_type = "";
            switch (pes_packet->nals[j]->nal_unit_type) {
            case NAL_UNIT_TYPE_CODED_SLICE_IDR:
                str_type = "Coded slice of an IDR picture";
                if (pes_packet->nals[j]->parsed) {
                    slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                    str_slice_type = get_slice_type(sh);
                    ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_SLICE_TYPE)->setText(str_slice_type);
                    ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_SLICE_TYPE)->setForeground(QBrush(Qt::red));
                    ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_TYPE)->setForeground(QBrush(Qt::red));
                }
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_NON_IDR:
                str_type = "Coded slice of a non-IDR picture";
                if (pes_packet->nals[j]->parsed) {
                    slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                    str_slice_type = get_slice_type(sh);
                    ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_SLICE_TYPE)->setText(str_slice_type);
                    if (str_slice_type.contains('P')) {
                        ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_SLICE_TYPE)->setForeground(QBrush(Qt::blue));
                        ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_TYPE)->setForeground(QBrush(Qt::blue));
                    }
                }
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_AUX:
                str_type = "Coded slice of an auxiliary coded picture without partitioning";
                if (pes_packet->nals[j]->parsed) {
                    slice_header_t* sh = (slice_header_t*)pes_packet->nals[j]->parsed;
                    str_slice_type = get_slice_type(sh);
                    ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_SLICE_TYPE)->setText(str_slice_type);
                }
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A:
                str_type = "Coded slice data partition A";
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B:
                str_type = "Coded slice data partition B";
                break;
            case NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C:
                str_type = "Coded slice data partition C";
                break;
            case NAL_UNIT_TYPE_SEI:
                str_type = "Supplemental enhancement information (SEI)";
                break;
            case NAL_UNIT_TYPE_SPS:
                str_type = "Sequence parameter set";
                break;
            case NAL_UNIT_TYPE_PPS:
                str_type = "Picture parameter set";
                break;
            case NAL_UNIT_TYPE_AUD:
                str_type = "Access unit delimiter";
                break;
            case NAL_UNIT_TYPE_END_OF_SEQUENCE:
                str_type = "End of sequence";
                break;
            case NAL_UNIT_TYPE_END_OF_STREAM:
                str_type = "End of stream";
                break;
            case NAL_UNIT_TYPE_FILLER:
                str_type = "Filler data";
                break;
            case NAL_UNIT_TYPE_SPS_EXT:
                str_type = "Sequence parameter set extension";
                break;
            case NAL_UNIT_TYPE_UNSPECIFIED:
                str_type = "Unspecified";
                break;
            default:
                str_type = "Reserved";
                break;
            }

            ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_TYPE)->setText(str_type);
            ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_LENGTH)->setText(QString::number(pes_packet->nals[j]->len));
            ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_PES_PACKET_INDEX)->setText(QString::number(i));
            if (pes_packet->start_ts_index >= 0 && pes_packet->end_ts_index >= 0) {
                ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_TS_COUNT)->setText(QString::number(pes_packet->end_ts_index - pes_packet->start_ts_index + 1));
            }
            if (pes_packet->start_ts_index >= 0) {
                ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_START_TS_INDEX)->setText(QString::number(pes_packet->start_ts_index));
            }
            if (pes_packet->end_ts_index >= 0) {
                ui->tableWidgetH264NALList->item(nal_index, NAL_TABLE_COLUMN_END_TS_INDEX)->setText(QString::number(pes_packet->end_ts_index));
            }
            parse_progress->setValue(nal_index * 25 / nal_count + 75);
            QCoreApplication::processEvents();
        }
    }

    ui->tableWidgetH264NALList->scrollToTop();

    cur_file_handle.setFileName(cur_filename);
    cur_file_handle.open(QIODevice::ReadOnly);

    // update info table
    insert_info_table(tr("Media info:"));
    insert_info_table(tr("File path:"), cur_filename);
    insert_info_table(tr("File size:"), tr("%1 bytes").arg(cur_file_handle.size()));
    insert_info_table(tr("Duration:"), tr("%1 sec").arg((double)thread.tpc->ic->duration / AV_TIME_BASE, -1, 'f', 2, QChar('0')));
    insert_info_table(tr("Overall bit rate:"), tr("%1 Kbps").arg((double)thread.tpc->ic->bit_rate / 1000, -1, 'f', 2, QChar('0')));
    for (int i=0;i<thread.tpc->ic->nb_streams;i++) {
        insert_info_table(tr("Stream %1:").arg(i));
        const AVCodecDescriptor* codec_desc = avcodec_descriptor_get(thread.tpc->ic->streams[i]->codec->codec_id);
        insert_info_table(tr("Type:"), tr("%1").arg(av_get_media_type_string(codec_desc->type)));
        insert_info_table(tr("Codec type:"), tr("%1").arg(codec_desc->long_name));
        insert_info_table(tr("Duration:"), tr("%1 sec").arg((double)thread.tpc->ic->streams[i]->duration * av_q2d(thread.tpc->ic->streams[i]->time_base), -1, 'f', 2, QChar('0')));
        if (codec_desc->type == AVMEDIA_TYPE_VIDEO) {
            insert_info_table(tr("Frame rate:"), tr("%1").arg(av_q2d(thread.tpc->ic->streams[i]->avg_frame_rate)));
            insert_info_table(tr("Resolution:"), tr("%1 x %2").arg(thread.tpc->ic->streams[i]->codecpar->width).arg(thread.tpc->ic->streams[i]->codecpar->height));
        } else if (codec_desc->type == AVMEDIA_TYPE_AUDIO) {
            insert_info_table(tr("Bit rate:"), tr("%1 Kbps").arg((double)thread.tpc->ic->streams[i]->codecpar->bit_rate / 1000, -1, 'f', 2, QChar('0')));
            insert_info_table(tr("Sample rate:"), tr("%1 Hz").arg(thread.tpc->ic->streams[i]->codecpar->sample_rate));
        }
    }

    QString total_ts_count = tr("%1").arg(thread.tpc->nb_ts_packets);
    int total_ts_count_len = total_ts_count.length();
    QString file_len_str = tr("%1").arg(cur_file_handle.size());
    int file_len_str_len = file_len_str.length();
    insert_info_table(tr("TS packet statics:"));
    insert_info_table(tr("Audio TS Count:"), tr("%1 (%2%, %3 bytes)").arg(audio_ts_count, total_ts_count_len)
                      .arg((double)audio_ts_count * 100.0 / thread.tpc->nb_ts_packets, 5, 'f', 2)
                      .arg((uint64_t)audio_ts_count * TS_PACKET_LEN, file_len_str_len));
    insert_info_table(tr("I slice TS Count:"), tr("%1 (%2%, %3 bytes)").arg(i_slice_ts_count, total_ts_count_len)
                      .arg((double)i_slice_ts_count * 100.0 / thread.tpc->nb_ts_packets, 5, 'f', 2)
                      .arg((uint64_t)i_slice_ts_count * TS_PACKET_LEN, file_len_str_len));
    insert_info_table(tr("P slice TS Count:"), tr("%1 (%2%, %3 bytes)").arg(p_slice_ts_count, total_ts_count_len)
                      .arg((double)p_slice_ts_count * 100.0 / thread.tpc->nb_ts_packets, 5, 'f', 2)
                      .arg((uint64_t)p_slice_ts_count * TS_PACKET_LEN, file_len_str_len));
    insert_info_table(tr("B slice TS Count:"), tr("%1 (%2%, %3 bytes)").arg(b_slice_ts_count, total_ts_count_len)
                      .arg((double)b_slice_ts_count * 100.0 / thread.tpc->nb_ts_packets, 5, 'f', 2)
                      .arg((uint64_t)b_slice_ts_count * TS_PACKET_LEN, file_len_str_len));
    insert_info_table(tr("Other TS Count:"), tr("%1 (%2%, %4 bytes)").arg(other_ts_count, total_ts_count_len)
                      .arg((double)other_ts_count * 100.0 / thread.tpc->nb_ts_packets, 5, 'f', 2)
                      .arg((uint64_t)other_ts_count * TS_PACKET_LEN, file_len_str_len));
    if (unknown_ts_count > 0) {
        insert_info_table(tr("Unknown TS Count:"), tr("%1 (%2%, %3 bytes)").arg(unknown_ts_count, total_ts_count_len)
                          .arg((double)unknown_ts_count * 100.0 / thread.tpc->nb_ts_packets, 5, 'f', 2)
                          .arg((uint64_t)unknown_ts_count * TS_PACKET_LEN, file_len_str_len));
    }
    insert_info_table(tr("Total:"), total_ts_count);

    insert_info_table(tr("PES Usage Rate:"));
    insert_info_table(tr("PES total length:"), tr("%1 bytes").arg(pes_packet_total_len));
    insert_info_table(tr("TS total length:"), tr("%1 bytes").arg(ts_packet_total_len));
    insert_info_table(tr("PES usage rate:"), tr("%1%").arg((double)pes_packet_total_len * 100.0 / (double)ts_packet_total_len, 0, 'f', 1));

    insert_info_table(tr("GOP (count %1):").arg(gops.count()));
    for (int i=0;i<gops.count();i++) {
        insert_info_table(tr("%1").arg(i), gops[i]);
    }

    ui->tableWidgetInfo->resizeColumnsToContents();

    status_label->setText(tr("Update table content OK!"));
    parse_progress->setValue(100);
    progress_bar_dissmiss_timer_id = startTimer(2000);

    ui->treeWidgetStreamPacketList->setEnabled(true);
    ui->tableWidgetFFmpegAVPacketList->setEnabled(true);
    tableWidgetTSPacketList->setEnabled(true);
    tableWidgetPESPacketList->setEnabled(true);
    ui->tableWidgetH264NALList->setEnabled(true);

}

void MainWindow::timerEvent(QTimerEvent *e) {
    if (e->timerId() == progress_bar_dissmiss_timer_id) {
        parse_progress->setVisible(false);
        killTimer(progress_bar_dissmiss_timer_id);
        status_label->setText(tr(""));
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    ui->tabWidget->setCurrentIndex(0);
    QMainWindow::showEvent(event);
}

#define TREE_BYTE_VIEW_DIVIDER 5

void MainWindow::resizeEvent(QResizeEvent *) {
    QList<int> sizes;
    sizes = ts_view_splitter->sizes();
    sizes[0] = ts_view_splitter->height();
    if (sizes.length() > 1) {
        sizes[ts_view_splitter->sizes().length() - 1] = ts_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
        sizes[0] -= ts_view_splitter->handleWidth() + ts_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
    }

    if (sizes.length() > 2) {
        sizes[ts_view_splitter->sizes().length() - 2] = ts_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
        sizes[0] -= ts_view_splitter->handleWidth() + ts_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
    }

    ts_view_splitter->setSizes(sizes);

    sizes = pes_view_splitter->sizes();
    sizes[0] = pes_view_splitter->height();
    if (sizes.length() > 1) {
        sizes[pes_view_splitter->sizes().length() - 1] = pes_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
        sizes[0] -= pes_view_splitter->handleWidth() + pes_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
    }

    if (sizes.length() > 2) {
        sizes[pes_view_splitter->sizes().length() - 2] = pes_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
        sizes[0] -= pes_view_splitter->handleWidth() + pes_view_splitter->height() / TREE_BYTE_VIEW_DIVIDER;
    }

    pes_view_splitter->setSizes(sizes);
}

void MainWindow::slot_stream_view() {
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::slot_FFmpeg_stream_packet_view() {
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::slot_ts_packet_view() {
    ui->tabWidget->setCurrentIndex(2);
}

void MainWindow::slot_pes_packet_view() {
    ui->tabWidget->setCurrentIndex(3);
}

void MainWindow::slot_h264_nal_view() {
    ui->tabWidget->setCurrentIndex(4);
}

void MainWindow::slot_play_stream() {
    PlayDialog* dlg = new PlayDialog(thread.tpc, ts_packet_select, cur_filename, this);
    QEventLoop loop;
    connect(dlg, SIGNAL(finished(int)), &loop, SLOT(quit()));
    ui->actionPlay_Stream->setEnabled(false);
    dlg->show();
    loop.exec();
    ui->actionPlay_Stream->setEnabled(true);
    return;
}

void MainWindow::slot_tableWidgetTSPacketList_cellChanged(int row, int column)
{
    if (column != TS_PACKET_TABLE_COLUMN_SELECT) {
        return;
    }

    if (row >= ts_packet_select.count()) {
        return;
    }

    ts_packet_select[row] = tableWidgetTSPacketList->item(row, column)->checkState() == Qt::Checked;
}

void MainWindow::slot_tableWidgetTSPacketList_itemSelectionChanged() {
    if (tableWidgetTSPacketList->selectedItems().count() > 0) {
        ui->actionEnable_Selected_Packets->setEnabled(true);
        ui->actionDisable_Selected_Packets->setEnabled(true);

        if (tableWidgetTSPacketList->selectedItems().count() == TS_PACKET_TABLE_COLUMN_COUNT) {
            int row = tableWidgetTSPacketList->selectedItems()[0]->row();
            ts_packet_info* packet = thread.tpc->ts_packets[row];
            if (cur_file_handle.isOpen()) {
                cur_file_handle.seek(packet->start_pos);
                if (cur_data_buf) {
                    free(cur_data_buf);
                }
                cur_data_buf = (uint8_t*)malloc(packet->len);
                cur_file_handle.read((char*)cur_data_buf, packet->len);
            }
            proto_tree_ts->fillProtocolTree(packet, cur_data_buf);

            byte_view_ts->set_buf(cur_data_buf, packet->len);
            byte_view_ts->protoItemSelected(0, 0, 0, 0);
            byte_view_ts->viewport()->update();
        }
    } else {
        ui->actionEnable_Selected_Packets->setEnabled(false);
        ui->actionDisable_Selected_Packets->setEnabled(false);
    }
}

void MainWindow::on_treeWidgetStreamPacketList_itemSelectionChanged() {
    if (ui->treeWidgetStreamPacketList->selectedItems().count() > 0) {
        ui->actionEnable_Selected_Packets->setEnabled(true);
        ui->actionDisable_Selected_Packets->setEnabled(true);
    } else {
        ui->actionEnable_Selected_Packets->setEnabled(false);
        ui->actionDisable_Selected_Packets->setEnabled(false);
    }
}

void MainWindow::on_tableWidgetFFmpegAVPacketList_itemSelectionChanged() {
    if (ui->tableWidgetFFmpegAVPacketList->selectedItems().count() > 0) {
        ui->actionEnable_Selected_Packets->setEnabled(true);
        ui->actionDisable_Selected_Packets->setEnabled(true);
    } else {
        ui->actionEnable_Selected_Packets->setEnabled(false);
        ui->actionDisable_Selected_Packets->setEnabled(false);
    }
}

void MainWindow::slot_tableWidgetPESPacketList_itemSelectionChanged() {
    if (tableWidgetPESPacketList->selectedItems().count() > 0) {
        ui->actionEnable_Selected_Packets->setEnabled(true);
        ui->actionDisable_Selected_Packets->setEnabled(true);

        if (tableWidgetPESPacketList->selectedItems().count() == PES_PACKET_TABLE_COLUMN_COUNT) {
            int row = tableWidgetPESPacketList->selectedItems()[0]->row();
            pes_packet_info* packet = thread.tpc->pes_packets[row];
            proto_tree_pes->fillProtocolTree(packet, packet->data);

            byte_view_pes->set_buf(packet->data, packet->len);
            byte_view_pes->protoItemSelected(0, 0, 0, 0);
            byte_view_pes->viewport()->update();
        }
    } else {
        ui->actionEnable_Selected_Packets->setEnabled(false);
        ui->actionDisable_Selected_Packets->setEnabled(false);
    }
}

void MainWindow::on_tableWidgetH264NALList_itemSelectionChanged() {
    if (ui->tableWidgetH264NALList->selectedItems().count() > 0) {
        ui->actionEnable_Selected_Packets->setEnabled(true);
        ui->actionDisable_Selected_Packets->setEnabled(true);
    } else {
        ui->actionEnable_Selected_Packets->setEnabled(false);
        ui->actionDisable_Selected_Packets->setEnabled(false);
    }
}

void MainWindow::slot_enable_selected_packets() {
    switch (ui->tabWidget->currentIndex()) {
    case 0:
        foreach (QTreeWidgetItem* item, ui->treeWidgetStreamPacketList->selectedItems()) {
            int stream_index = item->data(0, 0).toInt();
            if (stream_index < thread.tpc->nb_stream_packets) {
                for (int ts_index = thread.tpc->stream_packets[stream_index].start_ts_index;
                     ts_index <= thread.tpc->stream_packets[stream_index].end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Checked);
                    }
                }
            }
        }

        break;
    case 1:
        foreach (QTableWidgetItem* item, ui->tableWidgetFFmpegAVPacketList->selectedItems()) {
            int stream_index = item->row();
            if (stream_index < thread.tpc->nb_stream_packets) {
                for (int ts_index = thread.tpc->stream_packets[stream_index].start_ts_index;
                     ts_index <= thread.tpc->stream_packets[stream_index].end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Checked);
                    }
                }
            }
        }

        break;
    case 2:
        foreach (QTableWidgetItem* item, tableWidgetTSPacketList->selectedItems()) {
            int ts_index = item->row();
            if (ts_index < ts_packet_select.count()) {
                tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Checked);
            }
        }

        break;
    case 3:
        foreach (QTableWidgetItem* item, tableWidgetPESPacketList->selectedItems()) {
            int pes_index = item->row();
            if (pes_index < thread.tpc->nb_pes_packets) {
                for (int ts_index = thread.tpc->pes_packets[pes_index]->start_ts_index;
                     ts_index <= thread.tpc->pes_packets[pes_index]->end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Checked);
                    }
                }
            }
        }

        break;
    case 4:
        foreach (QTableWidgetItem* item, ui->tableWidgetH264NALList->selectedItems()) {
            int pes_index = item->data(0).toInt();
            if (pes_index < thread.tpc->nb_pes_packets) {
                for (int ts_index = thread.tpc->pes_packets[pes_index]->start_ts_index;
                     ts_index <= thread.tpc->pes_packets[pes_index]->end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Checked);
                    }
                }
            }
        }

        break;
    default:
        break;
    }
}

void MainWindow::slot_disable_selected_packets() {
    switch (ui->tabWidget->currentIndex()) {
    case 0:
        foreach (QTreeWidgetItem* item, ui->treeWidgetStreamPacketList->selectedItems()) {
            int stream_index = item->data(0, 0).toInt();
            if (stream_index < thread.tpc->nb_stream_packets) {
                for (int ts_index = thread.tpc->stream_packets[stream_index].start_ts_index;
                     ts_index <= thread.tpc->stream_packets[stream_index].end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Unchecked);
                    }
                }
            }
        }

        break;
    case 1:
        foreach (QTableWidgetItem* item, ui->tableWidgetFFmpegAVPacketList->selectedItems()) {
            int stream_index = item->row();
            if (stream_index < thread.tpc->nb_stream_packets) {
                for (int ts_index = thread.tpc->stream_packets[stream_index].start_ts_index;
                     ts_index <= thread.tpc->stream_packets[stream_index].end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Unchecked);
                    }
                }
            }
        }

        break;
    case 2:
        foreach (QTableWidgetItem* item, tableWidgetTSPacketList->selectedItems()) {
            tableWidgetTSPacketList->item(item->row(), TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Unchecked);
        }

        break;
    case 3:
        foreach (QTableWidgetItem* item, tableWidgetPESPacketList->selectedItems()) {
            int pes_index = item->row();
            if (pes_index < thread.tpc->nb_pes_packets) {
                for (int ts_index = thread.tpc->pes_packets[pes_index]->start_ts_index;
                     ts_index <= thread.tpc->pes_packets[pes_index]->end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Unchecked);
                    }
                }
            }
        }

        break;
    case 4:
        foreach (QTableWidgetItem* item, ui->tableWidgetH264NALList->selectedItems()) {
            int pes_index = item->data(0).toInt();
            if (pes_index < thread.tpc->nb_pes_packets) {
                for (int ts_index = thread.tpc->pes_packets[pes_index]->start_ts_index;
                     ts_index <= thread.tpc->pes_packets[pes_index]->end_ts_index;
                     ts_index ++) {
                    if (ts_index < ts_packet_select.count()) {
                        tableWidgetTSPacketList->item(ts_index, TS_PACKET_TABLE_COLUMN_SELECT)->setCheckState(Qt::Unchecked);
                    }
                }
            }
        }

        break;
    default:
        break;
    }
}

void MainWindow::on_tabWidget_currentChanged(int index) {
    switch (index) {
    case 0:
        if (ui->treeWidgetStreamPacketList->selectedItems().count() > 0) {
            ui->actionEnable_Selected_Packets->setEnabled(true);
            ui->actionDisable_Selected_Packets->setEnabled(true);
        } else {
            ui->actionEnable_Selected_Packets->setEnabled(false);
            ui->actionDisable_Selected_Packets->setEnabled(false);
        }
        break;
    case 1:
        if (ui->tableWidgetFFmpegAVPacketList->selectedItems().count() > 0) {
            ui->actionEnable_Selected_Packets->setEnabled(true);
            ui->actionDisable_Selected_Packets->setEnabled(true);
        } else {
            ui->actionEnable_Selected_Packets->setEnabled(false);
            ui->actionDisable_Selected_Packets->setEnabled(false);
        }
        break;
    case 2:
        if (tableWidgetTSPacketList->selectedItems().count() > 0) {
            ui->actionEnable_Selected_Packets->setEnabled(true);
            ui->actionDisable_Selected_Packets->setEnabled(true);
        } else {
            ui->actionEnable_Selected_Packets->setEnabled(false);
            ui->actionDisable_Selected_Packets->setEnabled(false);
        }
        break;
    case 3:
        if (tableWidgetPESPacketList->selectedItems().count() > 0) {
            ui->actionEnable_Selected_Packets->setEnabled(true);
            ui->actionDisable_Selected_Packets->setEnabled(true);
        } else {
            ui->actionEnable_Selected_Packets->setEnabled(false);
            ui->actionDisable_Selected_Packets->setEnabled(false);
        }
        break;
    case 4:
        if (ui->tableWidgetH264NALList->selectedItems().count() > 0) {
            ui->actionEnable_Selected_Packets->setEnabled(true);
            ui->actionDisable_Selected_Packets->setEnabled(true);
        } else {
            ui->actionEnable_Selected_Packets->setEnabled(false);
            ui->actionDisable_Selected_Packets->setEnabled(false);
        }
        break;
    default:
        break;
    }

    resizeEvent(NULL);
}

void MainWindow::on_treeWidgetStreamPacketList_customContextMenuRequested(const QPoint &pos) {
    ui->menuPacket_K->popup(ui->treeWidgetStreamPacketList->mapToGlobal(pos));
}

void MainWindow::on_tableWidgetFFmpegAVPacketList_customContextMenuRequested(const QPoint &pos) {
    ui->menuPacket_K->popup(ui->tableWidgetFFmpegAVPacketList->mapToGlobal(pos));
}

void MainWindow::slot_tableWidgetTSPacketList_customContextMenuRequested(const QPoint &pos) {
    ui->menuPacket_K->popup(tableWidgetTSPacketList->mapToGlobal(pos));
}

void MainWindow::slot_tableWidgetPESPacketList_customContextMenuRequested(const QPoint &pos) {
    ui->menuPacket_K->popup(tableWidgetPESPacketList->mapToGlobal(pos));
}

void MainWindow::on_tableWidgetH264NALList_customContextMenuRequested(const QPoint &pos) {
    ui->menuPacket_K->popup(ui->tableWidgetH264NALList->mapToGlobal(pos));
}

void MainWindow::init_info_table() {
    ui->tableWidgetInfo->clearContents();
    ui->tableWidgetInfo->setColumnCount(2);
    ui->tableWidgetInfo->horizontalHeader()->setVisible(false);
    ui->tableWidgetInfo->verticalHeader()->setVisible(false);
    ui->tableWidgetInfo->setShowGrid(false);
    QHeaderView* header = ui->tableWidgetInfo->verticalHeader();
    if (header) {
        header->setSectionResizeMode(QHeaderView::Fixed);
        header->setDefaultSectionSize(20);
    }
}

void MainWindow::insert_info_table(QString section) {
    int row = ui->tableWidgetInfo->rowCount();
    ui->tableWidgetInfo->insertRow(row);
    ui->tableWidgetInfo->setItem(row, 0, new QTableWidgetItem(section));
    ui->tableWidgetInfo->item(row, 0)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->tableWidgetInfo->item(row, 0)->setForeground(QBrush(QColor(Qt::blue)));
    ui->tableWidgetInfo->item(row, 0)->setFont(QFont(QApplication::font().family(), -1, QFont::Bold));
}

void MainWindow::insert_info_table(QString title, QString info) {
    int row = ui->tableWidgetInfo->rowCount();
    ui->tableWidgetInfo->insertRow(row);
    ui->tableWidgetInfo->setItem(row, 0, new QTableWidgetItem(title));
    ui->tableWidgetInfo->item(row, 0)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->tableWidgetInfo->setItem(row, 1, new QTableWidgetItem(info));
}
