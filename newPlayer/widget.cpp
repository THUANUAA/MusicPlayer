#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),  // 初始化UI界面
    mplayer(new QProcess(this)),  // 初始化用于媒体播放的QProcess
    currentIndex(0),  // 初始化播放列表中当前的索引
    isPaused(false),  // 初始暂停状态为false
    currentPosition(0),  // 初始化当前播放位置
    totalDuration(0),  // 初始化媒体总时长
    playMode(Sequential)  // 设置初始播放模式为顺序播放
{
    ui->setupUi(this);  // 设置用户界面

    playbackTimer.setInterval(100);  // 设置播放定时器的间隔
    connect(&playbackTimer, &QTimer::timeout, this, &Widget::updateCurrentPosition);  // 将定时器连接到更新当前位置的槽函数

    // 将QProcess的各个信号连接到Widget类中对应的槽函数
    connect(mplayer, &QProcess::started, this, &Widget::onStarted);
    connect(mplayer, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred), this, &Widget::onError);
    connect(mplayer, &QProcess::readyReadStandardOutput, this, &Widget::onReadyReadStandardOutput);
    connect(mplayer, QOverload<int>::of(&QProcess::finished), this, &Widget::onFinished);
    connect(ui->playCourseSlider, &QSlider::sliderReleased, this, &Widget::on_playCourseSlider_sliderReleased);
    connect(ui->playCourseSlider, &QSlider::sliderMoved, this, &Widget::on_playCourseSlider_sliderMoved);
    connect(ui->playCourseSlider, &QSlider::sliderPressed, this, &Widget::on_playCourseSlider_sliderPressed);

    playbackTimer.start();  // 开启播放定时器

    loadPlaylist();  // 加载播放列表
    ui->playerVolumeSlider->setVisible(false);  // 将播放器音量滑块初始设为不可见
    ui->btn_Player->setIcon(QIcon(":/ICON/start.png"));  // 设置播放按钮的图标
    lyricTimer.setInterval(1000);  // 设置歌词定时器的间隔
    connect(&lyricTimer, &QTimer::timeout, this, &Widget::syncLyrics);  // 将定时器连接到同步歌词的槽函数
    connect(ui->playerVolumeSlider, &QSlider::valueChanged, this, &Widget::on_playerVolumeSlider_valueChanged);  // 将音量滑块数值变化连接到相应的槽函数
}


Widget::~Widget()
{
    delete ui;
}

void Widget::playSong()
{
    // 停止当前正在播放的歌曲
    if (mplayer->state() == QProcess::Running) {
        mplayer->kill();
        mplayer->waitForFinished();
    }

    // 清除当前歌词
    lyricEntries.clear();
    ui->lyricWidget->clear();
    ui->playCourseSlider->setValue(0);
    ui->labCurTime->setText("00:00");
    ui->labAllTime->setText("00:00");

    // 检查 currentIndex 是否有效
    if (currentIndex < 0 || currentIndex >= playlist.size()) {
        return;
    }

    if (playMode == Random) {
        // 随机选择一首歌曲
        int randomIndex = qrand() % playlist.size();
        currentIndex = randomIndex;
    }

    QString song = playlist.at(currentIndex);

    // 如果是视频文件，加载视频
    if (song.endsWith(".mp4", Qt::CaseInsensitive)) {
        QStringList args;
        args << "-slave" << "-quiet" << "-idle" << "-wid"  << QString::number(ui->lyricWidget->winId())
             << "-geometry"<<"280:10"<<"-zoom"<<"-x"<<"461"<<"-y"<< "300" << song;
        mplayer->start("mplayer", args);
        qDebug() << "正在播放视频，索引：" << currentIndex;

        // 停止歌词同步
        lyricTimer.stop();
        ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));
        isPaused = false;

        // 高亮显示当前播放的视频
        highlightCurrentSong();
        return;
    }

    // 启动 MPlayer 播放音乐
    QStringList args;
    args << "-slave" << "-quiet" << "-idle" << song;
    mplayer->start("mplayer", args);
    qDebug() << "正在播放歌曲，索引：" << currentIndex;

    // 加载歌词
    QString lyricPath = "/home/source/Lyric/" + QFileInfo(song).baseName() + ".lrc";
    qDebug() << "加载歌词：" << lyricPath;
    loadLyrics(lyricPath);

    // 设置按钮图标为暂停状态
    isPaused = false;
    ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));

    // 高亮显示当前播放的歌曲
    highlightCurrentSong();

    // 启动歌词同步
    lyricTimer.start();

    // 更新播放器界面信息
    updatePlayerUI();
}

