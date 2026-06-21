#include "gui/MainWindow.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <iostream>
#include <string>

using namespace PixelForge;

int main(int argc, char* argv[]) {
    // High DPI support
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("PixelForge");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PixelForge Team");
    app.setApplicationDisplayName("PixelForge — Image Style & Filter Editor");

    // Command line parsing
    QCommandLineParser parser;
    parser.setApplicationDescription("Desktop Image Style & Filter Editor");
    parser.addHelpOption();
    parser.addVersionOption();

    // Optional: input file as positional argument
    parser.addPositionalArgument("file", "Image file to open");

    // Optional CLI mode flags (future expansion)
    QCommandLineOption presetOption(
        QStringList() << "p" << "preset",
        "Apply preset filter and exit (CLI mode)",
        "preset_id");
    parser.addOption(presetOption);

    QCommandLineOption outputOption(
        QStringList() << "o" << "output",
        "Output file path (CLI mode)",
        "output");
    parser.addOption(outputOption);

    QCommandLineOption cliModeOption(
        QStringList() << "cli",
        "Run in CLI mode (no GUI)");
    parser.addOption(cliModeOption);

    parser.process(app);

    // CLI mode (non-GUI processing)
    if (parser.isSet(cliModeOption)) {
        QStringList positionalArgs = parser.positionalArguments();
        if (positionalArgs.isEmpty()) {
            std::cerr << "Error: No input file specified for CLI mode.\n";
            std::cerr << "Usage: pixelforge --cli -p <preset> -o <output> <input>\n";
            return 1;
        }

        if (!parser.isSet(presetOption)) {
            std::cerr << "Error: No preset specified for CLI mode.\n";
            return 1;
        }

        std::string inputPath = positionalArgs.first().toStdString();
        std::string presetId = parser.value(presetOption).toStdString();
        std::string outputPath = parser.isSet(outputOption)
            ? parser.value(outputOption).toStdString()
            : "output.png";

        std::cout << "PixelForge CLI Mode\n";
        std::cout << "  Input:  " << inputPath << "\n";
        std::cout << "  Preset: " << presetId << "\n";
        std::cout << "  Output: " << outputPath << "\n";

        // TODO: Implement CLI processing engine
        // This would use ColorGradingModule directly without GUI
        std::cout << "CLI mode not yet implemented. Use GUI mode.\n";
        return 0;
    }

    // GUI mode
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow window;
    window.show();

    // Open file from command line if provided
    QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        // Delay file opening to after window is shown
        QString filePath = positionalArgs.first();
        QMetaObject::invokeMethod(&window, [&window, filePath]() {
            // The window handles drag-drop and file opening internally
            // We'll trigger open via a timer
        }, Qt::QueuedConnection);
    }

    return app.exec();
}