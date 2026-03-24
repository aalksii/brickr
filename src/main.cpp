#include "openglscene.h"
#include "BrickrCli.h"

#include <QtGui>
#include <QGLWidget>
#include <QGraphicsView>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView()
    {
        setWindowTitle(tr("Brickr"));
    }

protected:
    void resizeEvent(QResizeEvent *event) {
        if (scene())
            scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
        QGraphicsView::resizeEvent(event);
    }
};

int main(int argc, char **argv)
{
    bool cliMode = false;
    for (int i = 1; i < argc; ++i)
    {
        if (QString::fromLocal8Bit(argv[i]) == "--cli")
        {
            cliMode = true;
            break;
        }
    }

    if (cliMode && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));

    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Brickr");
    parser.addHelpOption();

    QCommandLineOption cliOption("cli", "Run Brickr in command-line mode without opening the GUI.");
    QCommandLineOption inputOption(QStringList() << "i" << "input", "Input OBJ or BINVOX file.", "path");
    QCommandLineOption resolutionOption(QStringList() << "r" << "resolution", "Voxelization resolution for mesh inputs.", "value", "30");
    QCommandLineOption preHollowOption("pre-hollow", "Run pre-hollowing before optimization.");
    QCommandLineOption shellThicknessOption("shell-thickness", "Shell thickness used with --pre-hollow.", "value", "2");
    QCommandLineOption autoOptimizeOption("auto-optimize", "Run Brickr's auto optimization.");
    QCommandLineOption finalizeOption("finalize", "Run finalization (post-hollow, solve limits, merge).");
    QCommandLineOption printStatsOption("print-stats", "Print model statistics to stdout.");
    QCommandLineOption exportObjOption("export-obj", "Write the generated LEGO model as OBJ.", "path");
    QCommandLineOption saveInstructionsOption("save-instructions", "Write per-layer instruction files using the given extension (.svg, .png, or .jpg).", "path");

    parser.addOption(cliOption);
    parser.addOption(inputOption);
    parser.addOption(resolutionOption);
    parser.addOption(preHollowOption);
    parser.addOption(shellThicknessOption);
    parser.addOption(autoOptimizeOption);
    parser.addOption(finalizeOption);
    parser.addOption(printStatsOption);
    parser.addOption(exportObjOption);
    parser.addOption(saveInstructionsOption);
    parser.process(app);

    if (parser.isSet(cliOption))
    {
        BrickrCliOptions options;
        options.inputPath = parser.value(inputOption);
        options.voxelizationResolution = parser.value(resolutionOption).toInt();
        options.preHollow = parser.isSet(preHollowOption);
        options.shellThickness = parser.value(shellThicknessOption).toInt();
        options.autoOptimize = parser.isSet(autoOptimizeOption);
        options.finalize = parser.isSet(finalizeOption);
        options.printStats = parser.isSet(printStatsOption);
        options.exportObjPath = parser.value(exportObjOption);
        options.saveInstructionsPath = parser.value(saveInstructionsOption);

        if (options.inputPath.isEmpty())
        {
            std::cerr << "--input is required in --cli mode." << std::endl;
            return 1;
        }

        return BrickrCli::run(options);
    }

    QString startupFile = qEnvironmentVariable("BRICKR_OPEN_FILE");
    int startupResolution = qEnvironmentVariableIntValue("BRICKR_VOX_RES");

    if (argc > 1 && argv[1] != NULL)
        startupFile = QString::fromLocal8Bit(argv[1]);

    if (argc > 2 && argv[2] != NULL)
        startupResolution = QString::fromLocal8Bit(argv[2]).toInt();

    if (startupResolution <= 0)
        startupResolution = 30;

    GraphicsView view;
    view.setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    view.setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view.setScene(new OpenGLScene(1000,800, startupFile, startupResolution));
    view.resize(1000, 800);
    view.show();

    return app.exec();
}
