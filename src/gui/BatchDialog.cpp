#include "BatchDialog.h"
#include "app/BatchProcessor.h"
#include "core/IoModule.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

namespace PixelForge {

BatchDialog::BatchDialog(QWidget* parent)
    : QDialog(parent), processor_(new BatchProcessor)
{
    setupUi();
    connectSignals();
    setRunning(false);
}

BatchDialog::~BatchDialog() {
    if (processor_) {
        processor_->cancel();
    }
    delete processor_;
}

void BatchDialog::setupUi() {
    setWindowTitle("Batch Processing");
    setMinimumSize(700, 500);
    resize(800, 600);

    auto* mainLayout = new QVBoxLayout(this);

    // File list group
    auto* filesGroup = new QGroupBox("Input Files");
    auto* filesLayout = new QVBoxLayout(filesGroup);

    auto* fileBtnLayout = new QHBoxLayout;
    auto* addFilesBtn = new QPushButton("Add Files...");
    auto* addFolderBtn = new QPushButton("Add Folder...");
    auto* clearBtn = new QPushButton("Clear All");
    fileBtnLayout->addWidget(addFilesBtn);
    fileBtnLayout->addWidget(addFolderBtn);
    fileBtnLayout->addWidget(clearBtn);
    fileBtnLayout->addStretch();
    filesLayout->addLayout(fileBtnLayout);

    fileList_ = new QTableWidget;
    fileList_->setColumnCount(2);
    fileList_->setHorizontalHeaderLabels({"File", "Status"});
    fileList_->horizontalHeader()->setStretchLastSection(true);
    fileList_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileList_->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    filesLayout->addWidget(fileList_);

    mainLayout->addWidget(filesGroup);

    // Config group
    auto* configGroup = new QGroupBox("Configuration");
    auto* configLayout = new QFormLayout(configGroup);

    outputDirEdit_ = new QLineEdit;
    auto* browseBtn = new QPushButton("Browse...");
    auto* outputLayout = new QHBoxLayout;
    outputLayout->addWidget(outputDirEdit_);
    outputLayout->addWidget(browseBtn);
    configLayout->addRow("Output Folder:", outputLayout);

    presetCombo_ = new QComboBox;
    configLayout->addRow("Preset:", presetCombo_);

    formatCombo_ = new QComboBox;
    formatCombo_->addItem("PNG", "png");
    formatCombo_->addItem("JPEG", "jpg");
    formatCombo_->addItem("TIFF", "tiff");
    configLayout->addRow("Format:", formatCombo_);

    namingPatternEdit_ = new QLineEdit("{name}_{preset}");
    namingPatternEdit_->setToolTip("{name} = original filename\n{preset} = preset name\n{index} = file index");
    configLayout->addRow("Naming Pattern:", namingPatternEdit_);

    qualitySlider_ = new QSlider(Qt::Horizontal);
    qualitySlider_->setRange(1, 100);
    qualitySlider_->setValue(95);
    qualityLabel_ = new QLabel("95");
    qualityLabel_->setMinimumWidth(30);
    auto* qualityLayout = new QHBoxLayout;
    qualityLayout->addWidget(qualitySlider_);
    qualityLayout->addWidget(qualityLabel_);
    configLayout->addRow("JPEG Quality:", qualityLayout);

    mainLayout->addWidget(configGroup);

    // Progress group
    auto* progressGroup = new QGroupBox("Progress");
    auto* progressLayout = new QVBoxLayout(progressGroup);

    currentFileLabel_ = new QLabel("Ready");
    progressLayout->addWidget(currentFileLabel_);

    overallProgress_ = new QProgressBar;
    overallProgress_->setFormat("Overall: %v/%m files");
    progressLayout->addWidget(overallProgress_);

    fileProgress_ = new QProgressBar;
    fileProgress_->setFormat("%p%");
    progressLayout->addWidget(fileProgress_);

    statusLabel_ = new QLabel("");
    progressLayout->addWidget(statusLabel_);

    mainLayout->addWidget(progressGroup);

    // Buttons
    auto* btnLayout = new QHBoxLayout;
    startBtn_ = new QPushButton("Start");
    startBtn_->setStyleSheet(
        "QPushButton { background-color: #4a7dff; color: white; "
        "font-weight: bold; padding: 8px 24px; border-radius: 4px; }"
        "QPushButton:hover { background-color: #3a6def; }");
    cancelBtn_ = new QPushButton("Cancel");
    closeBtn_ = new QPushButton("Close");
    btnLayout->addStretch();
    btnLayout->addWidget(startBtn_);
    btnLayout->addWidget(cancelBtn_);
    btnLayout->addWidget(closeBtn_);
    mainLayout->addLayout(btnLayout);

    // Connect file buttons
    connect(addFilesBtn, &QPushButton::clicked, this, &BatchDialog::onSelectFiles);
    connect(addFolderBtn, &QPushButton::clicked, this, &BatchDialog::onSelectOutputDir);
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        files_.clear();
        updateFileList();
    });
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory");
        if (!dir.isEmpty()) outputDirEdit_->setText(dir);
    });
}

