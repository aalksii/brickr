#include "openglscene.h"

#include <QtGui>
#include <QGLWidget>
#include <QGraphicsView>
#include <QApplication>
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
    QApplication app(argc, argv);

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
