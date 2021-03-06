/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2016 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <QDialog>
#include <QWidget>
#include <QList>
#include <QListWidgetItem>
#include <QSettings>
#include <QFileDialog>
#include <QThread>
#include "settingsdialog.h"
#include "applicationdialog.h"
#include "applicationlist.h"
#include "translationhandler.h"
#include "common.h"

SettingsDialog::SettingsDialog(ApplicationList *list,
                               TranslationHandler *translator,
                               QWidget *parent) :
    QDialog(parent),
    mApplications(list),
    mTempApplications(new ApplicationList(this)),
    mTranslator(translator)
{
    mUI.setupUi(this);
    QSettings settings;
    mTempApplications->copy(list);

    mUI.mJobs->setText(settings.value(SETTINGS_CHECK_THREADS, 1).toString());
    mUI.mForce->setCheckState(boolToCheckState(settings.value(SETTINGS_CHECK_FORCE, false).toBool()));
    mUI.mShowFullPath->setCheckState(boolToCheckState(settings.value(SETTINGS_SHOW_FULL_PATH, false).toBool()));
    mUI.mShowNoErrorsMessage->setCheckState(boolToCheckState(settings.value(SETTINGS_SHOW_NO_ERRORS, false).toBool()));
    mUI.mShowDebugWarnings->setCheckState(boolToCheckState(settings.value(SETTINGS_SHOW_DEBUG_WARNINGS, false).toBool()));
    mUI.mSaveAllErrors->setCheckState(boolToCheckState(settings.value(SETTINGS_SAVE_ALL_ERRORS, false).toBool()));
    mUI.mSaveFullPath->setCheckState(boolToCheckState(settings.value(SETTINGS_SAVE_FULL_PATH, false).toBool()));
    mUI.mInlineSuppressions->setCheckState(boolToCheckState(settings.value(SETTINGS_INLINE_SUPPRESSIONS, false).toBool()));
    mUI.mEnableInconclusive->setCheckState(boolToCheckState(settings.value(SETTINGS_INCONCLUSIVE_ERRORS, false).toBool()));
    mUI.mShowStatistics->setCheckState(boolToCheckState(settings.value(SETTINGS_SHOW_STATISTICS, false).toBool()));
    mUI.mShowErrorId->setCheckState(boolToCheckState(settings.value(SETTINGS_SHOW_ERROR_ID, false).toBool()));

#ifdef Q_OS_WIN
    mUI.mLabelVsInclude->setVisible(true);
    mUI.mEditVsInclude->setVisible(true);
    mUI.mEditVsInclude->setText(settings.value(SETTINGS_VS_INCLUDE_PATHS, QString()).toString());
#else
    mUI.mLabelVsInclude->setVisible(false);
    mUI.mEditVsInclude->setVisible(false);
    mUI.mEditVsInclude->setText(QString());
#endif
    connect(mUI.mButtons, &QDialogButtonBox::accepted, this, &SettingsDialog::ok);
    connect(mUI.mButtons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(mUI.mBtnAddApplication, SIGNAL(clicked()),
            this, SLOT(addApplication()));
    connect(mUI.mBtnRemoveApplication, SIGNAL(clicked()),
            this, SLOT(removeApplication()));
    connect(mUI.mBtnEditApplication, SIGNAL(clicked()),
            this, SLOT(editApplication()));
    connect(mUI.mBtnDefaultApplication, SIGNAL(clicked()),
            this, SLOT(defaultApplication()));
    connect(mUI.mListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
            this, SLOT(editApplication()));
    connect(mUI.mBtnAddIncludePath, SIGNAL(clicked()),
            this, SLOT(addIncludePath()));
    connect(mUI.mBtnRemoveIncludePath, SIGNAL(clicked()),
            this, SLOT(removeIncludePath()));
    connect(mUI.mBtnEditIncludePath, SIGNAL(clicked()),
            this, SLOT(editIncludePath()));

    mUI.mListWidget->setSortingEnabled(false);
    populateApplicationList();

    const int count = QThread::idealThreadCount();
    if (count != -1)
        mUI.mLblIdealThreads->setText(QString::number(count));
    else
        mUI.mLblIdealThreads->setText(tr("N/A"));

    loadSettings();
    initTranslationsList();
    initIncludepathsList();
}

SettingsDialog::~SettingsDialog()
{
    saveSettings();
}

void SettingsDialog::addIncludePath(const QString &path)
{
    if (path.isNull() || path.isEmpty())
        return;

    QListWidgetItem *item = new QListWidgetItem(path);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    mUI.mListIncludePaths->addItem(item);
}

void SettingsDialog::initIncludepathsList()
{
    QSettings settings;
    const QString allPaths = settings.value(SETTINGS_GLOBAL_INCLUDE_PATHS).toString();
    const QStringList paths = allPaths.split(";", QString::SkipEmptyParts);
    foreach (QString path, paths) {
        addIncludePath(path);
    }
}

void SettingsDialog::initTranslationsList()
{
    const QString current = mTranslator->getCurrentLanguage();
    QList<TranslationInfo> translations = mTranslator->getTranslations();
    foreach (TranslationInfo translation, translations) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(translation.mName);
        item->setData(LangCodeRole, QVariant(translation.mCode));
        mUI.mListLanguages->addItem(item);
        if (translation.mCode == current || translation.mCode == current.mid(0, 2))
            mUI.mListLanguages->setCurrentItem(item);
    }
}

