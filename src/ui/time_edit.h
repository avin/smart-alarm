#pragma once

#include <QLineEdit>
#include <QToolButton>
#include <QWidget>

#include <optional>

namespace smartalarm {

class TimeEdit : public QWidget {
    Q_OBJECT
public:
    explicit TimeEdit(QWidget *parent = nullptr);

    std::optional<QTime> time() const;
    void setTime(std::optional<QTime> time);
    void setInvalid(bool invalid);

signals:
    void changed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void normalizeText();
    void updateClearButtonGeometry();

    QLineEdit *m_edit = nullptr;
    QToolButton *m_clearButton = nullptr;
};

} // namespace smartalarm
