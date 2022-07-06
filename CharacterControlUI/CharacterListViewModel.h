#pragma once
#include <QAbstractTableModel>

class CharacterListViewModel : public QAbstractListModel
{
    Q_OBJECT
public:
    CharacterListViewModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
};