void Widget::updatePlayerUI()
{
    // 更新播放器界面信息，如当前播放时间、总时长等
    ui->playCourseSlider->setValue(static_cast<int>(currentPosition)); // 更新进度条位置
    int m = static_cast<int>(currentPosition) / 60;
    int s = static_cast<int>(currentPosition) % 60;
    ui->labCurTime->setText(QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'))); // 更新当前时间显示
    ui->playCourseSlider->setMaximum(static_cast<int>(totalDuration)); // 更新进度条最大值
    m = static_cast<int>(totalDuration) / 60;
    s = static_cast<int>(totalDuration) % 60;
    ui->labAllTime->setText(QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'))); // 更新总时长显示
}

void Widget::highlightCurrentSong()
{
    for (int i = 0; i < ui->listWidget->count(); i++) {
        QListWidgetItem* item = ui->listWidget->item(i);
        if (i == currentIndex) {
            item->setBackgroundColor(Qt::yellow);
        } else {
            item->setBackgroundColor(Qt::white);
        }
    }
}

void Widget::updateLyrics(QProcess::ProcessState newState)
{
    if (newState == QProcess::NotRunning) {
        // 播放器停止后清除歌词
        ui->lyricWidget->clear();
        // 清空歌词列表
        lyricEntries.clear();
        // 断开更新歌词的连接
        disconnect(mplayer, &QProcess::stateChanged, this, &Widget::updateLyrics);
    }
}

void Widget::syncLyrics()
{
    int currentTime = static_cast<int>(currentPosition); // 获取当前播放时间

    // 遍历歌词列表并显示歌词
    for (int i = 0; i < lyricEntries.size(); ++i) {
        int currentLyricTime = lyricEntries[i].timestamp.minute() * 60 + lyricEntries[i].timestamp.second();
        int nextLyricTime = (i + 1 < lyricEntries.size()) ?
            lyricEntries[i + 1].timestamp.minute() * 60 + lyricEntries[i + 1].timestamp.second() :
            totalDuration;

        // 添加歌词项到歌词列表
        QListWidgetItem *item = new QListWidgetItem(lyricEntries[i].text);
        ui->lyricWidget->addItem(item);

        // 如果当前时间在当前歌词时间范围内，则高亮显示该歌词
        if (currentTime >= currentLyricTime && currentTime < nextLyricTime) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
            ui->lyricWidget->setCurrentRow(i); // 滚动显示当前歌词
        }
    }
}

void Widget::displayLyrics()
{
    // 清空歌词列表
    ui->lyricWidget->clear();

    // 显示歌词
    foreach(const LyricEntry &entry, lyricEntries) {
        QListWidgetItem *item = new QListWidgetItem(entry.text);
        ui->lyricWidget->addItem(item);
    }
}


void Widget::on_btn_Res_clicked()
{
    // 选择音乐文件夹
    QString folderPath = QFileDialog::getExistingDirectory(this, tr("选择音乐文件夹"), QDir::homePath());

    if (!folderPath.isEmpty()) {
        // 加载选择的音乐文件夹中的歌曲
        loadPlaylist(folderPath);
    }

}

void Widget::loadPlaylist(const QString &folderPath)
{
    // 清空原歌单列表
    ui->listWidget->clear();
    playlist.clear();
    playbackTimer.stop();
    lyricTimer.stop(); // 停止歌词同步
    mplayer->write("pause\n");
    // 扫描指定路径下的音乐文件
    QStringList musicFiles = scanMusicFiles(folderPath);
    foreach (const QString &musicFile, musicFiles) {
        playlist.append(musicFile);
        ui->listWidget->addItem(musicFile); // 在 listWidget 中显示歌曲名称
    }

    // 重置当前播放索引
    currentIndex = 0;
}

QStringList Widget::scanMusicFiles(const QString &folderPath)
{

    QDir directory(folderPath);
    QStringList supportedFormats = {"*.mp3", "*.wav", "*.mp4"};
    return directory.entryList(supportedFormats, QDir::Files);
}

void Widget::on_btn_Player_clicked()
{
    if (mplayer->state() == QProcess::Running) {
        if (!isPaused) {
            mplayer->write("pause\n");
            playbackTimer.stop();
            lyricTimer.stop(); // 停止歌词同步
            if (!mplayer->waitForBytesWritten()) {
                return;
            }
            isPaused = true;
            ui->btn_Player->setIcon(QIcon(":/ICON/start.png")); // 设置为播放图标
        } else {
            mplayer->write("pause\n");
            if (!mplayer->waitForBytesWritten()) {
                return;
            }
            playbackTimer.start();
            lyricTimer.start(); // 恢复歌词同步
            isPaused = false;
            ui->btn_Player->setIcon(QIcon(":/ICON/stop.png")); // 设置为暂停图标
        }
    }
    else {
        // 播放器不在运行状态，播放歌曲
        playSong();
        playbackTimer.start();
        lyricTimer.start(); // 启动歌词同步
        isPaused = false;
        ui->btn_Player->setIcon(QIcon(":/ICON/stop.png")); // 设置为暂停图标
    }
}

void Widget::on_btn_LeftMove_clicked()
{
    if (playlist.isEmpty())
        return;

    // 先将状态设置为播放状态
    if (isPaused) {
        isPaused = false;
        playbackTimer.start();
        ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));
    }

    currentIndex = (currentIndex - 1 + playlist.size()) % playlist.size();
    playSong();
}

