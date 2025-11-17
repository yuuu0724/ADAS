#include "display.h"
#include <QKeyEvent>
#include <QTime>
#include <QVBoxLayout>
#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QMutex>
#include <QApplication>
#include <QDesktopWidget>
QMutex mutex;

DisplayWindow::DisplayWindow(int width, int height, QWidget *parent)
    : QWidget(parent), screenWidth(width),screenHeight(height),frame_count(0),fps(0)
{
    setWindowTitle("Seatbelt ADAS");
    setFixedSize(screenWidth,screenHeight);
    setWindowFlags(Qt::FramelessWindowHint);
    label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(screenWidth,screenHeight);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    last_time = QTime::currentTime();

    QTimer *timer = new QTimer(this);
    connect(timer,&QTimer::timeout,this,&DisplayWindow::updateFrame);
    timer->start(16);

    int screenWidth_total = QApplication::desktop()->screenGeometry().width();
    int screenHeight_total = QApplication::desktop()->screenGeometry().height();
    this->move(0, 0); // 靠左上角
}

void DisplayWindow::pushFrame(const cv::Mat &frame)
{
    QMutexLocker locker(&mutex);
    if(frame_queue.size()>5) frame_queue.pop();
    frame_queue.push(frame.clone());
}

void DisplayWindow::updateFrame()
{
    cv::Mat frame;
    {
        QMutexLocker locker(&mutex);
        if(!frame_queue.empty())
        {
            frame = frame_queue.back().clone();
            while(!frame_queue.empty())
            {
                frame_queue.pop();
            }
        }
    }
    if(frame.empty()) return;

    frame_count++;
    int elapsed = last_time.msecsTo(QTime::currentTime());
    if(elapsed >=1000){
        fps = frame_count *1000.0 /elapsed;
        frame_count = 0;
        last_time = QTime::currentTime();
    }
    
    cv::putText(frame, QString("FPS:%1").arg(fps,0,'f',1).toStdString(),cv::Point(20,40),
                cv::FONT_HERSHEY_SIMPLEX,1.0,cv::Scalar(0,255,0),2);
    cv::Mat rgb_frame;
    cv::cvtColor(frame,rgb_frame,cv::COLOR_BGR2RGB);
    QImage img(rgb_frame.data,rgb_frame.cols,rgb_frame.rows,rgb_frame.step,QImage::Format_RGB888);
    label->setPixmap(QPixmap::fromImage(img).scaled(screenWidth,screenHeight,Qt::KeepAspectRatio));
}

void DisplayWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        QApplication::quit();
    }
}