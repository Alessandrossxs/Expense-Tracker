#pragma once

#include <QCoreApplication>
#include <QString>

inline QString translatedCategoryName(const QString &name)
{
    static const char *const categoryNames[] = {
        QT_TRANSLATE_NOOP("CategoryNames", "Stipendio"),
        QT_TRANSLATE_NOOP("CategoryNames", "Regali"),
        QT_TRANSLATE_NOOP("CategoryNames", "Altro"),
        QT_TRANSLATE_NOOP("CategoryNames", "Alimentari"),
        QT_TRANSLATE_NOOP("CategoryNames", "Trasporti"),
        QT_TRANSLATE_NOOP("CategoryNames", "Casa"),
        QT_TRANSLATE_NOOP("CategoryNames", "Svago"),
        QT_TRANSLATE_NOOP("CategoryNames", "Salute")
    };
    for (const char *categoryName : categoryNames) {
        if (name == QString::fromUtf8(categoryName))
            return QCoreApplication::translate("CategoryNames", categoryName);
    }
    return name;
}
