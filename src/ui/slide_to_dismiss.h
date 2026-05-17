#pragma once

#include <QPropertyAnimation>
#include <QWidget>

namespace smartalarm {

class SlideToDismiss : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int handleX READ handleX WRITE setHandleX)
public:
    explicit SlideToDismiss(QWidget *parent = nullptr);

    int handleX() const;
    void setHandleX(int x);

signals:
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    int maxHandleX() const;
    void animateBack();

    int m_handleX = 0;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

} // namespace smartalarm