Qt::CheckState SettingsDialog::boolToCheckState(bool yes)
{
    if (yes) {
        return Qt::Checked;
    }
    return Qt::Unchecked;
}

bool SettingsDialog::checkStateToBool(Qt::CheckState state)
{
    if (state == Qt::Checked) {
        return true;
    }
    return false;
}


void SettingsDialog::loadSettings()
{
    QSettings settings;
    resize(settings.value(SETTINGS_CHECK_DIALOG_WIDTH, 800).toInt(),
           settings.value(SETTINGS_CHECK_DIALOG_HEIGHT, 600).toInt());
}

void SettingsDialog::saveSettings() const
{
    QSettings settings;
    settings.setValue(SETTINGS_CHECK_DIALOG_WIDTH, size().width());
    settings.setValue(SETTINGS_CHECK_DIALOG_HEIGHT, size().height());
}

void SettingsDialog::saveSettingValues() const
{
    int jobs = mUI.mJobs->text().toInt();
    if (jobs <= 0) {
        jobs = 1;
    }

    QSettings settings;
    settings.setValue(SETTINGS_CHECK_THREADS, jobs);
    saveCheckboxValue(&settings, mUI.mForce, SETTINGS_CHECK_FORCE);
    saveCheckboxValue(&settings, mUI.mSaveAllErrors, SETTINGS_SAVE_ALL_ERRORS);
    saveCheckboxValue(&settings, mUI.mSaveFullPath, SETTINGS_SAVE_FULL_PATH);
    saveCheckboxValue(&settings, mUI.mShowFullPath, SETTINGS_SHOW_FULL_PATH);
    saveCheckboxValue(&settings, mUI.mShowNoErrorsMessage, SETTINGS_SHOW_NO_ERRORS);
    saveCheckboxValue(&settings, mUI.mShowDebugWarnings, SETTINGS_SHOW_DEBUG_WARNINGS);
    saveCheckboxValue(&settings, mUI.mInlineSuppressions, SETTINGS_INLINE_SUPPRESSIONS);
    saveCheckboxValue(&settings, mUI.mEnableInconclusive, SETTINGS_INCONCLUSIVE_ERRORS);
    saveCheckboxValue(&settings, mUI.mShowStatistics, SETTINGS_SHOW_STATISTICS);
    saveCheckboxValue(&settings, mUI.mShowErrorId, SETTINGS_SHOW_ERROR_ID);

#ifdef Q_OS_WIN
    QString vsIncludePaths = mUI.mEditVsInclude->text();
    if (vsIncludePaths.startsWith("INCLUDE="))
        vsIncludePaths.remove(0, 8);
    settings.setValue(SETTINGS_VS_INCLUDE_PATHS, vsIncludePaths);
#endif

    const QListWidgetItem *currentLang = mUI.mListLanguages->currentItem();
    if (currentLang) {
        const QString langcode = currentLang->data(LangCodeRole).toString();
        settings.setValue(SETTINGS_LANGUAGE, langcode);
    }

    const int count = mUI.mListIncludePaths->count();
    QString includePaths;
    for (int i = 0; i < count; i++) {
        QListWidgetItem *item = mUI.mListIncludePaths->item(i);
        includePaths += item->text();
        includePaths += ";";
    }
    settings.setValue(SETTINGS_GLOBAL_INCLUDE_PATHS, includePaths);
}

void SettingsDialog::saveCheckboxValue(QSettings *settings, QCheckBox *box,
                                       const QString &name)
{
    settings->setValue(name, checkStateToBool(box->checkState()));
}

