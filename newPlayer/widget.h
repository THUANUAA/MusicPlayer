#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QStringList>
#include <QDir>
#include <QListWidgetItem>
#include <QStringList>
#include <QTimer>
#include <QTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

// 自定义结构体来存储歌词的时间戳和文本
struct LyricEntry {
    QTime timestamp;
    QString text;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_btn_Res_clicked();
    void on_btn_model_clicked();
    void on_btn_LeftMove_clicked();
    void on_btn_Player_clicked();
    void on_btn_RightMove_clicked();
    void on_btn_Volume_clicked();
    void on_playerVolumeSlider_valueChanged(int value);
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item); // 添加这一行
    void loadPlaylist(const QString &folderPath);
    void loadLyrics(const QString &lyricPath);
    QStringList scanMusicFiles(const QString &folderPath);
    void syncLyrics();
    void displayLyrics();
    void updateLyrics(QProcess::ProcessState newState);
    void updateCurrentPosition(); // 添加这一行
    void highlightCurrentSong();
    void onError(QProcess::ProcessError error);
    void onStarted();
    void onReadyReadStandardOutput();
    void onFinished(int exitCode);
    void on_playCourseSlider_sliderReleased();
    void on_playCourseSlider_sliderPressed();
    void on_playCourseSlider_sliderMoved(int position);
    void updatePlayerUI();
    void updateSongInfo();
private:
    void loadPlaylist();
    void playSong();

    Ui::Widget *ui;
    QProcess *mplayer;
    QStringList playlist;
    QString currentSong;
    QTimer playbackTimer; // 声明 playbackTimer
    int currentIndex;
    bool isPaused;
    QStringList lyricPaths;
    int currentPosition; // 存储当前播放的位置（单位：秒）
    int totalDuration; // 存储当前歌曲的总时长（单位：秒）
    QTimer lyricTimer; // 声明 lyricTimer 对象
    QList<LyricEntry> lyricEntries; // 声明lyricEntries
    QMap<QString, QString> lyricFilePaths; // 存储歌曲文件名和对应的歌词文件路径
private:
    enum PlayMode { Sequential, Random };
    PlayMode playMode;
};

#endif // WIDGET_H
