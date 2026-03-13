#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTimer>
#include "worker.h"

class QLineEdit;
class QCheckBox;
class QComboBox;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void browseSrc();
    void browseOut();
    void toggleRun();
    void onTimerTicked();
    void workerFinished();

private:
    void setupUI();
    void lockUI(bool locked);
    QByteArray getHexKey() const;

    QLineEdit *srcDirEdit, *outDirEdit, *maskEdit, *keyEdit;
    QCheckBox *deleteCheck;
    QComboBox *conflictCombo, *modeCombo;
    QSpinBox *timerSpinBox;
    QPushButton *startBtn;
    QProgressBar *progressBar;
    QLabel *statusLabel;

    QThread workerThread;
    Worker *worker;
    QTimer *runTimer;
    bool isRunning;
};

#endif // MAINWINDOW_H
