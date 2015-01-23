#include "MusicFilesWidget.h"
#include "ui_MusicFilesWidget.h"

#include <QDebug>
#include "../globals/Manager.h"
#include "MusicMultiScrapeDialog.h"

MusicFilesWidget *MusicFilesWidget::m_instance;

MusicFilesWidget::MusicFilesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MusicFilesWidget)
{
    m_instance = this;
    Manager::instance()->setMusicFilesWidget(this);
    ui->setupUi(this);

    m_proxyModel = new MusicProxyModel(this);
    m_proxyModel->setSourceModel(Manager::instance()->musicModel());
    ui->music->setModel(m_proxyModel);
    ui->music->sortByColumn(0, Qt::AscendingOrder);
    ui->music->setAttribute(Qt::WA_MacShowFocusRect, false);

    ui->statusLabel->setText(tr("%n artist(s)", "", 0) + ", " + tr("%n album(s)", "", 0));

#ifdef Q_OS_WIN
    ui->music->setAnimated(false);
#endif

    connect(ui->music->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(onItemSelected(QModelIndex)), Qt::QueuedConnection);
    connect(m_proxyModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateStatusLabel()));
    connect(m_proxyModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(updateStatusLabel()));
}

MusicFilesWidget::~MusicFilesWidget()
{
    delete ui;
}

MusicFilesWidget *MusicFilesWidget::instance()
{
    return m_instance;
}

void MusicFilesWidget::setFilter(QList<Filter *> filters, QString text)
{
    if (!filters.isEmpty())
        m_proxyModel->setFilterWildcard("*" + filters.first()->shortText() + "*");
    else
        m_proxyModel->setFilterWildcard("*" + text + "*");
    m_proxyModel->setFilter(filters, text);
}

void MusicFilesWidget::onItemSelected(QModelIndex index)
{
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    if (Manager::instance()->musicModel()->getItem(sourceIndex)->type() == TypeArtist)
        emit sigArtistSelected(Manager::instance()->musicModel()->getItem(sourceIndex)->artist());
    else if (Manager::instance()->musicModel()->getItem(sourceIndex)->type() == TypeAlbum)
        emit sigAlbumSelected(Manager::instance()->musicModel()->getItem(sourceIndex)->album());
}

void MusicFilesWidget::updateStatusLabel()
{
    if (m_proxyModel->filterRegExp().pattern().isEmpty() || m_proxyModel->filterRegExp().pattern() == "**") {
        int albumCount = 0;
        foreach (Artist *artist, Manager::instance()->musicModel()->artists())
            albumCount += artist->albums().count();
        ui->statusLabel->setText(tr("%n artists", "", m_proxyModel->rowCount()) + ", " + tr("%n albums", "", albumCount));
    } else {
        ui->statusLabel->setText(tr("%1 of %n artists", "", Manager::instance()->musicModel()->artists().count()).arg(m_proxyModel->rowCount()));
    }
}

QList<Artist*> MusicFilesWidget::selectedArtists()
{
    QList<Artist*> artists;
    foreach (const QModelIndex &index, ui->music->selectionModel()->selectedIndexes()) {
        MusicModelItem *item = Manager::instance()->musicModel()->getItem(m_proxyModel->mapToSource(index));
        if (item->type() == TypeArtist)
            artists.append(item->artist());
    }
    return artists;
}

QList<Album*> MusicFilesWidget::selectedAlbums()
{
    QList<Album*> albums;
    foreach (const QModelIndex &index, ui->music->selectionModel()->selectedIndexes()) {
        MusicModelItem *item = Manager::instance()->musicModel()->getItem(m_proxyModel->mapToSource(index));
        if (item->type() == TypeAlbum)
            albums.append(item->album());
    }
    return albums;
}

void MusicFilesWidget::multiScrape()
{
    MusicMultiScrapeDialog::instance()->setItems(selectedArtists(), selectedAlbums());
    MusicMultiScrapeDialog::instance()->exec();
}
