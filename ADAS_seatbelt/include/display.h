#pragma once 
#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QTime>
#include <QMutex>
#include <queue>
#include <opencv2/opencv.hpp>

class DisplayWindow : public QWidget{
    Q_OBJECT

public:
    DisplayWindow(int width, int height, QWidget *parent = NULL);
    void pushFrame(const cv::Mat &frame);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateFrame();

private:
    QLabel *label;
    std::queue<cv::Mat> frame_queue;
    int screenWidth;
    int screenHeight;
    int frame_count;
    double fps;
    QTime last_time;
};