#include <QtGui/QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include "mavlinkhandler.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setApplicationName("MAVLink Reader");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SpeedyBee");

    // Регистрируем тип в QML системе
    qmlRegisterType<MavlinkHandler>("MavlinkReader", 1, 0, "MavlinkHandler");

    QQmlApplicationEngine engine;

    // Получаем путь к директории с исполняемым файлом
    QString applicationDirPath = QDir::currentPath();
    engine.rootContext()->setContextProperty("applicationDirPath", applicationDirPath);

    // Создаем и регистрируем MAVLink handler
    MavlinkHandler* mavlinkHandler = new MavlinkHandler(&app);
    engine.rootContext()->setContextProperty("mavlinkHandler", mavlinkHandler);

    // Загружаем основной QML файл из модуля MavlinkReader
    engine.loadFromModule("MavlinkReader", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load QML!";
        return -1;
    }

    qDebug() << "✅ MAVLink Reader application started successfully";

    return app.exec();
}
