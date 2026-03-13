#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QAtomicInt>

class Worker : public QObject {
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);
    void requestStop();

public slots:
    void processFiles(const QString& srcDir, const QString& mask, bool deleteInput,
                      const QString& outDir, int conflictPolicy, const QByteArray& key);

signals:
    void progress(int percent);
    void status(const QString& msg);
    void finished();

private:
    QAtomicInt m_stopFlag;
    QString resolveConflict(const QString& filePath, int policy);
};

#endif // WORKER_H
