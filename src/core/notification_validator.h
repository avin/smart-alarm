#pragma once

#include "core/notification.h"

namespace smartalarm {

enum class ValidationErrorCode {
    Required,
    OutOfRange,
    Invalid
};

struct ValidationError {
    ValidationErrorCode code = ValidationErrorCode::Invalid;
    QString fieldPath;
};

struct ValidationResult {
    QVector<ValidationError> errors;
    [[nodiscard]] bool ok() const { return errors.isEmpty(); }
    void add(ValidationErrorCode code, QString fieldPath) { errors.push_back({ code, std::move(fieldPath) }); }
};

class NotificationValidator {
public:
    static ValidationResult validate(const Notification &notification);
};

} // namespace smartalarm
