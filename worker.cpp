#include "worker.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>

Worker::Worker(QObject *parent) : QObject(parent), m_stopFlag(0) {}

void Worker::requestStop() {
    m_stopFlag = 1;
}

QString Worker::resolveConflict(const QString& filePath, int policy) {
    if (policy == 0) return filePath; // Перезапись

    QFileInfo fi(filePath);
    QString base = fi.absolutePath() + "/" + fi.completeBaseName();
    QString ext = fi.suffix().isEmpty() ? "" : "." + fi.suffix();

    int counter = 1;
    QString newPath = filePath;
    while (QFile::exists(newPath)) {
        newPath = base + QString("_%1").arg(counter++) + ext;
    }
    return newPath;
}

void Worker::processFiles(const QString& srcDir, const QString& mask, bool deleteInput,
                          const QString& outDir, int conflictPolicy, const QByteArray& key) {
    m_stopFlag = 0;
    QDir dir(srcDir);
    if (!dir.exists()) {
        emit status("Ошибка: Исходная директория не существует.");
        emit finished();
        return;
    }

    QStringList filters = mask.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
    dir.setNameFilters(filters);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoSymLinks);

    if (files.isEmpty()) {
        emit status("Файлы по заданной маске не найдены.");
        emit finished();
        return;
    }

    int totalFiles = files.size();
    for (int i = 0; i < totalFiles; ++i) {
        if (m_stopFlag.loadRelaxed()) {
            emit status("Остановлено пользователем.");
            break;
        }

        const QFileInfo& fi = files.at(i);
        QString outPath = resolveConflict(outDir + "/" + fi.fileName(), conflictPolicy);

        QFile inFile(fi.absoluteFilePath());
        QFile outFile(outPath);

        if (!inFile.open(QIODevice::ReadOnly)) {
            emit status(QString("Не удалось открыть %1").arg(fi.fileName()));
            continue;
        }
        if (!outFile.open(QIODevice::WriteOnly)) {
            emit status(QString("Не удалось создать %1").arg(QFileInfo(outPath).fileName()));
            continue;
        }

        emit status(QString("Обработка: %1 (%2/%3)").arg(fi.fileName()).arg(i + 1).arg(totalFiles));

        const qint64 fileSize = inFile.size();
        qint64 processedSize = 0;
        const qint64 chunkSize = 1024 * 1024; // 1 МБ буфер
        qint64 byteIndex = 0;

        while (!inFile.atEnd() && !m_stopFlag.loadRelaxed()) {
            QByteArray buffer = inFile.read(chunkSize);
            for (int j = 0; j < buffer.size(); ++j) {
                buffer[j] = buffer[j] ^ key[static_cast<int>(byteIndex % 8)];
                byteIndex++;
            }
            outFile.write(buffer);
            processedSize += buffer.size();

            int currentProgress = (i * 100 / totalFiles) + ((processedSize * 100 / fileSize) / totalFiles);
            emit progress(currentProgress);
        }

        inFile.close();
        outFile.close();

        if (deleteInput && !m_stopFlag.loadRelaxed()) {
            QFile::remove(fi.absoluteFilePath());
        }
    }

    if (!m_stopFlag.loadRelaxed()) {
        emit progress(100);
        emit status("Обработка завершена.");
    }
    emit finished();
}