void SettingsDialog::addApplication()
{
    Application app;
    ApplicationDialog dialog(tr("Add a new application"), app, this);

    if (dialog.exec() == QDialog::Accepted) {
        mTempApplications->addApplication(app);
        mUI.mListWidget->addItem(app.getName());
    }
}

void SettingsDialog::removeApplication()
{
    QList<QListWidgetItem *> selected = mUI.mListWidget->selectedItems();
    foreach (QListWidgetItem *item, selected) {
        const int removeIndex = mUI.mListWidget->row(item);
        const int currentDefault = mTempApplications->getDefaultApplication();
        mTempApplications->removeApplication(removeIndex);
        if (removeIndex == currentDefault)
            // If default app is removed set default to unknown
            mTempApplications->setDefault(-1);
        else if (removeIndex < currentDefault)
            // Move default app one up if earlier app was removed
            mTempApplications->setDefault(currentDefault - 1);
    }
    mUI.mListWidget->clear();
    populateApplicationList();
}

void SettingsDialog::editApplication()
{
    QList<QListWidgetItem *> selected = mUI.mListWidget->selectedItems();
    QListWidgetItem *item = 0;
    foreach (item, selected) {
        int row = mUI.mListWidget->row(item);
        Application& app = mTempApplications->getApplication(row);
        ApplicationDialog dialog(tr("Modify an application"), app, this);

        if (dialog.exec() == QDialog::Accepted) {
            QString name = app.getName();
            if (mTempApplications->getDefaultApplication() == row)
                name += tr(" [Default]");
            item->setText(name);
        }
    }
}

void SettingsDialog::defaultApplication()
{
    QList<QListWidgetItem *> selected = mUI.mListWidget->selectedItems();
    if (!selected.isEmpty()) {
        int index = mUI.mListWidget->row(selected[0]);
        mTempApplications->setDefault(index);
        mUI.mListWidget->clear();
        populateApplicationList();
    }
}

void SettingsDialog::populateApplicationList()
{
    const int defapp = mTempApplications->getDefaultApplication();
    for (int i = 0; i < mTempApplications->getApplicationCount(); i++) {
        const Application& app = mTempApplications->getApplication(i);
        QString name = app.getName();
        if (i == defapp) {
            name += " ";
            name += tr("[Default]");
        }
        mUI.mListWidget->addItem(name);
    }

    // Select default application, or if there is no default app then the
    // first item.
    if (defapp == -1)
        mUI.mListWidget->setCurrentRow(0);
    else {
        if (mTempApplications->getApplicationCount() > defapp)
            mUI.mListWidget->setCurrentRow(defapp);
        else
            mUI.mListWidget->setCurrentRow(0);
    }
}

void SettingsDialog::ok()
{
    mApplications->copy(mTempApplications);
    accept();
}

bool SettingsDialog::showFullPath() const
{
    return checkStateToBool(mUI.mShowFullPath->checkState());
}

bool SettingsDialog::saveFullPath() const
{
    return checkStateToBool(mUI.mSaveFullPath->checkState());
}

bool SettingsDialog::saveAllErrors() const
{
    return checkStateToBool(mUI.mSaveAllErrors->checkState());
}

bool SettingsDialog::showNoErrorsMessage() const
{
    return checkStateToBool(mUI.mShowNoErrorsMessage->checkState());
}

bool SettingsDialog::showErrorId() const
{
    return checkStateToBool(mUI.mShowErrorId->checkState());
}

bool SettingsDialog::showInconclusive() const
{
    return checkStateToBool(mUI.mEnableInconclusive->checkState());
}

void SettingsDialog::addIncludePath()
{
    QString selectedDir = QFileDialog::getExistingDirectory(this,
                          tr("Select include directory"),
                          getPath(SETTINGS_LAST_INCLUDE_PATH));

    if (!selectedDir.isEmpty()) {
        addIncludePath(selectedDir);
        setPath(SETTINGS_LAST_INCLUDE_PATH, selectedDir);
    }
}

void SettingsDialog::removeIncludePath()
{
    const int row = mUI.mListIncludePaths->currentRow();
    QListWidgetItem *item = mUI.mListIncludePaths->takeItem(row);
    delete item;
}

void SettingsDialog::editIncludePath()
{
    QListWidgetItem *item = mUI.mListIncludePaths->currentItem();
    mUI.mListIncludePaths->editItem(item);
}