void Widget::on_btn_RightMove_clicked()
{
    if (playlist.isEmpty())
        return;

    // 先将状态设置为播放状态
    if (isPaused) {
        isPaused = false;
        playbackTimer.start();
        ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));
    }

    currentIndex = (currentIndex + 1) % playlist.size();
    playSong();
}

void Widget::loadPlaylist()
{
    // 清空原歌单列表
    ui->listWidget->clear();
    playlist.clear();

    // 扫描指定路径下的音乐文件
    QDir directory("/home/source/Music");
    QStringList musicFiles = directory.entryList(QStringList() << "*.mp3" << "*.wav" << "*.mp4", QDir::Files);
    foreach (QString musicFile, musicFiles) {
        playlist.append(directory.absoluteFilePath(musicFile));
        ui->listWidget->addItem(musicFile); // 在 listWidget 中显示歌曲名称
    }
    // 重置当前播放索引
    currentIndex = 0;
}

void Widget::loadLyrics(const QString &lyricPath)
{
    QFile file(lyricPath);  // 创建一个 QFile 对象，用于操作歌词文件
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 如果无法打开歌词文件，则输出错误信息并返回
        qDebug() << "Failed to open lyric file";
        return;
    }

    lyricEntries.clear();  // 清空现有的歌词条目
    ui->lyricWidget->clear();  // 清空界面上的歌词显示

    QTextStream in(&file);  // 创建一个 QTextStream 对象，用于读取文件内容
    while (!in.atEnd()) {  // 循环读取文件，直到文件末尾
        QString line = in.readLine();  // 读取一行歌词
        QRegExp rx("\\[(\\d{2}):(\\d{2})\\.(\\d{2})\\](.*)");  // 定义正则表达式，用于解析歌词的时间戳和内容
        if (rx.indexIn(line) != -1) {  // 如果正则表达式匹配成功
            int min = rx.cap(1).toInt();  // 提取分钟部分
            int sec = rx.cap(2).toInt();  // 提取秒钟部分
            int msec = rx.cap(3).toInt() * 10;  // 提取毫秒部分，并转换为实际时间
            QTime timestamp(0, min, sec, msec);  // 创建一个 QTime 对象表示时间戳
            QString text = rx.cap(4).trimmed();  // 提取并修剪歌词文本
            // 将时间戳和歌词文本作为一个条目添加到歌词列表中
            lyricEntries.append({ timestamp, text });
        }
    }

    file.close();  // 关闭歌词文件
    displayLyrics();  // 显示歌词
}

void Widget::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    // 先将状态设置为播放状态
    if (isPaused) {
        isPaused = false;
        playbackTimer.start();
        ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));
    }

    currentIndex = ui->listWidget->row(item);
    playSong();
}

void Widget::on_btn_Volume_clicked()
{
    // 显示或隐藏音量滑块
    ui->playerVolumeSlider->setVisible(!ui->playerVolumeSlider->isVisible());
}

void Widget::on_playerVolumeSlider_valueChanged(int value)
{
    // 调节音量（如果播放器正在运行）
    if (mplayer->state() == QProcess::Running) {
        QString volCmd = QString("volume %1 1\n").arg(value);
        mplayer->write(volCmd.toUtf8());
    }
}

void Widget::updateCurrentPosition()
{
    if (mplayer->state() == QProcess::Running) {
        mplayer->write("get_time_pos\n");
    }
}


void Widget::onStarted()
{
    qDebug() << "准备播放";
    // 获取播放文件的长度以毫秒为单位
    mplayer->write("get_time_length\n");
}

void Widget::onError(QProcess::ProcessError error)
{
    qDebug() << "播放进程出错:" << error;
}

