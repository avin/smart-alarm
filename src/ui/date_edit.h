#pragma once

#include <QDate>
#include <QLineEdit>
#include <QToolButton>
#include <QWidget>

#include <optional>

namespace smartalarm {

class DateEdit : public QWidget {
    Q_OBJECT
public:
    explicit DateEdit(QWidget *parent = nullptr);

    std::optional<QDate> date() const;
    void setDate(std::optional<QDate> date);
    void setInvalid(bool invalid);

signals:
    void changed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void openCalendar();
    void updateClearButtonGeometry();

    QLineEdit *m_edit = nullptr;
    QToolButton *m_clearButton = nullptr;
    std::optional<QDate> m_date;
};

} // namespace smartalarm
