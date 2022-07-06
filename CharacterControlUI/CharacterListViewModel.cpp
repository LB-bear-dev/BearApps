#include "PCH.h"
#include "CharacterListViewModel.h"
#pragma optimize("", off)
CharacterListViewModel::CharacterListViewModel(QObject* parent) : QAbstractListModel(parent)
{

}

int CharacterListViewModel::rowCount(const QModelIndex& parent) const
{
	return 10;
}

QVariant CharacterListViewModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DisplayRole)
        return QString("Row%1, Column%2")
        .arg(index.row() + 1)
        .arg(index.column() + 1);

    return QVariant();
}
