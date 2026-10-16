#pragma once

#include <QAbstractItemView>
#include <QComboBox>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

class ComboPopupDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyleOptionViewItem itemOption(option);
        itemOption.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, itemOption, index);
    }
};

inline void polishComboPopup(QComboBox *comboBox)
{
    comboBox->view()->setItemDelegate(new ComboPopupDelegate(comboBox));
}