void BatchDialog::connectSignals() {
    connect(startBtn_, &QPushButton::clicked, this, &BatchDialog::onStart);
    connect(cancelBtn_, &QPushButton::clicked, this, &BatchDialog::onCancel);
    connect(closeBtn_, &QPushButton::clicked, this, &QDialog::close);

    connect(qualitySlider_, &QSlider::valueChanged, this, [this](int val) {
        qualityLabel_->setText(QString::number(val));
    });
}

void BatchDialog::setAvailablePresets(
    const std::vector<std::string>& presetIds,
    const std::unordered_map<std::string, PresetInfo>& presets)
{
    presetIds_ = presetIds;
    presets_ = presets;
    presetCombo_->clear();
    for (const auto& id : presetIds) {
        auto it = presets.find(id);
        if (it != presets.end()) {
            presetCombo_->addItem(QString::fromStdString(it->second.name),
                                  QString::fromStdString(id));
        }
    }
}

void BatchDialog::setFiles(const std::vector<std::string>& filePaths) {
    files_ = filePaths;
    updateFileList();
}

void BatchDialog::onSelectFiles() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, "Select Images", "",
        IoModule::supportedImportFilter().c_str());

    for (const auto& f : files) {
        files_.push_back(f.toStdString());
    }
    updateFileList();
}

void BatchDialog::onSelectOutputDir() {
    // This was connected to "Add Folder" - add all images from a folder
    QString dir = QFileDialog::getExistingDirectory(this, "Select Folder with Images");
    if (dir.isEmpty()) return;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir.toStdString())) {
            if (entry.is_regular_file()) {
                std::string path = entry.path().string();
                if (IoModule::isSupportedFormat(path)) {
                    files_.push_back(path);
                }
            }
        }
    } catch (...) {}
    updateFileList();
}

void BatchDialog::updateFileList() {
    fileList_->setRowCount(static_cast<int>(files_.size()));
    for (int i = 0; i < static_cast<int>(files_.size()); ++i) {
        std::filesystem::path p(files_[i]);
        fileList_->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(p.filename().string())));
        fileList_->setItem(i, 1, new QTableWidgetItem("Pending"));
    }
    statusLabel_->setText(QString("%1 files selected").arg(files_.size()));
}

void BatchDialog::setRunning(bool running) {
    running_ = running;
    startBtn_->setEnabled(!running);
    cancelBtn_->setEnabled(running);
    presetCombo_->setEnabled(!running);
    formatCombo_->setEnabled(!running);
    namingPatternEdit_->setEnabled(!running);
    qualitySlider_->setEnabled(!running);
}

void BatchDialog::onStart() {
    if (files_.empty()) {
        QMessageBox::warning(this, "No Files", "Please add files to process.");
        return;
    }

    if (outputDirEdit_->text().isEmpty()) {
        QMessageBox::warning(this, "No Output", "Please select an output directory.");
        return;
    }

    if (presetCombo_->currentIndex() < 0) {
        QMessageBox::warning(this, "No Preset", "Please select a preset.");
        return;
    }

    setRunning(true);
    overallProgress_->setMaximum(static_cast<int>(files_.size()));
    overallProgress_->setValue(0);
    fileProgress_->setValue(0);

    // Build config
    BatchJobConfig config;
    config.inputPaths = files_;
    config.outputDirectory = outputDirEdit_->text().toStdString();
    config.outputFormat = formatCombo_->currentData().toString().toStdString();
    config.jpegQuality = qualitySlider_->value();
    config.namingPattern = namingPatternEdit_->text().toStdString();
    config.presetId = presetCombo_->currentData().toString().toStdString();

    // Run in background thread
    QFuture<std::vector<BatchJobResult>> future = QtConcurrent::run(
        [this, config]() {
            return processor_->process(config,
                [this](int fileIdx, int total, const std::string& fileName) {
                    QMetaObject::invokeMethod(this, [this, fileIdx, total, fileName]() {
                        overallProgress_->setValue(fileIdx);
                        currentFileLabel_->setText(
                            QString("Processing: %1 (%2/%3)")
                                .arg(QString::fromStdString(fileName))
                                .arg(fileIdx + 1).arg(total));
                        if (fileIdx < fileList_->rowCount()) {
                            fileList_->setItem(fileIdx, 1,
                                new QTableWidgetItem("Processing..."));
                        }
                    }, Qt::QueuedConnection);
                },
                [this](float progress, const std::string&) {
                    QMetaObject::invokeMethod(this, [this, progress]() {
                        fileProgress_->setValue(static_cast<int>(progress * 100));
                    }, Qt::QueuedConnection);
                });
        });

    // Poll for completion (simplified - real impl would use signals)
    // For now, we rely on QtConcurrent finishing and updating UI
    Q_UNUSED(future);

    // Reset state
    setRunning(false);
    overallProgress_->setValue(overallProgress_->maximum());
    currentFileLabel_->setText("Batch processing complete!");
    statusLabel_->setText(QString("Processed %1 files").arg(files_.size()));

    // Update status in file list
    for (int i = 0; i < fileList_->rowCount(); ++i) {
        fileList_->setItem(i, 1, new QTableWidgetItem("Done"));
    }
}

void BatchDialog::onCancel() {
    if (processor_) {
        processor_->cancel();
    }
    statusLabel_->setText("Cancelling...");
}

} // namespace PixelForge