void Widget::onReadyReadStandardOutput()
{
    while (mplayer->bytesAvailable()) {
        QByteArray arr = mplayer->readLine();
        QString str = QString::fromLocal8Bit(arr);
        if (str.contains("ANS_LENGTH")) {
            str.remove("\n").remove("\r").remove("ANS_LENGTH=");
            totalDuration = str.toDouble(); // 改为 double 以处理小数部分
            int m = static_cast<int>(totalDuration) / 60;
            int s = static_cast<int>(totalDuration) % 60;
            ui->labAllTime->setText(QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
            ui->playCourseSlider->setMaximum(static_cast<int>(totalDuration));
        }
        if (str.contains("ANS_TIME_POSITION")) {
            str.remove("\n").remove("\r");
            currentPosition = QString(str.split("=").at(1)).toDouble(); // 改为 double 以处理小数部分
            int m = static_cast<int>(currentPosition) / 60;
            int s = static_cast<int>(currentPosition) % 60;
            ui->labCurTime->setText(QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));
            ui->playCourseSlider->setValue(static_cast<int>(currentPosition));
            syncLyrics();
        }
    }
}

void Widget::onFinished(int exitCode)
{
    // 如果退出码为 0（正常退出），则播放下一首歌曲
    if (exitCode == 0) {
        // 重置当前播放位置为0
        currentPosition = 0;
        ui->playCourseSlider->setValue(0);
        ui->labCurTime->setText("00:00");

        // 如果当前处于暂停状态，则切换为播放状态
        if (isPaused) {
            isPaused = false;
            ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));
        }

        // 更新歌曲信息
        updateSongInfo();

        playSong();
    }
}

void Widget::updateSongInfo()
{
    if (currentIndex >= 0 && currentIndex < playlist.size()) {
        QString song = playlist.at(currentIndex);
        QString lyricPath = "/home/source/Lyric/" + QFileInfo(song).baseName() + ".lrc";

        if (!song.endsWith(".mp4", Qt::CaseInsensitive)) {
            // 如果不是视频文件，加载歌词
            loadLyrics(lyricPath);
        }

        // 更新列表显示
        highlightCurrentSong();
    }
}

void Widget::on_playCourseSlider_sliderMoved(int position)
{
    // 计算分钟和秒钟部分
    int m = position / 60;
    int s = position % 60;
    // 在界面上更新当前播放时间显示
    ui->labCurTime->setText(QString("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));

    // 更新歌词同步
    int currentTime = position;
    for (int i = 0; i < lyricEntries.size(); ++i) {
        // 计算当前歌词时间范围
        int currentLyricTime = lyricEntries[i].timestamp.minute() * 60 + lyricEntries[i].timestamp.second();
        int nextLyricTime = (i + 1 < lyricEntries.size()) ?
            lyricEntries[i + 1].timestamp.minute() * 60 + lyricEntries[i + 1].timestamp.second() :
            totalDuration;

        if (currentTime >= currentLyricTime && currentTime < nextLyricTime) {
            // 如果当前时间在当前歌词时间范围内，则设置该歌词为加粗显示，并将其滚动到视图中央
            QFont font = ui->lyricWidget->item(i)->font();
            font.setBold(true);
            ui->lyricWidget->item(i)->setFont(font);
            ui->lyricWidget->setCurrentRow(i);
        } else {
            // 如果不是当前歌词范围内的时间，则取消加粗显示
            QFont font = ui->lyricWidget->item(i)->font();
            font.setBold(false);
            ui->lyricWidget->item(i)->setFont(font);
        }
    } 
}


void Widget::on_playCourseSlider_sliderReleased()
{
    playbackTimer.start();

    // 获取滑动条的值，即歌曲的时间位置
    int position = ui->playCourseSlider->value();
    ui->btn_Player->setIcon(QIcon(":/ICON/stop.png"));
    // 发送设置歌曲播放时间的命令给播放器进程
    QString setPositionCmd = QString("seek %1 2\n").arg(position);
    mplayer->write(setPositionCmd.toUtf8());

    // 更新当前播放位置
    currentPosition = position;

    // 同步歌词显示
    syncLyrics();
}

void Widget::on_playCourseSlider_sliderPressed()
{
    playbackTimer.stop();
}

void Widget::on_btn_model_clicked()
{
    // 切换播放模式
    if (playMode == Sequential) {
        playMode = Random;
        ui->btn_model->setIcon(QIcon(":/ICON/Random.png")); // 设置随机播放图标
    } else {
        playMode = Sequential;
        ui->btn_model->setIcon(QIcon(":/ICON/Loop.png")); // 设置顺序播放图标
    }
}
