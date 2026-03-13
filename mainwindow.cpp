#include "mainwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isRunning(false) {
    setupUI();

    worker = new Worker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(worker, &Worker::progress, progressBar, &QProgressBar::setValue);
    connect(worker, &Worker::status, statusLabel, &QLabel::setText);
    connect(worker, &Worker::finished, this, &MainWindow::workerFinished);

    runTimer = new QTimer(this);
    connect(runTimer, &QTimer::timeout, this, &MainWindow::onTimerTicked);

    workerThread.start();
}

MainWindow::~MainWindow() {
    worker->requestStop();
    workerThread.quit();
    workerThread.wait();
}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    QFormLayout *formLayout = new QFormLayout();

    srcDirEdit = new QLineEdit();
    QPushButton *srcBtn = new QPushButton("Обзор...");
    QHBoxLayout *srcLayout = new QHBoxLayout();
    srcLayout->addWidget(srcDirEdit);
    srcLayout->addWidget(srcBtn);
    connect(srcBtn, &QPushButton::clicked, this, &MainWindow::browseSrc);

    maskEdit = new QLineEdit("*.txt *.bin");
    deleteCheck = new QCheckBox("Удалять входные файлы");

    outDirEdit = new QLineEdit();
    QPushButton *outBtn = new QPushButton("Обзор...");
    QHBoxLayout *outLayout = new QHBoxLayout();
    outLayout->addWidget(outDirEdit);
    outLayout->addWidget(outBtn);
    connect(outBtn, &QPushButton::clicked, this, &MainWindow::browseOut);

    conflictCombo = new QComboBox();
    conflictCombo->addItems({"Перезапись", "Добавить счетчик (модификация имени)"});

    modeCombo = new QComboBox();
    modeCombo->addItems({"Разовый запуск", "По таймеру"});
    timerSpinBox = new QSpinBox();
    timerSpinBox->setSuffix(" сек");
    timerSpinBox->setRange(1, 3600);
    timerSpinBox->setValue(60);

    keyEdit = new QLineEdit("0123456789ABCDEF");
    keyEdit->setMaxLength(16);
    keyEdit->setToolTip("Введите 16 шестнадцатеричных символов (8 байт)");

    formLayout->addRow("Исходная папка:", srcLayout);
    formLayout->addRow("Маска файлов:", maskEdit);
    formLayout->addRow("", deleteCheck);
    formLayout->addRow("Папка сохранения:", outLayout);
    formLayout->addRow("При совпадении имен:", conflictCombo);
    formLayout->addRow("Режим работы:", modeCombo);
    formLayout->addRow("Интервал таймера:", timerSpinBox);
    formLayout->addRow("XOR Ключ (HEX 8 байт):", keyEdit);

    mainLayout->addLayout(formLayout);

    startBtn = new QPushButton("Старт");
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::toggleRun);
    mainLayout->addWidget(startBtn);

    progressBar = new QProgressBar();
    progressBar->setValue(0);
    mainLayout->addWidget(progressBar);

    statusLabel = new QLabel("Готов");
    mainLayout->addWidget(statusLabel);

    setCentralWidget(central);
    setWindowTitle("File XOR Modifier");
    resize(500, 350);
}

void MainWindow::browseSrc() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите исходную папку");
    if (!dir.isEmpty()) srcDirEdit->setText(dir);
}

void MainWindow::browseOut() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку сохранения");
    if (!dir.isEmpty()) outDirEdit->setText(dir);
}

QByteArray MainWindow::getHexKey() const {
    QString hex = keyEdit->text().remove(QRegularExpression("[^0-9A-Fa-f]"));
    while (hex.length() < 16) hex += "0"; // Дополняем нулями если не хватает
    return QByteArray::fromHex(hex.toUtf8());
}

void MainWindow::toggleRun() {
    if (isRunning) {
        worker->requestStop();
        if (runTimer->isActive()) runTimer->stop();
        statusLabel->setText("Остановка...");
        startBtn->setEnabled(false);
    } else {
        if (srcDirEdit->text().isEmpty() || outDirEdit->text().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Укажите исходную и целевую папки!");
            return;
        }

        isRunning = true;
        startBtn->setText("Стоп");
        lockUI(true);

        if (modeCombo->currentIndex() == 1) { // По таймеру
            runTimer->start(timerSpinBox->value() * 1000);
            statusLabel->setText("Ожидание таймера...");
            onTimerTicked(); // Сразу запускаем первый проход
        } else {
            onTimerTicked();
        }
    }
}

void MainWindow::onTimerTicked() {
    progressBar->setValue(0);
    QMetaObject::invokeMethod(worker, "processFiles", Qt::QueuedConnection,
                              Q_ARG(QString, srcDirEdit->text()),
                              Q_ARG(QString, maskEdit->text()),
                              Q_ARG(bool, deleteCheck->isChecked()),
                              Q_ARG(QString, outDirEdit->text()),
                              Q_ARG(int, conflictCombo->currentIndex()),
                              Q_ARG(QByteArray, getHexKey()));
}

void MainWindow::workerFinished() {
    if (modeCombo->currentIndex() == 0 || !isRunning) { // Разовый запуск или остановлено
        isRunning = false;
        startBtn->setText("Старт");
        startBtn->setEnabled(true);
        lockUI(false);
    } else {
        statusLabel->setText(QString("Ожидание... Следующий запуск через %1 сек.").arg(timerSpinBox->value()));
    }
}

void MainWindow::lockUI(bool locked) {
    srcDirEdit->setEnabled(!locked);
    maskEdit->setEnabled(!locked);
    deleteCheck->setEnabled(!locked);
    outDirEdit->setEnabled(!locked);
    conflictCombo->setEnabled(!locked);
    modeCombo->setEnabled(!locked);
    timerSpinBox->setEnabled(!locked);
    keyEdit->setEnabled(!locked);
}
